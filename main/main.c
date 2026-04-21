/**
 * @file    main.c
 * @brief   示波器面板显示调节控制系统 - 主程序
 * @chip    ESP32-S3
 * @author  宋振豫 (2205040336)
 * @date    2026
 *
 * 系统架构:
 *   - encoder_task  : 编码器事件处理 → 更新通道参数, 发送 UART
 *   - button_task   : 按键事件处理   → 切换工作模式, 更新 LED
 *   - display_task  : LCD 定时刷新   → 显示当前状态 (100ms 周期)
 *   - heartbeat_task: 心跳包发送     → 每秒向上位机报告状态
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "pin_config.h"
#include "panel_state.h"
#include "encoder.h"
#include "button.h"
#include "led.h"
#include "lcd_st7789.h"
#include "uart_comm.h"
#include "wifi_log.h"
#include "waveform.h"
/* #include "adc_sampler.h" */  /* 若已接入 ADC 硬件（非 GPIO4/5），取消注释 */

static const char *TAG = "MAIN";

/* ─── 全局状态 & 事件队列 (panel_state.h 中声明 extern) ─── */
system_state_t g_state;
QueueHandle_t  g_event_queue;

/* ══════════════════════════════════════════════════════
 * 编码器事件处理：调节通道参数索引
 * ════════════════════════════════════════════════════*/
static void handle_encoder(encoder_id_t id, int8_t dir)
{
    xSemaphoreTake(g_state.mutex, portMAX_DELAY);

    channel_param_t *ch = NULL;
    int is_h = 0;

    switch (id) {
    case ENC_CH1_H: ch = &g_state.ch[0]; is_h = 1; break;
    case ENC_CH1_V: ch = &g_state.ch[0]; is_h = 0; break;
    case ENC_CH2_H: ch = &g_state.ch[1]; is_h = 1; break;
    case ENC_CH2_V: ch = &g_state.ch[1]; is_h = 0; break;
    default: break;
    }

    if (ch) {
        if (is_h) {
            int idx = (int)ch->timebase_idx + dir;
            if (idx < 0) idx = 0;
            if (idx >= (int)TIMEBASE_STEPS) idx = (int)TIMEBASE_STEPS - 1;
            ch->timebase_idx = (uint8_t)idx;
        } else {
            int idx = (int)ch->voltscale_idx + dir;
            if (idx < 0) idx = 0;
            if (idx >= (int)VOLTSCALE_STEPS) idx = (int)VOLTSCALE_STEPS - 1;
            ch->voltscale_idx = (uint8_t)idx;
        }
    }

    xSemaphoreGive(g_state.mutex);
    uart_send_encoder(id, dir);
}

/* ══════════════════════════════════════════════════════
 * 按键事件处理
 *   BTN_RUN : 切换 内置波形 ↔ ADC 实采 (MODE_INTERNAL ↔ MODE_RUN)
 *   BTN_TDR : 切换 TDR 模式 (MODE_INTERNAL ↔ MODE_TDR)
 *   BTN_CH1 : 切换 CH1 波形显示 / 隐藏
 *   BTN_CH2 : 切换 CH2 波形显示 / 隐藏
 * ════════════════════════════════════════════════════*/
static void handle_button(button_id_t id, button_state_t state)
{
    if (state != BTN_STATE_PRESSED) return;

    xSemaphoreTake(g_state.mutex, portMAX_DELAY);

    system_mode_t new_mode = g_state.mode;
    int mode_changed = 0;

    switch (id) {
    case BTN_RUN:
        new_mode = (g_state.mode == MODE_RUN) ? MODE_INTERNAL : MODE_RUN;
        mode_changed = 1;
        break;
    case BTN_TDR:
        new_mode = (g_state.mode == MODE_TDR) ? MODE_INTERNAL : MODE_TDR;
        mode_changed = 1;
        break;
    case BTN_CH1:
        g_state.ch[0].visible = !g_state.ch[0].visible;
        ESP_LOGI(TAG, "CH1 %s", g_state.ch[0].visible ? "show" : "hide");
        break;
    case BTN_CH2:
        g_state.ch[1].visible = !g_state.ch[1].visible;
        ESP_LOGI(TAG, "CH2 %s", g_state.ch[1].visible ? "show" : "hide");
        break;
    default:
        break;
    }

    if (mode_changed) {
        g_state.mode = new_mode;
        /* RUN → ADC 源，其余 → 内置正弦/三角 */
        waveform_set_source(new_mode == MODE_RUN ? WAVE_SRC_ADC : WAVE_SRC_SINE);
    }

    system_state_t snap = g_state;
    xSemaphoreGive(g_state.mutex);

    led_apply(&snap);
    uart_send_button(id, state);
    if (mode_changed) uart_send_mode(new_mode);
}

/* ══════════════════════════════════════════════════════
 * FreeRTOS 任务
 * ════════════════════════════════════════════════════*/

/* 事件处理任务 (编码器 + 按键) */
static void event_task(void *pv)
{
    panel_event_t evt;
    while (1) {
        if (xQueueReceive(g_event_queue, &evt, portMAX_DELAY) == pdTRUE) {
            switch (evt.type) {
            case EVT_ENCODER:
                handle_encoder(evt.encoder.id, evt.encoder.direction);
                break;
            case EVT_BUTTON:
                handle_button(evt.button.id, evt.button.state);
                break;
            }
        }
    }
}

/* 显示刷新任务 (100ms 周期) */
static void display_task(void *pv)
{
    while (1) {
        xSemaphoreTake(g_state.mutex, portMAX_DELAY);
        system_state_t snap = g_state;
        xSemaphoreGive(g_state.mutex);
        waveform_render(&snap);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* 心跳任务 (1s 周期) */
static void heartbeat_task(void *pv)
{
    while (1) {
        uart_send_heartbeat();
        uart_send_status();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ══════════════════════════════════════════════════════
 * ADC 采集完成回调 → 喂入波形模块
 * ════════════════════════════════════════════════════*/
static void adc_cb(uint8_t ch, const uint16_t *samples, uint16_t count)
{
    waveform_feed(ch, samples, count);
}

/* ══════════════════════════════════════════════════════
 * app_main
 * ════════════════════════════════════════════════════*/
void app_main(void)
{
    /* 0. 启动 WiFi AP 并重定向日志（须最先初始化，后续所有模块日志均通过 WiFi 输出） */
    wifi_log_init();

    ESP_LOGI(TAG, "=== OSC Panel Control System starting ===");

    /* 1. 初始化全局状态 (开机: 内置波形模式, CH1/CH2 均可见) */
    g_state.mutex = xSemaphoreCreateMutex();
    g_state.mode  = MODE_INTERNAL;
    g_state.ch[0].timebase_idx  = TIMEBASE_DEFAULT;
    g_state.ch[0].voltscale_idx = VOLTSCALE_DEFAULT;
    g_state.ch[0].visible       = 1;
    g_state.ch[1].timebase_idx  = TIMEBASE_DEFAULT;
    g_state.ch[1].voltscale_idx = VOLTSCALE_DEFAULT;
    g_state.ch[1].visible       = 1;

    /* 2. 事件队列 (最多32个事件) */
    g_event_queue = xQueueCreate(32, sizeof(panel_event_t));

    /* 3. 初始化各外设 */
    uart_comm_init();
    led_init();
    lcd_init();
    waveform_init();          /* 默认内置波形 (CH1 正弦 + CH2 三角) */
    encoder_init();
    button_init();

    /* 上电根据初始状态刷新 LED (CH1/CH2 亮, RUN/TDR 灭) */
    led_apply(&g_state);

    /* ── 启用 ADC 实采时取消下方注释，并确认 adc_sampler.h 中的 GPIO
     *    未与 encoder/button 冲突 (默认 GPIO4/5 会与 ENC2/ENC3 冲突) ──
    adc_sampler_init(adc_cb);
    adc_sampler_start();
    ── */

    ESP_LOGI(TAG, "All peripherals initialized");

    /* 4. 创建 FreeRTOS 任务 */
    xTaskCreate(event_task,     "event",     4096, NULL, 10, NULL);
    xTaskCreate(display_task,   "display",   4096, NULL,  5, NULL);
    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL,  3, NULL);

    ESP_LOGI(TAG, "Tasks created, system running");
}

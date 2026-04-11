/**
 * @file    adc_sampler.h
 * @brief   双通道 ADC 采样接口
 *
 * ┌──────────────────────────────────────────────────────────────┐
 * │  重要：ESP32-S3 ADC 引脚约束                                   │
 * │  • ADC1：GPIO 1~10（可与 WiFi 共存）                           │
 * │  • ADC2：GPIO11~20（WiFi 开启时不可用）                         │
 * │  本板 GPIO1~12 已被编码器/按键占用，使用前请在硬件上          │
 * │  将示波器输入信号接到空闲的 ADC1 引脚并修改下方宏定义。       │
 * └──────────────────────────────────────────────────────────────┘
 *
 * 默认占位引脚（请根据实际硬件修改）:
 *   ADC_CH1_GPIO  GPIO_NUM_4   ← 改成实际接线引脚
 *   ADC_CH2_GPIO  GPIO_NUM_5   ← 改成实际接线引脚
 *
 * 切换到 ADC 模式:
 *   1. 在 app_main 中调用 adc_sampler_init(my_callback)
 *   2. 调用 adc_sampler_start()
 *   3. 在回调里调用 waveform_feed(ch, samples, count)
 *   4. 调用 waveform_set_source(WAVE_SRC_ADC)
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"

/* ─── 修改这两行以匹配实际硬件接线 ─── */
#define ADC_CH1_GPIO      GPIO_NUM_4    /* CH1 模拟输入引脚 */
#define ADC_CH2_GPIO      GPIO_NUM_5    /* CH2 模拟输入引脚 */

#define ADC_SAMPLE_COUNT  240           /* 每次采集点数（= LCD 宽度）*/
#define ADC_RESOLUTION    12            /* ADC 位数，输出 0~4095    */

/**
 * @brief ADC 采集完成回调
 * @param ch      通道索引：0=CH1, 1=CH2
 * @param samples ADC_SAMPLE_COUNT 个 12-bit 原始值（0~4095）
 * @param count   实际点数（= ADC_SAMPLE_COUNT）
 */
typedef void (*adc_done_cb_t)(uint8_t ch,
                               const uint16_t *samples,
                               uint16_t count);

/**
 * @brief 初始化 ADC（oneshot 模式）并创建采样任务
 * @param cb  采集完成回调，NULL 则只初始化不启动任务
 */
esp_err_t adc_sampler_init(adc_done_cb_t cb);

/** @brief 开始采样（启动后台任务） */
esp_err_t adc_sampler_start(void);

/** @brief 停止采样（挂起后台任务） */
esp_err_t adc_sampler_stop(void);

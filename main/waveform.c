/**
 * @file    waveform.c
 * @brief   示波器波形显示实现
 */

#include "waveform.h"
#include "lcd_st7789.h"
#include "panel_state.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "WAVE";

/* ══════════════════════════════════════════════════════
 * 颜色定义
 * ════════════════════════════════════════════════════*/
#define COL_BG         0x0000   /* 纯黑背景          */
#define COL_GRID       0x2104   /* 极暗灰 — 网格主线  */
#define COL_GRID_CTR   0x4208   /* 稍亮灰 — 中心线    */
#define COL_CH1        0x07E0   /* 绿色  — CH1        */
#define COL_CH2        0x07FF   /* 青色  — CH2        */
#define COL_INFO_BG    0x1082   /* 顶栏背景           */
#define COL_TRIG       0xFD20   /* 橙色  — 触发电平   */

/* ══════════════════════════════════════════════════════
 * 内部状态
 * ════════════════════════════════════════════════════*/
static wave_src_t        s_src = WAVE_SRC_SINE;
static uint16_t          s_adc[2][WAVE_SAMPLES]; /* 12-bit ADC 原始值  */
static SemaphoreHandle_t s_mutex;

/* ══════════════════════════════════════════════════════
 * 辅助：时基索引 → 正弦波每屏周期数
 * timebase 越快(索引越小) → 显示越多周期
 * ════════════════════════════════════════════════════*/
static float timebase_to_cycles(uint8_t idx)
{
    if (idx < 7)  return 8.0f;   /* 2ns  ~ 200ns  */
    if (idx < 14) return 4.0f;   /* 500ns ~ 50us  */
    if (idx < 21) return 2.0f;   /* 100us ~ 50ms  */
    return 1.0f;                  /* 100ms ~ 5s    */
}

/* ══════════════════════════════════════════════════════
 * 辅助：电压档索引 → 每格电压 (mV)
 *   idx  0= 1mV  1= 2mV  2= 5mV  3=10mV  4=20mV  5=50mV
 *        6=100mV 7=200mV 8=500mV 9= 1V  10= 2V  11= 5V  12=10V
 * ════════════════════════════════════════════════════*/
static float voltscale_to_mv_per_div(uint8_t idx)
{
    static const float tbl[] = {
        1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000
    };
    if (idx >= 13) idx = 12;
    return tbl[idx];
}

/* ══════════════════════════════════════════════════════
 * 生成正弦波 Y 像素坐标（相对波形区顶部）
 *   cycles      : 一屏周期数
 *   amp_divs    : 振幅（格数，0.5格 = 峰值偏移半格）
 *   phase_offset: 相位偏移 (弧度)，让双通道不重叠
 * ════════════════════════════════════════════════════*/
static void gen_sine_pixels(uint8_t *ypx, uint16_t n,
                             float cycles, float amp_divs, float phase_offset)
{
    float center = WAVE_AREA_H / 2.0f;              /* 波形区垂直中心    */
    float amp_px = amp_divs * (float)WAVE_CELL_H;  /* 振幅（像素）      */

    for (uint16_t i = 0; i < n; i++) {
        float phase = 2.0f * (float)M_PI * cycles * i / (float)n + phase_offset;
        float y = center - amp_px * sinf(phase);
        /* 限幅 */
        if (y < 0.0f)                   y = 0.0f;
        if (y >= (float)WAVE_AREA_H)    y = (float)(WAVE_AREA_H - 1);
        ypx[i] = (uint8_t)y;
    }
}

/* ══════════════════════════════════════════════════════
 * 生成三角波 Y 像素坐标（相对波形区顶部）
 *   cycles      : 一屏周期数
 *   amp_divs    : 振幅（格数）
 *   phase_offset: 归一化相位偏移 (0~1 对应 0~2π)
 * ════════════════════════════════════════════════════*/
static void gen_triangle_pixels(uint8_t *ypx, uint16_t n,
                                 float cycles, float amp_divs, float phase_offset)
{
    float center = WAVE_AREA_H / 2.0f;
    float amp_px = amp_divs * (float)WAVE_CELL_H;

    for (uint16_t i = 0; i < n; i++) {
        /* 相位 ∈ [0,1) */
        float t = cycles * (float)i / (float)n + phase_offset;
        t -= (float)((int)t);            /* 取小数部分 */
        /* 对称三角: 0→1→0  峰值在 0.5 */
        float tri = (t < 0.5f) ? (4.0f * t - 1.0f)
                                : (3.0f - 4.0f * t);
        float y = center - amp_px * tri;
        if (y < 0.0f)                 y = 0.0f;
        if (y >= (float)WAVE_AREA_H)  y = (float)(WAVE_AREA_H - 1);
        ypx[i] = (uint8_t)y;
    }
}

/* ══════════════════════════════════════════════════════
 * 将 ADC 原始值（0~4095）映射到波形区 Y 像素
 *   ADC 4095 → 顶部 (y=0)
 *   ADC 2048 → 中心
 *   ADC    0 → 底部 (y=WAVE_AREA_H-1)
 *   volt_idx 决定可见幅度：每格 mv_per_div
 *   ESP32-S3 ADC 满幅 ≈ 3300mV
 * ════════════════════════════════════════════════════*/
static void adc_to_pixels(uint8_t *ypx, const uint16_t *raw, uint16_t n,
                           uint8_t volt_idx)
{
    float mv_per_div   = voltscale_to_mv_per_div(volt_idx);
    float mv_full      = 3300.0f;           /* ADC 量程 mV        */
    float px_per_mv    = (float)WAVE_CELL_H / mv_per_div;
    float center_mv    = mv_full / 2.0f;    /* ADC 中点电压       */
    float center_px    = WAVE_AREA_H / 2.0f;

    for (uint16_t i = 0; i < n; i++) {
        float mv = raw[i] * mv_full / 4095.0f;
        float y  = center_px - (mv - center_mv) * px_per_mv;
        if (y < 0.0f)                 y = 0.0f;
        if (y >= (float)WAVE_AREA_H)  y = (float)(WAVE_AREA_H - 1);
        ypx[i] = (uint8_t)y;
    }
}

/* ══════════════════════════════════════════════════════
 * 绘制网格（graticule）
 * ════════════════════════════════════════════════════*/
static void draw_graticule(void)
{
    /* 横线 */
    for (int r = 0; r <= WAVE_GRID_ROWS; r++) {
        uint16_t y = WAVE_AREA_Y + r * WAVE_CELL_H;
        uint16_t col = (r == WAVE_GRID_ROWS / 2) ? COL_GRID_CTR : COL_GRID;
        lcd_draw_hline(0, y, WAVE_AREA_W, col);
    }
    /* 竖线 */
    for (int c = 0; c <= WAVE_GRID_COLS; c++) {
        uint16_t x = c * WAVE_CELL_W;
        uint16_t col = (c == WAVE_GRID_COLS / 2) ? COL_GRID_CTR : COL_GRID;
        lcd_draw_vline(x, WAVE_AREA_Y, WAVE_AREA_H, col);
    }
    /* 中心十字刻度短划 */
    uint16_t cx = WAVE_AREA_W / 2;
    uint16_t cy = WAVE_AREA_Y + WAVE_AREA_H / 2;
    for (int r = 0; r <= WAVE_GRID_ROWS; r++) {
        uint16_t y = WAVE_AREA_Y + r * WAVE_CELL_H;
        lcd_fill_rect(cx - 2, y, 5, 1, COL_GRID_CTR);
    }
    for (int c = 0; c <= WAVE_GRID_COLS; c++) {
        uint16_t x = c * WAVE_CELL_W;
        lcd_fill_rect(x, cy - 2, 1, 5, COL_GRID_CTR);
    }
}

/* ══════════════════════════════════════════════════════
 * 绘制顶部信息栏
 * ════════════════════════════════════════════════════*/
static void draw_info_bar(const system_state_t *st)
{
    /* 背景 (16px = 两行 8px 字符) */
    lcd_fill_rect(0, 0, LCD_WIDTH, WAVE_INFO_H, COL_INFO_BG);

    char buf[48];
    uint16_t ch1_col = st->ch[0].visible ? COL_CH1 : 0x4208;
    uint16_t ch2_col = st->ch[1].visible ? COL_CH2 : 0x4208;

    /* 上行 y=0 — CH1 时基 + 电压档 */
    snprintf(buf, sizeof(buf), "1H:%-6s V:%s",
             TIMEBASE_STR[st->ch[0].timebase_idx],
             VOLTSCALE_STR[st->ch[0].voltscale_idx]);
    lcd_draw_string(2, 0, buf, ch1_col, COL_INFO_BG, 1);

    /* 右侧跨两行 — 模式 (scale=2: 12×16px, 占满 info bar) */
    const char *mode_s;
    uint16_t    mode_c;
    switch (st->mode) {
    case MODE_RUN:      mode_s = "RUN";  mode_c = 0x07E0; break;
    case MODE_TDR:      mode_s = "TDR";  mode_c = 0xF81F; break;
    case MODE_INTERNAL:
    default:            mode_s = "INT";  mode_c = 0xFFFF; break;
    }
    lcd_draw_string(LCD_WIDTH - (int)strlen(mode_s) * 12 - 2, 0,
                    mode_s, mode_c, COL_INFO_BG, 2);

    /* 下行 y=8 — CH2 时基 + 电压档 */
    snprintf(buf, sizeof(buf), "2H:%-6s V:%s",
             TIMEBASE_STR[st->ch[1].timebase_idx],
             VOLTSCALE_STR[st->ch[1].voltscale_idx]);
    lcd_draw_string(2, 8, buf, ch2_col, COL_INFO_BG, 1);
}

/* ══════════════════════════════════════════════════════
 * 绘制单条波形（连接相邻采样点）
 *   ypx : WAVE_SAMPLES 个 Y 坐标 (相对波形区顶部)
 *   col : 波形颜色
 * ════════════════════════════════════════════════════*/
static void draw_trace(const uint8_t *ypx, uint16_t color)
{
    for (uint16_t x = 0; x < WAVE_SAMPLES; x++) {
        uint16_t y_abs = WAVE_AREA_Y + ypx[x];

        if (x == 0) {
            /* 第一列只画一个像素 */
            lcd_fill_rect(x, y_abs, 1, 1, color);
        } else {
            /* 连接前一列到当前列（垂直线段） */
            uint16_t y_prev = WAVE_AREA_Y + ypx[x - 1];
            uint16_t y_top  = (y_abs < y_prev) ? y_abs  : y_prev;
            uint16_t y_bot  = (y_abs > y_prev) ? y_abs  : y_prev;
            lcd_draw_vline(x, y_top, y_bot - y_top + 1, color);
        }
    }
}

/* ══════════════════════════════════════════════════════
 * 公开接口实现
 * ════════════════════════════════════════════════════*/
void waveform_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    memset(s_adc, 0, sizeof(s_adc));
    lcd_fill(COL_BG);
    ESP_LOGI(TAG, "waveform_init done (src=SINE)");
}

void waveform_set_source(wave_src_t src)
{
    s_src = src;
    ESP_LOGI(TAG, "source -> %s", src == WAVE_SRC_SINE ? "SINE" : "ADC");
}

void waveform_feed(uint8_t ch, const uint16_t *samples, uint16_t count)
{
    if (ch > 1 || !samples) return;
    if (count > WAVE_SAMPLES) count = WAVE_SAMPLES;

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    memcpy(s_adc[ch], samples, count * sizeof(uint16_t));
    xSemaphoreGive(s_mutex);
}

void waveform_render(const system_state_t *state)
{
    /* 1. 清波形区 */
    lcd_fill_rect(0, WAVE_AREA_Y, WAVE_AREA_W, WAVE_AREA_H, COL_BG);

    /* 2. 网格 */
    draw_graticule();

    /* 3. 波形 */
    static uint8_t ypx[2][WAVE_SAMPLES];

    if (s_src == WAVE_SRC_SINE) {
        /* 内置测试波形: CH1=正弦, CH2=三角 */
        static float s_phase = 0.0f;
        s_phase += 0.15f;
        if (s_phase > 2.0f * (float)M_PI) s_phase -= 2.0f * (float)M_PI;

        float cyc1 = timebase_to_cycles(state->ch[0].timebase_idx);
        float cyc2 = timebase_to_cycles(state->ch[1].timebase_idx);
        gen_sine_pixels(ypx[0], WAVE_SAMPLES, cyc1, 2.8f, s_phase);
        gen_triangle_pixels(ypx[1], WAVE_SAMPLES, cyc2, 2.0f,
                            s_phase / (2.0f * (float)M_PI));
    } else {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        adc_to_pixels(ypx[0], s_adc[0], WAVE_SAMPLES, state->ch[0].voltscale_idx);
        adc_to_pixels(ypx[1], s_adc[1], WAVE_SAMPLES, state->ch[1].voltscale_idx);
        xSemaphoreGive(s_mutex);
    }

    /* 按 visibility 绘制 (CH2 先下层, CH1 后上层) */
    if (state->ch[1].visible) draw_trace(ypx[1], COL_CH2);
    if (state->ch[0].visible) draw_trace(ypx[0], COL_CH1);

    /* 4. 顶部信息栏 */
    draw_info_bar(state);

    /* 5. 一次性刷新到 LCD（消除撕裂） */
    lcd_flush();
}

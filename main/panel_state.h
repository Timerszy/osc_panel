#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/* ─── 系统工作模式 ───
 *   INTERNAL : 开机默认，显示内置三角+正弦测试波形
 *   RUN      : ADC 实采模式（按 RUN 进入）
 *   TDR      : TDR 模式（按 TDR 进入）                         */
typedef enum {
    MODE_INTERNAL = 0,   // 内置波形（默认）
    MODE_RUN      = 1,   // ADC 实采
    MODE_TDR      = 2,   // TDR
} system_mode_t;

/* ─── 编码器ID ─── */
typedef enum {
    ENC_CH1_H = 0,   // CH1 横坐标 (时基)
    ENC_CH1_V = 1,   // CH1 纵坐标 (电压档)
    ENC_CH2_H = 2,   // CH2 横坐标 (时基)
    ENC_CH2_V = 3,   // CH2 纵坐标 (电压档)
    ENC_COUNT  = 4,
} encoder_id_t;

/* ─── 按键ID ─── */
typedef enum {
    BTN_RUN   = 0,   // 切换内置波形 / ADC 模式
    BTN_CH1   = 1,   // CH1 波形 显/隐
    BTN_TDR   = 2,   // 切换 TDR 模式
    BTN_CH2   = 3,   // CH2 波形 显/隐
    BTN_COUNT = 4,
} button_id_t;

typedef enum {
    BTN_STATE_RELEASED = 0,
    BTN_STATE_PRESSED  = 1,
} button_state_t;

/* ─── 时基档位字符串 (29档) ─── */
static const char *const TIMEBASE_STR[] = {
    "2ns", "5ns", "10ns", "20ns", "50ns",
    "100ns", "200ns", "500ns",
    "1us", "2us", "5us", "10us", "20us", "50us",
    "100us", "200us", "500us",
    "1ms", "2ms", "5ms", "10ms", "20ms", "50ms",
    "100ms", "200ms", "500ms",
    "1s", "2s", "5s",
};
#define TIMEBASE_STEPS  (sizeof(TIMEBASE_STR) / sizeof(TIMEBASE_STR[0]))
#define TIMEBASE_DEFAULT 14  // 100us/div

/* ─── 电压档位字符串 (13档) ─── */
static const char *const VOLTSCALE_STR[] = {
    "1mV", "2mV", "5mV", "10mV", "20mV", "50mV",
    "100mV", "200mV", "500mV",
    "1V", "2V", "5V", "10V",
};
#define VOLTSCALE_STEPS  (sizeof(VOLTSCALE_STR) / sizeof(VOLTSCALE_STR[0]))
#define VOLTSCALE_DEFAULT 9  // 1V/div

/* ─── 通道参数 ─── */
typedef struct {
    uint8_t timebase_idx;   // 横坐标档位索引
    uint8_t voltscale_idx;  // 纵坐标档位索引
    uint8_t visible;        // 1 = 显示, 0 = 隐藏
} channel_param_t;

/* ─── 系统状态 ─── */
typedef struct {
    system_mode_t   mode;
    channel_param_t ch[2];  // ch[0]=CH1, ch[1]=CH2
    SemaphoreHandle_t mutex;
} system_state_t;

/* ─── 事件类型 ─── */
typedef enum {
    EVT_ENCODER = 0,
    EVT_BUTTON  = 1,
} panel_event_type_t;

/* ─── 事件结构体 ─── */
typedef struct {
    panel_event_type_t type;
    union {
        struct {
            encoder_id_t  id;
            int8_t        direction;  // +1=顺时针, -1=逆时针
        } encoder;
        struct {
            button_id_t   id;
            button_state_t state;
        } button;
    };
} panel_event_t;

/* ─── 全局状态 & 事件队列 (main.c 中定义) ─── */
extern system_state_t g_state;
extern QueueHandle_t  g_event_queue;

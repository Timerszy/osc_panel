#include "button.h"
#include "pin_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

static const char *TAG = "BUTTON";

#define DEBOUNCE_MS  20

typedef struct {
    gpio_num_t   pin;
    button_id_t  id;
    TimerHandle_t timer;
    volatile uint8_t raw_state;  // 0=释放, 1=按下(低电平有效)
} btn_ctx_t;

static btn_ctx_t s_btn[BTN_COUNT] = {
    {BTN_RUN_PIN, BTN_RUN, NULL, 0},
    {BTN_CH1_PIN, BTN_CH1, NULL, 0},
    {BTN_TDR_PIN, BTN_TDR, NULL, 0},
    {BTN_CH2_PIN, BTN_CH2, NULL, 0},
};

/* 消抖定时器回调：延时后确认按键状态 */
static void debounce_timer_cb(TimerHandle_t xTimer)
{
    btn_ctx_t *ctx = (btn_ctx_t *)pvTimerGetTimerID(xTimer);
    uint8_t level = (uint8_t)gpio_get_level(ctx->pin);
    button_state_t state = (level == 0) ? BTN_STATE_PRESSED : BTN_STATE_RELEASED;

    if ((uint8_t)state == ctx->raw_state) return;  // 状态未变，忽略
    ctx->raw_state = (uint8_t)state;

    panel_event_t evt = {
        .type   = EVT_BUTTON,
        .button = {.id = ctx->id, .state = state},
    };
    xQueueSend(g_event_queue, &evt, 0);
}

/* GPIO 中断：仅触发消抖定时器 */
static void IRAM_ATTR btn_isr_handler(void *arg)
{
    btn_ctx_t *ctx = (btn_ctx_t *)arg;
    BaseType_t woken = pdFALSE;
    xTimerResetFromISR(ctx->timer, &woken);
    if (woken) portYIELD_FROM_ISR();
}

void button_init(void)
{
    gpio_config_t cfg = {
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };

    for (int i = 0; i < BTN_COUNT; i++) {
        cfg.pin_bit_mask = (1ULL << s_btn[i].pin);
        gpio_config(&cfg);

        s_btn[i].timer = xTimerCreate(
            "btn_debounce",
            pdMS_TO_TICKS(DEBOUNCE_MS),
            pdFALSE,            // 单次定时器
            &s_btn[i],
            debounce_timer_cb
        );

        gpio_isr_handler_add(s_btn[i].pin, btn_isr_handler, &s_btn[i]);
    }

    ESP_LOGI(TAG, "4x button initialized");
}

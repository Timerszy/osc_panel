#include "encoder.h"
#include "pin_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "ENCODER";

/* 正交解码状态表: state[上一状态][当前状态] => 方向 */
static const int8_t enc_table[4][4] = {
    { 0, -1,  1,  0},
    { 1,  0,  0, -1},
    {-1,  0,  0,  1},
    { 0,  1, -1,  0},
};

typedef struct {
    gpio_num_t  pin_a;
    gpio_num_t  pin_b;
    encoder_id_t id;
    volatile uint8_t last_state;
} enc_ctx_t;

static enc_ctx_t s_enc[ENC_COUNT] = {
    {ENC1_PIN_A, ENC1_PIN_B, ENC_CH1_H, 0},
    {ENC2_PIN_A, ENC2_PIN_B, ENC_CH1_V, 0},
    {ENC3_PIN_A, ENC3_PIN_B, ENC_CH2_H, 0},
    {ENC4_PIN_A, ENC4_PIN_B, ENC_CH2_V, 0},
};

/* ISR：所有编码器共用，通过 arg 区分 */
static void IRAM_ATTR enc_isr_handler(void *arg)
{
    enc_ctx_t *ctx = (enc_ctx_t *)arg;
    uint8_t curr = ((uint8_t)gpio_get_level(ctx->pin_a) << 1) |
                    (uint8_t)gpio_get_level(ctx->pin_b);
    int8_t dir = enc_table[ctx->last_state & 0x03][curr & 0x03];
    ctx->last_state = curr;

    if (dir == 0) return;

    panel_event_t evt = {
        .type = EVT_ENCODER,
        .encoder = {.id = ctx->id, .direction = dir},
    };
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(g_event_queue, &evt, &woken);
    if (woken) portYIELD_FROM_ISR();
}

void encoder_init(void)
{
    gpio_config_t cfg = {
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };

    gpio_install_isr_service(0);

    for (int i = 0; i < ENC_COUNT; i++) {
        cfg.pin_bit_mask = (1ULL << s_enc[i].pin_a) | (1ULL << s_enc[i].pin_b);
        gpio_config(&cfg);

        /* 读初始状态 */
        s_enc[i].last_state = ((uint8_t)gpio_get_level(s_enc[i].pin_a) << 1) |
                               (uint8_t)gpio_get_level(s_enc[i].pin_b);

        gpio_isr_handler_add(s_enc[i].pin_a, enc_isr_handler, &s_enc[i]);
        gpio_isr_handler_add(s_enc[i].pin_b, enc_isr_handler, &s_enc[i]);
    }

    ESP_LOGI(TAG, "4x encoder initialized");
}

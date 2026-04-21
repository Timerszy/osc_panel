#include "led.h"
#include "pin_config.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LED";

/* 按 button_id_t 顺序排列：RUN / CH1 / TDR / CH2 */
static const gpio_num_t s_led_pins[BTN_COUNT] = {
    LED_RUN_PIN,
    LED_CH1_PIN,
    LED_TDR_PIN,
    LED_CH2_PIN,
};

void led_init(void)
{
    gpio_config_t cfg = {
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << LED_RUN_PIN) |
                        (1ULL << LED_CH1_PIN) |
                        (1ULL << LED_TDR_PIN) |
                        (1ULL << LED_CH2_PIN) |
                        (1ULL << WS2812_PIN),
    };
    gpio_config(&cfg);
    gpio_set_level(WS2812_PIN, 0);   /* 关闭板载 WS2812B RGB LED */

    /* 全部熄灭 */
    for (int i = 0; i < BTN_COUNT; i++) {
        gpio_set_level(s_led_pins[i], 0);
    }
    ESP_LOGI(TAG, "LEDs initialized (RUN/CH1/TDR/CH2, WS2812 off)");
}

void led_apply(const system_state_t *st)
{
    gpio_set_level(LED_RUN_PIN, st->mode == MODE_RUN ? 1 : 0);
    gpio_set_level(LED_TDR_PIN, st->mode == MODE_TDR ? 1 : 0);
    gpio_set_level(LED_CH1_PIN, st->ch[0].visible ? 1 : 0);
    gpio_set_level(LED_CH2_PIN, st->ch[1].visible ? 1 : 0);
}

void led_set(button_id_t id, uint8_t on)
{
    if (id < BTN_COUNT) {
        gpio_set_level(s_led_pins[id], on ? 1 : 0);
    }
}

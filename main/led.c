#include "led.h"
#include "pin_config.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LED";

static const gpio_num_t s_led_pins[3] = {
    LED_RUN_PIN,
    LED_SCOPE_PIN,
    LED_TDR_PIN,
};

void led_init(void)
{
    gpio_config_t cfg = {
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << LED_RUN_PIN) |
                        (1ULL << LED_SCOPE_PIN) |
                        (1ULL << LED_TDR_PIN) |
                        (1ULL << WS2812_PIN),
    };
    gpio_config(&cfg);
    gpio_set_level(WS2812_PIN, 0);   /* 关闭板载 WS2812B RGB LED */
    led_update(MODE_IDLE);
    ESP_LOGI(TAG, "LEDs initialized (WS2812 off)");
}

void led_update(system_mode_t mode)
{
    gpio_set_level(LED_RUN_PIN,   mode == MODE_RUN   ? 1 : 0);
    gpio_set_level(LED_SCOPE_PIN, mode == MODE_SCOPE ? 1 : 0);
    gpio_set_level(LED_TDR_PIN,   mode == MODE_TDR   ? 1 : 0);
}

void led_set(button_id_t id, uint8_t on)
{
    if (id < BTN_RESET) {
        gpio_set_level(s_led_pins[id], on ? 1 : 0);
    }
}

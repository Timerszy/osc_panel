/**
 * @file    adc_sampler.c
 * @brief   双通道 ADC oneshot 采样实现
 *
 * 采样方式：Oneshot（逐点读取），适合低速调试。
 * 后续如需高速采样可改用 Continuous 模式（esp_adc/adc_continuous.h）。
 */

#include "adc_sampler.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ADC";

/* ── 内部状态 ── */
static adc_oneshot_unit_handle_t s_adc_hdl   = NULL;
static adc_cali_handle_t         s_cali[2]   = {NULL, NULL};
static adc_done_cb_t             s_cb        = NULL;
static TaskHandle_t              s_task      = NULL;
static volatile bool             s_running   = false;

/* GPIO → ADC1 通道映射（ESP32-S3） */
static adc_channel_t gpio_to_adc1_ch(gpio_num_t gpio)
{
    /* ADC1: GPIO1=CH0 … GPIO10=CH9 */
    if (gpio >= GPIO_NUM_1 && gpio <= GPIO_NUM_10) {
        return (adc_channel_t)(gpio - GPIO_NUM_1);
    }
    ESP_LOGE(TAG, "GPIO%d is not an ADC1 pin on ESP32-S3", gpio);
    return ADC_CHANNEL_0;
}

/* ── 采样任务 ── */
static void adc_task(void *arg)
{
    static uint16_t buf[ADC_SAMPLE_COUNT];
    adc_channel_t ch[2] = {
        gpio_to_adc1_ch(ADC_CH1_GPIO),
        gpio_to_adc1_ch(ADC_CH2_GPIO),
    };

    ESP_LOGI(TAG, "ADC task started  CH1=GPIO%d(ADC1_CH%d)  CH2=GPIO%d(ADC1_CH%d)",
             ADC_CH1_GPIO, ch[0], ADC_CH2_GPIO, ch[1]);

    while (1) {
        if (!s_running) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        /* 轮流采两个通道 */
        for (int c = 0; c < 2; c++) {
            for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
                int raw = 0;
                adc_oneshot_read(s_adc_hdl, ch[c], &raw);
                buf[i] = (uint16_t)raw;
            }
            if (s_cb) {
                s_cb((uint8_t)c, buf, ADC_SAMPLE_COUNT);
            }
        }

        /* 采样间隔：约 50ms → 每帧 ~20Hz */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ── 校准初始化（可选，无校准曲线时静默失败） ── */
static void cali_init(adc_unit_t unit, adc_channel_t ch,
                      adc_atten_t atten, adc_cali_handle_t *out)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg = {
        .unit_id  = unit,
        .chan     = ch,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cfg, out) == ESP_OK) {
        ESP_LOGI(TAG, "Curve fitting calibration OK (ch%d)", ch);
        return;
    }
#endif
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cfg2 = {
        .unit_id       = unit,
        .atten         = atten,
        .bitwidth      = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cfg2, out) == ESP_OK) {
        ESP_LOGI(TAG, "Line fitting calibration OK (ch%d)", ch);
        return;
    }
#endif
    ESP_LOGW(TAG, "No calibration available (ch%d), raw values only", ch);
    *out = NULL;
}

/* ════════════════════════════════════════════════════
 * 公开接口
 * ════════════════════════════════════════════════════*/
esp_err_t adc_sampler_init(adc_done_cb_t cb)
{
    s_cb = cb;

    /* 初始化 ADC1 */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_hdl));

    /* 配置两个通道：11dB 衰减（量程 0~3.1V）*/
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,   /* 12-bit */
        .atten    = ADC_ATTEN_DB_12,        /* 0~3.1V  */
    };
    adc_channel_t ch1 = gpio_to_adc1_ch(ADC_CH1_GPIO);
    adc_channel_t ch2 = gpio_to_adc1_ch(ADC_CH2_GPIO);
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_hdl, ch1, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_hdl, ch2, &chan_cfg));

    /* 尝试校准 */
    cali_init(ADC_UNIT_1, ch1, ADC_ATTEN_DB_12, &s_cali[0]);
    cali_init(ADC_UNIT_1, ch2, ADC_ATTEN_DB_12, &s_cali[1]);

    /* 创建采样任务（初始挂起） */
    if (cb) {
        xTaskCreate(adc_task, "adc_sampler", 4096, NULL, 6, &s_task);
    }

    ESP_LOGI(TAG, "adc_sampler_init OK  CH1=GPIO%d  CH2=GPIO%d",
             ADC_CH1_GPIO, ADC_CH2_GPIO);
    return ESP_OK;
}

esp_err_t adc_sampler_start(void)
{
    s_running = true;
    ESP_LOGI(TAG, "ADC sampling started");
    return ESP_OK;
}

esp_err_t adc_sampler_stop(void)
{
    s_running = false;
    ESP_LOGI(TAG, "ADC sampling stopped");
    return ESP_OK;
}

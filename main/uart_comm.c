#include "uart_comm.h"
#include "pin_config.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string.h>

static const char *TAG = "UART_COMM";

#define FRAME_SOF1  0xAA
#define FRAME_SOF2  0x55

#define EVT_ENCODER  0x01
#define EVT_BUTTON   0x02
#define EVT_MODE     0x03
#define EVT_STATUS   0x04
#define EVT_HEARTBEAT 0x05

/* ─── 内部函数: 组帧并发送 ─── */
static void send_frame(uint8_t type, const uint8_t *data, uint8_t len)
{
    uint8_t buf[16];
    uint8_t idx = 0;

    buf[idx++] = FRAME_SOF1;
    buf[idx++] = FRAME_SOF2;
    buf[idx++] = type;
    buf[idx++] = len;

    uint8_t csum = type ^ len;
    for (uint8_t i = 0; i < len; i++) {
        buf[idx++] = data[i];
        csum ^= data[i];
    }
    buf[idx++] = csum;

    uart_write_bytes(UART_HOST_PORT, (const char *)buf, idx);
}

/* ─── 公开接口 ─── */

void uart_comm_init(void)
{
    uart_config_t cfg = {
        .baud_rate  = UART_HOST_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_HOST_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_HOST_PORT, &cfg);
    uart_set_pin(UART_HOST_PORT,
                 UART_HOST_TX,
                 UART_HOST_RX,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "UART%d @ %dbps TX=%d RX=%d",
             UART_HOST_PORT, UART_HOST_BAUD, UART_HOST_TX, UART_HOST_RX);
}

void uart_send_encoder(encoder_id_t id, int8_t direction)
{
    uint8_t data[2] = {(uint8_t)id, (uint8_t)direction};
    send_frame(EVT_ENCODER, data, 2);
}

void uart_send_button(button_id_t id, button_state_t state)
{
    uint8_t data[2] = {(uint8_t)id, (uint8_t)state};
    send_frame(EVT_BUTTON, data, 2);
}

void uart_send_mode(system_mode_t mode)
{
    uint8_t data[1] = {(uint8_t)mode};
    send_frame(EVT_MODE, data, 1);
}

void uart_send_status(void)
{
    xSemaphoreTake(g_state.mutex, portMAX_DELAY);
    uint8_t data[7] = {
        (uint8_t)g_state.mode,
        g_state.ch[0].timebase_idx,
        g_state.ch[0].voltscale_idx,
        g_state.ch[0].visible,
        g_state.ch[1].timebase_idx,
        g_state.ch[1].voltscale_idx,
        g_state.ch[1].visible,
    };
    xSemaphoreGive(g_state.mutex);
    send_frame(EVT_STATUS, data, sizeof(data));
}

void uart_send_heartbeat(void)
{
    send_frame(EVT_HEARTBEAT, NULL, 0);
}

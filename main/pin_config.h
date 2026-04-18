#pragma once
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

/* ─── 旋钮编码器 GPIO (EC11, 每个两路正交脉冲) ─── */
#define ENC1_PIN_A   GPIO_NUM_1    // CH1 横坐标 (时基)
#define ENC1_PIN_B   GPIO_NUM_2
#define ENC2_PIN_A   GPIO_NUM_3    // CH1 纵坐标 (电压档)
#define ENC2_PIN_B   GPIO_NUM_4
#define ENC3_PIN_A   GPIO_NUM_5    // CH2 横坐标 (时基)
#define ENC3_PIN_B   GPIO_NUM_6
#define ENC4_PIN_A   GPIO_NUM_7    // CH2 纵坐标 (电压档)
#define ENC4_PIN_B   GPIO_NUM_8

/* ─── 按键 GPIO (低有效, 内部上拉) ─── */
#define BTN_RUN_PIN     GPIO_NUM_9
#define BTN_SCOPE_PIN   GPIO_NUM_10
#define BTN_TDR_PIN     GPIO_NUM_11
#define BTN_RESET_PIN   GPIO_NUM_12

/* ─── 指示灯 GPIO (高有效) ─── */
#define LED_RUN_PIN     GPIO_NUM_13
#define LED_SCOPE_PIN   GPIO_NUM_14
#define LED_TDR_PIN     GPIO_NUM_15

/* ─── 板载 WS2812B RGB LED ─── */
#define WS2812_PIN      GPIO_NUM_48

/* ─── SPI LCD (ST7789, 240x240) ─── */
/* GPIO35/36/37 在 N16R8 上被 Octal PSRAM 占用，改用 GPIO41/42/47 */
#define LCD_SPI_HOST    SPI2_HOST
#define LCD_PIN_MOSI    GPIO_NUM_41
#define LCD_PIN_SCLK    GPIO_NUM_42
#define LCD_PIN_CS      GPIO_NUM_47
#define LCD_PIN_DC      GPIO_NUM_38
#define LCD_PIN_RST     GPIO_NUM_39
#define LCD_PIN_BL      GPIO_NUM_40
#define LCD_SPI_FREQ_HZ (40 * 1000 * 1000)

/* ─── UART 与上位机通信 ─── */
#define UART_HOST_PORT  UART_NUM_1
#define UART_HOST_TX    GPIO_NUM_17
#define UART_HOST_RX    GPIO_NUM_18
#define UART_HOST_BAUD  115200
#define UART_BUF_SIZE   256

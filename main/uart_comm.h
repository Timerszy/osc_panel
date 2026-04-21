#pragma once
#include "panel_state.h"

/**
 * 通信帧格式:
 *   [0xAA][0x55][TYPE(1)][LEN(1)][DATA(LEN)][CHECKSUM(1)]
 *   CHECKSUM = XOR(TYPE ^ LEN ^ DATA[0..LEN-1])
 *
 * 事件类型:
 *   0x01 ENCODER_EVENT: DATA=[enc_id(1)][dir(1): 0x01=CW, 0xFF=CCW]
 *                       enc_id: 0=CH1-H 1=CH1-V 2=CH2-H 3=CH2-V
 *   0x02 BUTTON_EVENT:  DATA=[btn_id(1)][state(1): 0x01=按下, 0x00=释放]
 *                       btn_id: 0=RUN 1=CH1 2=TDR 3=CH2
 *   0x03 MODE_CHANGE:   DATA=[mode(1)]
 *                       mode: 0=INTERNAL 1=RUN(ADC) 2=TDR
 *   0x04 STATUS_REPORT: DATA=[mode(1)]
 *                            [ch1_h(1)][ch1_v(1)][ch1_vis(1)]
 *                            [ch2_h(1)][ch2_v(1)][ch2_vis(1)]
 *   0x05 HEARTBEAT:     DATA=[] (空帧, 每秒发一次)
 */

/** @brief 初始化 UART 通信 */
void uart_comm_init(void);

/** @brief 发送编码器事件 */
void uart_send_encoder(encoder_id_t id, int8_t direction);

/** @brief 发送按键事件 */
void uart_send_button(button_id_t id, button_state_t state);

/** @brief 发送模式切换 */
void uart_send_mode(system_mode_t mode);

/** @brief 发送完整状态报告 */
void uart_send_status(void);

/** @brief 发送心跳包 */
void uart_send_heartbeat(void);

#pragma once
#include "panel_state.h"

/** @brief 初始化四路指示灯 (LED_RUN / LED_TDR / LED_CH1 / LED_CH2) */
void led_init(void);

/**
 * @brief 根据系统状态一次性刷新所有指示灯
 *        LED_RUN : 仅 MODE_RUN 时亮
 *        LED_TDR : 仅 MODE_TDR 时亮
 *        LED_CH1 : ch[0].visible 时亮
 *        LED_CH2 : ch[1].visible 时亮
 */
void led_apply(const system_state_t *st);

/** @brief 直接控制单个按键对应的 LED (BTN_RUN/BTN_CH1/BTN_TDR/BTN_CH2) */
void led_set(button_id_t id, uint8_t on);

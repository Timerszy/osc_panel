#pragma once
#include "panel_state.h"

/** @brief 初始化三路状态指示灯 */
void led_init(void);

/** @brief 根据系统模式更新指示灯 (RUN/SCOPE/TDR 互斥点亮) */
void led_update(system_mode_t mode);

/** @brief 直接控制单个 LED */
void led_set(button_id_t id, uint8_t on);

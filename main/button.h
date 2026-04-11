#pragma once
#include "panel_state.h"

/**
 * @brief 初始化四路按键 (中断 + FreeRTOS 定时器消抖)
 * @note  按键有效时将 EVT_BUTTON 事件推送至 g_event_queue
 */
void button_init(void);

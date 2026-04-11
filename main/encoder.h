#pragma once
#include "panel_state.h"

/**
 * @brief 初始化四路旋钮编码器 (中断驱动, 正交解码)
 * @note  编码器变化时将 EVT_ENCODER 事件推送至 g_event_queue
 */
void encoder_init(void);

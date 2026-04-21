/**
 * @file    waveform.h
 * @brief   示波器波形显示模块
 *
 * 显示布局 (240×240):
 *   Y=  0~15  : 顶部信息栏 (模式 / 时基 / 电压档)
 *   Y= 16~239 : 波形区 (224px = 8 格 × 28px/格)
 *
 * 数据来源:
 *   WAVE_SRC_SINE  — 内置测试波形 (CH1 正弦 + CH2 三角)
 *   WAVE_SRC_ADC   — 外部 ADC 采样（通过 waveform_feed 喂入）
 *
 * 渲染根据 state->ch[i].visible 决定该通道是否绘制。
 */
#pragma once
#include <stdint.h>
#include "panel_state.h"

/* ─── 显示尺寸 ─── */
#define WAVE_INFO_H   16              /* 顶部信息栏高度 (px) */
#define WAVE_AREA_Y   WAVE_INFO_H     /* 波形区起始 Y */
#define WAVE_AREA_H   224             /* 波形区高度: 8 格 × 28px  */
#define WAVE_AREA_W   240             /* 波形区宽度: 10 格 × 24px */
#define WAVE_GRID_ROWS 8
#define WAVE_GRID_COLS 10
#define WAVE_CELL_H   (WAVE_AREA_H / WAVE_GRID_ROWS)   /* 28 px/格 */
#define WAVE_CELL_W   (WAVE_AREA_W / WAVE_GRID_COLS)   /* 24 px/格 */

/* ─── 采样点数 (= LCD 宽度, 每列一个采样) ─── */
#define WAVE_SAMPLES  240

/* ─── 数据来源 ─── */
typedef enum {
    WAVE_SRC_SINE = 0,   /**< 内置正弦波 (默认) */
    WAVE_SRC_ADC,        /**< 外部 ADC 数据      */
} wave_src_t;

/**
 * @brief 初始化波形模块（清屏、绘制初始背景）
 */
void waveform_init(void);

/**
 * @brief 设置波形数据来源
 */
void waveform_set_source(wave_src_t src);

/**
 * @brief 喂入 ADC 采样数据（在 ADC 采样完成回调中调用）
 *
 * @param ch      通道索引 0=CH1, 1=CH2
 * @param samples 12-bit ADC 原始值数组 (0~4095)
 * @param count   采样点数，超过 WAVE_SAMPLES 部分被截断
 */
void waveform_feed(uint8_t ch, const uint16_t *samples, uint16_t count);

/**
 * @brief 渲染一帧波形到 LCD（在 display_task 中每 100ms 调用一次）
 *
 * @param state 当前系统状态（时基、电压档、模式）
 */
void waveform_render(const system_state_t *state);

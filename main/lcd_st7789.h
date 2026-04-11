#pragma once
#include <stdint.h>
#include <stddef.h>

/* ─── 屏幕分辨率 ─── */
#define LCD_WIDTH   240
#define LCD_HEIGHT  240

/* ─── RGB565 常用颜色 ─── */
#define LCD_BLACK    0x0000
#define LCD_WHITE    0xFFFF
#define LCD_RED      0xF800
#define LCD_GREEN    0x07E0
#define LCD_BLUE     0x001F
#define LCD_CYAN     0x07FF
#define LCD_MAGENTA  0xF81F
#define LCD_YELLOW   0xFFE0
#define LCD_ORANGE   0xFD20
#define LCD_GRAY     0x8410
#define LCD_DKGRAY   0x4208

/** @brief 初始化 SPI 总线和 ST7789 */
void lcd_init(void);

/** @brief 填充矩形区域 */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/** @brief 填充全屏 */
void lcd_fill(uint16_t color);

/**
 * @brief 绘制单个字符 (6x8 像素, 可缩放)
 * @param x, y   左上角坐标
 * @param ch     ASCII 字符 (0x20~0x7E)
 * @param fg     前景色 (RGB565)
 * @param bg     背景色 (RGB565)
 * @param scale  缩放倍数 (1=原始6x8, 2=12x16, 3=18x24)
 */
void lcd_draw_char(uint16_t x, uint16_t y, char ch,
                   uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * @brief 绘制字符串
 * @param x, y   起始坐标
 * @param str    以 '\0' 结尾的字符串
 * @param fg     前景色
 * @param bg     背景色
 * @param scale  缩放倍数
 */
void lcd_draw_string(uint16_t x, uint16_t y, const char *str,
                     uint16_t fg, uint16_t bg, uint8_t scale);

/** @brief 绘制水平线 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

/** @brief 绘制垂直线 */
void lcd_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

/**
 * @file lcd_st7735.h
 * @brief ST7735 LCD 드라이버 헤더
 */

#ifndef __LCD_ST7735_H
#define __LCD_ST7735_H

#include <stdint.h>

/* ===== LCD 크기 (160x80 또는 160x128) ===== */
#define LCD_WIDTH   160
#define LCD_HEIGHT  80

/* ===== API ===== */
void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void LCD_WriteColorFast(uint16_t color, uint32_t count);
void LCD_WriteBuffer(uint16_t *buf, uint32_t count);

/* ===== 색상 매크로 (RGB565) ===== */
#define RGB565(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))

#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F

#endif /* __LCD_ST7735_H */

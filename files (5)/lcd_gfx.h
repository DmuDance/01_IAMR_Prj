/**
 * @file lcd_gfx.h
 * @brief LCD 그래픽 함수 헤더
 */

#ifndef __LCD_GFX_H
#define __LCD_GFX_H

#include <stdint.h>
#include "lcd_st7735.h"

/* ===== 그래픽 함수 ===== */
void LCD_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void LCD_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void LCD_RoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void LCD_ThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t t, uint16_t color);
void LCD_VLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void LCD_DrawPixel(int16_t x, int16_t y, uint16_t color);

#endif /* __LCD_GFX_H */

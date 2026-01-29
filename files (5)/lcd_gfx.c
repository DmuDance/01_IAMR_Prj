/**
 * @file lcd_gfx.c
 * @brief LCD 그래픽 함수 - 성능 최적화 버전
 * 
 * 최적화 내용:
 * 1. FillCircle: 수평선 기반 (픽셀 단위 → 라인 단위)
 * 2. RoundRect: 중복 영역 제거
 * 3. ThickLine: Bresenham 알고리즘 적용
 */

#include "drivers/lcd_gfx.h"
#include "drivers/lcd_st7735.h"

/**
 * @brief 사각형 채우기
 */
void LCD_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    /* 경계 검사 */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_WIDTH)  w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    LCD_SetWindow(x, y, x + w - 1, y + h - 1);
    LCD_WriteColorFast(color, (uint32_t)w * h);
}

/**
 * @brief 수평선 그리기 (내부용, 빠름)
 */
static inline void LCD_HLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (y < 0 || y >= LCD_HEIGHT) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (w <= 0) return;

    LCD_SetWindow(x, y, x + w - 1, y);
    LCD_WriteColorFast(color, w);
}

/**
 * @brief 원 채우기 - 수평선 기반 (10배 이상 빠름)
 */
void LCD_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 1 - r;

    /* 중앙 수평선 */
    LCD_HLine(x0 - r, y0, 2 * r + 1, color);

    while (x >= y)
    {
        y++;

        if (err < 0)
        {
            err += 2 * y + 1;
        }
        else
        {
            /* 수평선 그리기 (위/아래 대칭) */
            LCD_HLine(x0 - x, y0 + y, 2 * x + 1, color);
            LCD_HLine(x0 - x, y0 - y, 2 * x + 1, color);
            x--;
            err += 2 * (y - x + 1);
        }

        if (x >= y)
        {
            LCD_HLine(x0 - y, y0 + x, 2 * y + 1, color);
            LCD_HLine(x0 - y, y0 - x, 2 * y + 1, color);
        }
    }
}

/**
 * @brief 둥근 모서리 사각형 - 최적화
 */
void LCD_RoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 1) r = 1;

    /* 중앙 직사각형 (모서리 제외) */
    LCD_FillRect(x + r, y, w - 2 * r, h, color);
    
    /* 좌우 측면 */
    LCD_FillRect(x, y + r, r, h - 2 * r, color);
    LCD_FillRect(x + w - r, y + r, r, h - 2 * r, color);

    /* 4개 모서리 원 (1/4씩) */
    int16_t cx, cy;
    int16_t px = r, py = 0;
    int16_t err = 1 - r;

    while (px >= py)
    {
        /* 좌상단 모서리 */
        cx = x + r; cy = y + r;
        LCD_HLine(cx - px, cy - py, px, color);
        LCD_HLine(cx - py, cy - px, py, color);

        /* 우상단 모서리 */
        cx = x + w - r - 1; cy = y + r;
        LCD_HLine(cx + 1, cy - py, px, color);
        LCD_HLine(cx + 1, cy - px, py, color);

        /* 좌하단 모서리 */
        cx = x + r; cy = y + h - r - 1;
        LCD_HLine(cx - px, cy + py, px, color);
        LCD_HLine(cx - py, cy + px, py, color);

        /* 우하단 모서리 */
        cx = x + w - r - 1; cy = y + h - r - 1;
        LCD_HLine(cx + 1, cy + py, px, color);
        LCD_HLine(cx + 1, cy + px, py, color);

        py++;
        if (err < 0)
        {
            err += 2 * py + 1;
        }
        else
        {
            px--;
            err += 2 * (py - px + 1);
        }
    }
}

/**
 * @brief 두꺼운 선 - Bresenham 알고리즘
 */
void LCD_ThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t t, uint16_t color)
{
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t r = t / 2;

    while (1)
    {
        /* 현재 위치에 원 또는 사각형 */
        if (t <= 2)
            LCD_FillRect(x0 - r, y0 - r, t, t, color);
        else
            LCD_FillCircle(x0, y0, r, color);

        if (x0 == x1 && y0 == y1) break;

        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

/**
 * @brief 수직선 그리기
 */
void LCD_VLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if (x < 0 || x >= LCD_WIDTH) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    if (h <= 0) return;

    LCD_SetWindow(x, y, x, y + h - 1);
    LCD_WriteColorFast(color, h);
}

/**
 * @brief 픽셀 하나 찍기
 */
void LCD_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return;
    LCD_SetWindow(x, y, x, y);
    LCD_WriteColorFast(color, 1);
}

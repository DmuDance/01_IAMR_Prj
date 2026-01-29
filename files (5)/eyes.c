/**
 * @file eyes.c
 * @brief 눈 표정 드라이버 - 성능 최적화 버전
 * 
 * 최적화 내용:
 * 1. dirty flag로 변경 시에만 그리기
 * 2. 중복 호출 방지
 * 3. 영역 클리어 최적화
 */

#include "drivers/eyes.h"
#include "drivers/lcd_st7735.h"
#include "drivers/lcd_gfx.h"
#include "main.h"

/* ===== 색상 정의 ===== */
#define BLACK       0x0000
#define EYE_COLOR   0x07E0  // 녹색

/* ===== 눈 위치 ===== */
#define LX  40      // 왼쪽 눈 X
#define RX  120     // 오른쪽 눈 X
#define CY  40      // 눈 Y (중앙)

/* ===== 눈 영역 크기 (클리어용) ===== */
#define EYE_W   50
#define EYE_H   60
#define EYE_Y   (CY - 30)

/* ===== 상태 관리 ===== */
static Expression_t current_expr = EXPR_NEUTRAL;
static uint8_t dirty = 1;  // 처음엔 그려야 함

/* ===== 내부 함수 ===== */

static void Eye_Normal(int16_t cx)
{
    LCD_RoundRect(cx - 15, CY - 25, 30, 50, 10, EYE_COLOR);
}

static void Eye_Closed(int16_t cx)
{
    LCD_FillRect(cx - 15, CY - 3, 30, 6, EYE_COLOR);
}

static void Eye_Happy(int16_t cx)
{
    /* 반원 형태 (웃는 눈) */
    LCD_RoundRect(cx - 15, CY - 5, 30, 25, 12, EYE_COLOR);
}

static void Eye_Angry(int16_t cx, int8_t dir)
{
    /* dir: -1 = 왼쪽 눈, +1 = 오른쪽 눈 */
    int16_t x0 = cx - 15;
    int16_t x1 = cx + 15;
    int16_t y0 = CY - 20 + (dir < 0 ? 15 : 0);
    int16_t y1 = CY - 20 + (dir < 0 ? 0 : 15);
    
    LCD_ThickLine(x0, y0, x1, y1, 4, EYE_COLOR);
}

static void Eye_Sad(int16_t cx, int8_t dir)
{
    /* 처진 눈 (슬픈 표정) */
    int16_t x0 = cx - 12;
    int16_t x1 = cx + 12;
    int16_t y0 = CY - 10 + (dir < 0 ? 0 : 8);
    int16_t y1 = CY - 10 + (dir < 0 ? 8 : 0);
    
    LCD_ThickLine(x0, y0, x1, y1, 3, EYE_COLOR);
    LCD_FillRect(cx - 10, CY, 20, 4, EYE_COLOR);
}

static void Eye_LookLeft(int16_t cx)
{
    /* 왼쪽을 보는 눈 (동공 위치 이동) */
    LCD_RoundRect(cx - 15, CY - 25, 30, 50, 10, EYE_COLOR);
    LCD_FillRect(cx - 12, CY - 10, 8, 20, BLACK);  // 왼쪽에 동공
}

static void Eye_LookRight(int16_t cx)
{
    /* 오른쪽을 보는 눈 */
    LCD_RoundRect(cx - 15, CY - 25, 30, 50, 10, EYE_COLOR);
    LCD_FillRect(cx + 4, CY - 10, 8, 20, BLACK);  // 오른쪽에 동공
}

static void ClearEyeArea(void)
{
    /* 양쪽 눈 영역만 클리어 (전체 화면 X) */
    LCD_FillRect(LX - 25, EYE_Y, EYE_W, EYE_H, BLACK);
    LCD_FillRect(RX - 25, EYE_Y, EYE_W, EYE_H, BLACK);
}

/* ===== 외부 API ===== */

/**
 * @brief 표정 직접 그리기 (강제)
 */
void Eyes_Draw(Expression_t expr)
{
    ClearEyeArea();

    switch (expr)
    {
        case EXPR_NEUTRAL:
            Eye_Normal(LX);
            Eye_Normal(RX);
            break;

        case EXPR_BLINK:
            Eye_Closed(LX);
            Eye_Closed(RX);
            break;

        case EXPR_HAPPY:
            Eye_Happy(LX);
            Eye_Happy(RX);
            break;

        case EXPR_ANGRY:
            Eye_Angry(LX, -1);  // 왼쪽
            Eye_Angry(RX, +1);  // 오른쪽
            break;

        case EXPR_SLEEPY:
            Eye_Closed(LX);
            Eye_Closed(RX);
            break;

        case EXPR_SAD:
            Eye_Sad(LX, -1);
            Eye_Sad(RX, +1);
            break;

        case EXPR_LOOK_LEFT:
            Eye_LookLeft(LX);
            Eye_LookLeft(RX);
            break;

        case EXPR_LOOK_RIGHT:
            Eye_LookRight(LX);
            Eye_LookRight(RX);
            break;

        default:
            Eye_Normal(LX);
            Eye_Normal(RX);
            break;
    }

    dirty = 0;
}

/**
 * @brief 표정 설정 (변경 시에만 그리기)
 */
void Eyes_SetExpression(Expression_t expr)
{
    if (expr != current_expr)
    {
        current_expr = expr;
        dirty = 1;
    }
}

/**
 * @brief 현재 표정 반환
 */
Expression_t Eyes_GetExpression(void)
{
    return current_expr;
}

/**
 * @brief 변경 사항이 있으면 그리기
 */
void Eyes_Update(void)
{
    if (dirty)
    {
        Eyes_Draw(current_expr);
    }
}

/**
 * @brief 강제 갱신 플래그 설정
 */
void Eyes_Invalidate(void)
{
    dirty = 1;
}

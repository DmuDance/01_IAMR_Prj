/**
 * @file anim.c
 * @brief 애니메이션 드라이버 - 성능 최적화 버전
 * 
 * 최적화 내용:
 * 1. LCD_Init 중복 호출 제거
 * 2. 깜빡임 논블로킹 타이머 기반
 * 3. Eyes_Update() 통합으로 중복 드로잉 방지
 */

#include "main.h"
#include "drivers/lcd_st7735.h"
#include "drivers/eyes.h"
#include "drivers/anim.h"

/* ===== 깜빡임 설정 ===== */
#define BLINK_INTERVAL_MS   3000    // 깜빡임 주기 (3초)
#define BLINK_DURATION_MS   150     // 눈 감고 있는 시간

/* ===== 상태 변수 ===== */
static uint32_t last_blink_time = 0;
static uint8_t  is_blinking = 0;
static Expression_t saved_expr = EXPR_NEUTRAL;

/**
 * @brief 애니메이션 초기화
 * @note LCD_Init()은 main에서 한 번만 호출할 것
 */
void Anim_Init(void)
{
    /* LCD_Init()은 main.c에서 호출하므로 여기서 제외 */
    /* LCD_Clear()도 main.c에서 호출 */
    
    Eyes_SetExpression(EXPR_NEUTRAL);
    Eyes_Update();
    
    last_blink_time = HAL_GetTick();
}

/**
 * @brief 표정 설정
 */
void Anim_Set(Expression_t expr)
{
    Eyes_SetExpression(expr);
}

/**
 * @brief 깜빡임 업데이트 (논블로킹)
 */
void Blink_Update(void)
{
    uint32_t now = HAL_GetTick();
    Expression_t current = Eyes_GetExpression();

    /* 깜빡임 중이 아닐 때 */
    if (!is_blinking)
    {
        /* 깜빡임 주기 도달 & 이미 눈 감은 상태가 아닐 때 */
        if ((now - last_blink_time >= BLINK_INTERVAL_MS) &&
            (current != EXPR_BLINK) &&
            (current != EXPR_SLEEPY))
        {
            saved_expr = current;
            Eyes_SetExpression(EXPR_BLINK);
            Eyes_Update();
            is_blinking = 1;
            last_blink_time = now;
        }
    }
    /* 깜빡임 중일 때 */
    else
    {
        if (now - last_blink_time >= BLINK_DURATION_MS)
        {
            Eyes_SetExpression(saved_expr);
            Eyes_Update();
            is_blinking = 0;
            last_blink_time = now;
        }
    }
}

/**
 * @brief 애니메이션 업데이트 (메인 루프에서 호출)
 */
void Anim_Update(void)
{
    /* 깜빡임 처리 */
    Blink_Update();
    
    /* 표정 변경 시에만 그리기 */
    Eyes_Update();
}

/**
 * @file eyes.h
 * @brief 눈 표정 드라이버 헤더
 */

#ifndef __EYES_H
#define __EYES_H

#include <stdint.h>

/* ===== 표정 열거형 ===== */
typedef enum {
    EXPR_NEUTRAL = 0,   // 기본
    EXPR_BLINK,         // 깜빡임
    EXPR_HAPPY,         // 행복
    EXPR_ANGRY,         // 화남
    EXPR_SLEEPY,        // 졸림
    EXPR_SAD,           // 슬픔
    EXPR_LOOK_LEFT,     // 왼쪽 보기
    EXPR_LOOK_RIGHT,    // 오른쪽 보기
} Expression_t;

/* ===== API ===== */
void Eyes_Draw(Expression_t expr);          // 강제 그리기
void Eyes_SetExpression(Expression_t expr); // 표정 설정 (lazy)
Expression_t Eyes_GetExpression(void);      // 현재 표정 반환
void Eyes_Update(void);                     // dirty 시에만 그리기
void Eyes_Invalidate(void);                 // 강제 갱신 플래그

#endif /* __EYES_H */

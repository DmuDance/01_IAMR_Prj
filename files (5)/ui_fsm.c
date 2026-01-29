/**
 * @file ui_fsm.c
 * @brief UI 상태 머신 - 성능 최적화 버전
 * 
 * 최적화 내용:
 * 1. Eyes_Draw() 직접 호출 → Eyes_SetExpression() 사용
 * 2. 상태 변경 시에만 표정 업데이트
 * 3. 중복 드로잉 완전 제거
 */

#include "ui_fsm.h"
#include "drivers/eyes.h"
#include "robot_state.h"

static UI_State_t ui_state = UI_BOOT;
static RobotState_t prev_robot_state = STATE_IDLE;

void UI_Init(void)
{
    ui_state = UI_BOOT;
    prev_robot_state = STATE_IDLE;
}

void UI_Update(void)
{
    RobotState_t rs = RobotState_Get();

    /* 상태 변경 시에만 처리 */
    if (rs == prev_robot_state)
    {
        return;
    }
    prev_robot_state = rs;

    switch (rs)
    {
        case STATE_IDLE:
            ui_state = UI_IDLE;
            Eyes_SetExpression(EXPR_SLEEPY);
            break;

        case STATE_SCAN:
            ui_state = UI_SCAN;
            Eyes_SetExpression(EXPR_LOOK_LEFT);
            break;

        case STATE_DECIDE:
            /* DECIDE는 UI 상태 변경 없음 (SCAN 유지) */
            break;

        case STATE_MOVE:
            ui_state = UI_DRIVE;
            Eyes_SetExpression(EXPR_HAPPY);
            break;

        case STATE_ALERT:
            ui_state = UI_ALERT;
            Eyes_SetExpression(EXPR_ANGRY);
            break;

        case STATE_REVERSE:
            ui_state = UI_DRIVE;
            Eyes_SetExpression(EXPR_SAD);
            break;

        default:
            break;
    }
    
    /* 
     * 주의: Eyes_Update()는 Anim_Update()에서 호출됨
     * 여기서 Eyes_Draw()를 직접 호출하지 않음
     */
}

UI_State_t UI_GetState(void)
{
    return ui_state;
}

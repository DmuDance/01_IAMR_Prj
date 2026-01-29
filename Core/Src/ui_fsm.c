/* ui_fsm.c */
#include "ui_fsm.h"
#include "drivers/ultrasonic.h"
#include "robot_state.h"
#include "drivers/lcd_st7735.h"
#include "drivers/eyes.h"   // 🔥 추가

extern uint8_t scan_angle;
extern uint8_t start_flag;
extern uint8_t manual_mode;
extern uint8_t manual_command;

static RobotState_t prev_state = STATE_IDLE;  // 🔥 상태 기억

void UI_Init(void)
{
    LCD_Clear(COLOR_BLACK);
    Eyes_SetExpression(EXPR_SLEEPY); // 초기 표정
    prev_state = RobotState_Get();
}

void UI_Update(void)
{
    RobotState_t state = RobotState_Get();
    uint16_t distance = Ultrasonic_GetDistance();
    char line1[17];
    char line2[17];

    /* 🔥 상태 변경 시에만 얼굴 변경 */
    if (state != prev_state)
    {
        switch (state)
        {
            case STATE_IDLE:
                Eyes_SetExpression(EXPR_SLEEPY);
                break;

            case STATE_SCAN:
                Eyes_SetExpression(EXPR_LOOK_LEFT);
                break;

            case STATE_MOVE:
                Eyes_SetExpression(EXPR_HAPPY);
                break;

            case STATE_ALERT:
                Eyes_SetExpression(EXPR_ANGRY);   // 🔥 화난 표정
                break;

            case STATE_REVERSE:
                Eyes_SetExpression(EXPR_SAD);
                break;

            default:
                break;
        }
        prev_state = state;
    }

    /* ===== LCD 출력 ===== */
    if (start_flag == 1)
    {
        switch (state)
        {
            case STATE_IDLE:    snprintf(line1, sizeof(line1), "AUTO : IDLE   "); break;
            case STATE_SCAN:    snprintf(line1, sizeof(line1), "AUTO : SCAN   "); break;
            case STATE_DECIDE:  snprintf(line1, sizeof(line1), "AUTO : DECIDE "); break;
            case STATE_MOVE:    snprintf(line1, sizeof(line1), "AUTO : MOVE   "); break;
            case STATE_ALERT:   snprintf(line1, sizeof(line1), "AUTO : ALERT  "); break;
            case STATE_REVERSE: snprintf(line1, sizeof(line1), "AUTO : REVERSE"); break;
            default:            snprintf(line1, sizeof(line1), "AUTO : UNKNOWN"); break;
        }
    }
    else if (manual_mode == 1)
    {
        switch (manual_command)
        {
            case 1: snprintf(line1, sizeof(line1), "Manual : MOVE "); break;
            case 2: snprintf(line1, sizeof(line1), "Manual :REVERSE"); break;
            case 3: snprintf(line1, sizeof(line1), "Manual : LEFT "); break;
            case 4: snprintf(line1, sizeof(line1), "Manual : RIGHT"); break;
            case 5: snprintf(line1, sizeof(line1), "Manual : RESET"); break;
            default: snprintf(line1, sizeof(line1), "Manual : STOP "); break;
        }
    }
    else
    {
        snprintf(line1, sizeof(line1), "STATE: IDLE   ");
    }

    snprintf(line2, sizeof(line2),
             "D:%3dcm A:%3d%c", distance, scan_angle, 0xDF);

    LCD_XY(0, 0);
    LCD_PUTS(line1);
    LCD_XY(0, 1);
    LCD_PUTS(line2);
}

/* ui_fsm.c */
#include "ui_fsm.h"
#include "drivers/ultrasonic.h"
#include "robot_state.h"
#include "drivers/lcd_st7735.h"
//#include "drivers/eyes.h"   // 🔥 추가

#define LCD_CS_HIGH_FORCE()  (GPIOB->BSRR = GPIO_PIN_12)  // ← 이 줄 추가

extern uint8_t scan_angle;
extern uint8_t start_flag;
extern uint8_t manual_mode;
extern uint8_t manual_command;
extern uint16_t g_distance;
extern void LCD_CLEAR(void);
extern SPI_HandleTypeDef hspi2;
extern volatile uint8_t spi_busy;
extern volatile uint8_t spi_dma_busy;

static RobotState_t prev_state = STATE_IDLE;  // 🔥 상태 기억
static char prev1[17] = "";
static char prev2[17] = "";


void UI_Init(void)
{
	// prev 버퍼 강제 초기화 → 다음 Update에서 반드시 출력

	   // prev1[0] = '\0';
	    //prev2[0] = '\0';

	// HD44780 먼저
	    LCD_CLEAR();
	    HAL_Delay(5);
	    // ST7735 나중
	    //LCD_Clear(COLOR_BLACK);
	    prev_state = RobotState_Get();
}

void UI_Update(void)
{
//	static char prev1[17] = "";
//	static char prev2[17] = "";
    RobotState_t state = RobotState_Get();
    uint16_t distance = g_distance;
    char line1[17];
    char line2[17];
    /* ST7735 전송 중에는 I2C LCD 갱신을 잠시 미뤄 버스 충돌/지터 방지 */
    if (spi_busy || spi_dma_busy)
    {
        return;
    }
/*
     🔥 상태 변경 시에만 얼굴 변경
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
*/

    /* ===== LCD 출력 ===== */
    if (start_flag == 1)
    {
        switch (state)
        {
            case STATE_IDLE:    snprintf(line1, sizeof(line1), "AUTO : IDLE   "); break;
            case STATE_SCAN:    snprintf(line1, sizeof(line1), "AUTO : SCAN   "); break;
            case STATE_WAIT_ECHO: snprintf(line1, sizeof(line1), "AUTO : WAIT   "); break;
            case STATE_READ_ECHO: snprintf(line1, sizeof(line1), "AUTO : READ   "); break;
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

    if (strcmp(prev1, line1) != 0) {
       // HAL_SPI_Abort(&hspi2);          // SPI 완전 중단
        LCD_CS_HIGH_FORCE();            // CS 확실히 올림
        LCD_XY(0, 0);
        LCD_PUTS(line1);
        strcpy(prev1, line1);
        //Eyes_Invalidate();              // SPI 중단했으니 눈 다시 그리기
    }

    if (strcmp(prev2, line2) != 0) {
      //  HAL_SPI_Abort(&hspi2);
        LCD_CS_HIGH_FORCE();
        LCD_XY(0, 1);
        LCD_PUTS(line2);
        strcpy(prev2, line2);
        //Eyes_Invalidate();
    }
}

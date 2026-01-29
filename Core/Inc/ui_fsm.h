#ifndef UI_FSM_H
#define UI_FSM_H

#include "robot_state.h"
#include "drivers/eyes.h"   // ⭐ 이거 추가 (표정 정의는 여기서 가져옴)

typedef enum {
    UI_BOOT,
    UI_IDLE,
    UI_SCAN,
    UI_DRIVE,
    UI_ALERT,
    UI_ERROR
} UI_State_t;

void UI_Init(void);
void UI_Update(void);

#endif

#include "robot_state.h"

/* ★ 전역(파일 내부) 상태 변수 */
static RobotState_t currentState = STATE_IDLE;

void RobotState_Init(void)
{
    currentState = STATE_IDLE;
}

void RobotState_Set(RobotState_t state)
{
    currentState = state;
}

/* ★ 이 함수가 없어서 에러 난 거임 */
RobotState_t RobotState_Get(void)
{
    return currentState;
}

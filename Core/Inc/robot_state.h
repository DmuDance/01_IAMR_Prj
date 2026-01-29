
#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

typedef enum
{
    STATE_IDLE,
    STATE_SCAN,
    STATE_DECIDE,
    STATE_MOVE,
    STATE_REVERSE,
    STATE_ALERT
} RobotState_t;

void Handle_State(RobotState_t state);  // ★ 이 줄 필수
void RobotState_Init(void);
void RobotState_Set(RobotState_t state);
RobotState_t RobotState_Get(void);
#endif

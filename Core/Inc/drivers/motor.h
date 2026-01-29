#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f1xx_hal.h"

/* 방향 정의 */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD
} MotorDir_t;

/* 기본 제어 */
void Motor_Init(void);
void Motor_Stop(void);
void Motor_Forward(void);
void Motor_Backward(void);
void Motor_Left(void);
void Motor_Right(void);

/* 회전 */
void Motor_TurnLeft_Front(void);
void Motor_TurnRight_Front(void);
void Motor_TurnLeft_Back(void);
void Motor_TurnRight_Back(void);

#endif

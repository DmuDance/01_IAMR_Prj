#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f1xx_hal.h"

void Servo_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void Servo_SetAngle(uint8_t angle);   // 0~180

#endif

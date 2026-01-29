#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "stm32f1xx_hal.h"

/* 초기화 (타이머 핸들 전달) */
void Ultrasonic_Init(TIM_HandleTypeDef *htim);

/* 거리 반환 (cm) */
uint16_t Ultrasonic_GetDistance(void);

#endif

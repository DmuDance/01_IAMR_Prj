#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "stm32f1xx_hal.h"   // TIM_HandleTypeDef 때문에 필요
#include <stdint.h>

void Ultrasonic_Init(void);
void Ultrasonic_Trigger(void);
uint16_t Ultrasonic_Read(void);
void Ultrasonic_IC_Callback(TIM_HandleTypeDef *htim);

#endif

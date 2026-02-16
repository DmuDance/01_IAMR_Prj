#include "drivers/servo.h"

/* ===== PWM 파라미터 ===== */
#define SERVO_MIN 500     // 0.5ms
#define SERVO_MAX 2500    // 2.5ms
#define SERVO_CENTER 1500 // 1.5ms

static TIM_HandleTypeDef *servo_tim;
static uint32_t servo_channel;

void Servo_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    servo_tim = htim;
    servo_channel = channel;

    HAL_TIM_PWM_Start(servo_tim, servo_channel);
    __HAL_TIM_SET_COMPARE(servo_tim, servo_channel, SERVO_CENTER);
}

void Servo_SetAngle(uint8_t angle)
{
    if (angle > 180) angle = 180;

    uint16_t pulse = SERVO_MIN +
        (angle * (SERVO_MAX - SERVO_MIN)) / 180;

    __HAL_TIM_SET_COMPARE(servo_tim, servo_channel, pulse);
}

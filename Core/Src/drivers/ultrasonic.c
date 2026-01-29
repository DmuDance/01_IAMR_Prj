#include "drivers/ultrasonic.h"

/* ===== 핀맵 (robot_config.h로 나중에 이동 가능) ===== */
#define ULTRASONIC_TRIG_PORT   GPIOA
#define ULTRASONIC_TRIG_PIN    GPIO_PIN_0

#define ULTRASONIC_ECHO_PORT   GPIOA
#define ULTRASONIC_ECHO_PIN    GPIO_PIN_1

/* ===== 내부 변수 ===== */
static TIM_HandleTypeDef *us_tim;

/* ===== 내부 함수 ===== */
static void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(us_tim, 0);
    while (__HAL_TIM_GET_COUNTER(us_tim) < us);
}

static void trig_pulse(void)
{
    HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_RESET);
}

static uint32_t echo_time_us(void)
{
    uint32_t timeout = 30000; // 30ms
    uint32_t start = __HAL_TIM_GET_COUNTER(us_tim);

    // ECHO가 HIGH 될 때까지 대기 (타임아웃 포함)
    while (HAL_GPIO_ReadPin(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN) == GPIO_PIN_RESET)
    {
        if ((__HAL_TIM_GET_COUNTER(us_tim) - start) > timeout)
            return 0;
    }

    __HAL_TIM_SET_COUNTER(us_tim, 0);

    // ECHO가 LOW로 떨어질 때까지 대기 (타임아웃 포함)
    while (HAL_GPIO_ReadPin(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN) == GPIO_PIN_SET)
    {
        if (__HAL_TIM_GET_COUNTER(us_tim) > timeout)
            return 0;
    }

    return __HAL_TIM_GET_COUNTER(us_tim);
}

/* ===== 외부 함수 ===== */

void Ultrasonic_Init(TIM_HandleTypeDef *htim)
{
    us_tim = htim;
    HAL_TIM_Base_Start(us_tim);
}

uint16_t Ultrasonic_GetDistance(void)
{
    uint32_t echo_us;

    trig_pulse();
    echo_us = echo_time_us();

    if (echo_us < 240 || echo_us > 23000)
        return 0;

    /* cm 단위 변환 */
    return (uint16_t)(echo_us * 0.017);
}

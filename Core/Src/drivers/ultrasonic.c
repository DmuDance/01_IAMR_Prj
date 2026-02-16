#include "drivers/ultrasonic.h"

#define ULTRASONIC_TRIG_PORT GPIOA
#define ULTRASONIC_TRIG_PIN  GPIO_PIN_0

extern TIM_HandleTypeDef htim2;

static volatile uint32_t ic_val1 = 0;
static volatile uint32_t ic_val2 = 0;
static volatile uint8_t  ic_state = 0;   // 0=RISING 대기, 1=FALLING 대기
static volatile uint16_t distance_cm = 0;
static volatile uint8_t  distance_ready = 0;

/* 트리거 펄스 */
static void trig_pulse(void)
{
    HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_SET);
    for(volatile int i=0;i<200;i++);
    HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_RESET);
}

void Ultrasonic_Init(void)
{
	HAL_TIM_Base_Start(&htim2);   // 타이머 카운터 시작
	    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
	    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
}

void Ultrasonic_Trigger(void)
{
    distance_ready = 0;
    trig_pulse();
}

uint16_t Ultrasonic_Read(void)
{
    if(distance_ready)
    {
        distance_ready = 0;
        return distance_cm;
    }
    return 999;
}
void Ultrasonic_IC_Callback(TIM_HandleTypeDef *htim)
{
    if(ic_state == 0)  // RISING 감지
    {
        ic_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        ic_state = 1;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else  // FALLING 감지 → 펄스폭 계산
    {
        ic_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

        uint32_t diff;
        if(ic_val2 >= ic_val1)
            diff = ic_val2 - ic_val1;
        else
            diff = (0xFFFF - ic_val1 + ic_val2);

        // 노이즈 제거 후 즉시 플래그 설정 (printf 제거!)
        if(diff > 100 && diff < 30000)
        {
            distance_cm = diff / 58;
            distance_ready = 1;  // ← 여기가 핵심!
        }

        ic_state = 0;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
    }
}
/* 인터럽트 콜백 */
/*void Ultrasonic_IC_Callback(TIM_HandleTypeDef *htim)
{

    if(ic_state == 0)  // RISING 잡음
    {
        ic_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        ic_state = 1;

        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else  // FALLING 잡음 → 펄스폭 계산
    {
        ic_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

        uint32_t diff;
        if(ic_val2 >= ic_val1)
            diff = ic_val2 - ic_val1;
        else
            diff = (0xFFFF - ic_val1 + ic_val2);
        printf("ic_val1=%lu, ic_val2=%lu, diff=%lu\n", ic_val1, ic_val2, diff);
        if(diff > 100 && diff < 30000)  // 노이즈 제거
        {
            distance_cm = diff / 58;
            distance_ready = 1;
        }

        ic_state = 0;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
    }

}*/

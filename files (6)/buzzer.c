/**
 * @file buzzer.c
 * @brief 논블로킹 부저 드라이버
 * 
 * HAL_Delay() 없이 상태 머신으로 멜로디 재생
 * main loop에서 Buzzer_Update()를 호출해야 함
 */

#include "drivers/buzzer.h"
#include "robot_config.h"
#include "main.h"

/* ===== 멜로디 데이터 ===== */

// 엘리제를 위하여
static const BuzzerNote_t melody_elise[] = {
    {120, 80}, {120, 80}, {120, 80}, {120, 200},  // 따라라~
    {120, 80}, {120, 80}, {120, 200},              // 따라~
    {120, 80}, {120, 80}, {120, 80}, {120, 200},  // 따라라~
    {120, 80}, {120, 80}, {120, 200},              // 따라~
    {120, 80}, {120, 80}, {120, 80}, {120, 200},  // 따라라~
    {120, 80}, {120, 80}, {120, 200},              // 따라~
    {200, 300},                                     // 마무리
};
#define ELISE_LENGTH  (sizeof(melody_elise) / sizeof(melody_elise[0]))

// 경고음 (짧은 비프 2회)
static const BuzzerNote_t melody_alert[] = {
    {100, 100},
    {100, 100},
};
#define ALERT_LENGTH  (sizeof(melody_alert) / sizeof(melody_alert[0]))

/* ===== 상태 변수 ===== */
static const BuzzerNote_t *current_melody = NULL;
static uint8_t melody_length = 0;
static uint8_t melody_index = 0;
static uint8_t buzzer_phase = 0;    // 0=idle, 1=on, 2=off
static uint32_t buzzer_tick = 0;

/* ===== 기본 GPIO 제어 ===== */

void Buzzer_On(void)
{
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

/* ===== 멜로디 재생 API ===== */

void Buzzer_PlayMelody(const BuzzerNote_t *melody, uint8_t length)
{
    if (melody == NULL || length == 0)
        return;

    current_melody = melody;
    melody_length = length;
    melody_index = 0;
    buzzer_phase = 1;  // ON 상태로 시작
    buzzer_tick = HAL_GetTick();
    Buzzer_On();
}

void Buzzer_PlayElise(void)
{
    Buzzer_PlayMelody(melody_elise, ELISE_LENGTH);
}

void Buzzer_PlayAlert(void)
{
    Buzzer_PlayMelody(melody_alert, ALERT_LENGTH);
}

void Buzzer_Stop(void)
{
    Buzzer_Off();
    current_melody = NULL;
    melody_length = 0;
    melody_index = 0;
    buzzer_phase = 0;
}

uint8_t Buzzer_IsPlaying(void)
{
    return (buzzer_phase != 0);
}

/**
 * @brief 부저 상태 업데이트 (main loop에서 매번 호출)
 * 
 * 상태 머신:
 *   phase 0: 대기 (재생 안 함)
 *   phase 1: ON 상태 (on_ms 대기 중)
 *   phase 2: OFF 상태 (off_ms 대기 중)
 */
void Buzzer_Update(void)
{
    // 재생 중이 아니면 리턴
    if (buzzer_phase == 0 || current_melody == NULL)
        return;

    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - buzzer_tick;

    if (buzzer_phase == 1)  // ON 상태
    {
        // on_ms 시간이 지났으면 OFF로 전환
        if (elapsed >= current_melody[melody_index].on_ms)
        {
            Buzzer_Off();
            buzzer_phase = 2;
            buzzer_tick = now;
        }
    }
    else if (buzzer_phase == 2)  // OFF 상태
    {
        // off_ms 시간이 지났으면 다음 음표로
        if (elapsed >= current_melody[melody_index].off_ms)
        {
            melody_index++;

            if (melody_index >= melody_length)
            {
                // 멜로디 끝
                Buzzer_Stop();
            }
            else
            {
                // 다음 음표 시작
                Buzzer_On();
                buzzer_phase = 1;
                buzzer_tick = now;
            }
        }
    }
}

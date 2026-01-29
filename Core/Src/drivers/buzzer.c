/**
 * @file buzzer.c
 * @brief 논블로킹 부저 드라이버 - 마리오 음악
 */

#include "drivers/buzzer.h"
#include "robot_config.h"
#include "main.h"

static const BuzzerNote_t melody_elise[] = {
    {0, 0}
};
#define ELISE_LENGTH  (sizeof(melody_elise) / sizeof(melody_elise[0]))

static const BuzzerNote_t mario_short[] = {
    {150, 100},   /* 도 */
    {150, 100},   /* 도 */
    {150, 100},   /* 도 */
    {0, 50},      /* 쉼표 */
    {100, 100},   /* 솔 */
    {150, 100},   /* 도 */
    {0, 50},      /* 쉼표 */
    {50, 200},    /* 미 (길게) */
    {0, 100},     /* 쉼표 */
    {0, 100},     /* 쉼표 */

    {42, 100},    /* 라 */
    {0, 50},      /* 쉼표 */
    {42, 100},    /* 라 */
    {0, 50},      /* 쉼표 */
    {42, 100},    /* 라 */
    {0, 50},      /* 쉼표 */
    {42, 100},    /* 라 */
    {0, 50},      /* 쉼표 */
    {47, 100},    /* 시 */
    {0, 50},      /* 쉼표 */

    {50, 100},    /* 도 */
    {0, 50},      /* 쉼표 */
    {47, 100},    /* 시 */
    {0, 50},      /* 쉼표 */
    {45, 100},    /* 라 */
    {0, 50},      /* 쉼표 */
    {42, 150},    /* 솔 (길게) */
    {0, 100},     /* 쉼표 */

    {0, 0}        /* 종료 */
};
#define MARIO_LENGTH  (sizeof(mario_short) / sizeof(mario_short[0]))

static const BuzzerNote_t melody_alert[] = {
    {150, 100},
    {0, 100},
    {150, 100},
    {0, 0}
};
#define ALERT_LENGTH  (sizeof(melody_alert) / sizeof(melody_alert[0]))

static const BuzzerNote_t *current_melody = NULL;
static uint8_t melody_length = 0;
static uint8_t melody_index = 0;
static uint8_t buzzer_phase = 0;
static uint32_t buzzer_tick = 0;

void Buzzer_On(void)
{
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

void Buzzer_PlayMelody(const BuzzerNote_t *melody, uint8_t length)
{
    if (melody == NULL || length == 0)
        return;

    current_melody = melody;
    melody_length = length;
    melody_index = 0;
    buzzer_phase = 1;
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

void Buzzer_PlayMario(void)
{
    Buzzer_PlayMelody(mario_short, MARIO_LENGTH);
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

void Buzzer_Update(void)
{
    if (buzzer_phase == 0 || current_melody == NULL)
        return;

    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - buzzer_tick;

    if (buzzer_phase == 1)
    {
        if (elapsed >= current_melody[melody_index].on_ms)
        {
            Buzzer_Off();
            buzzer_phase = 2;
            buzzer_tick = now;
        }
    }
    else if (buzzer_phase == 2)
    {
        if (elapsed >= current_melody[melody_index].off_ms)
        {
            melody_index++;

            if (melody_index >= melody_length ||
                current_melody[melody_index].on_ms == 0)
            {
                Buzzer_Stop();
            }
            else
            {
                Buzzer_On();
                buzzer_phase = 1;
                buzzer_tick = now;
            }
        }
    }
}

/**
 * @file buzzer.c
 * @brief PWM 부저 드라이버 - TIM3 기반 논블로킹
 */

#include "drivers/buzzer.h"
#include "main.h"

/* ===== 내부 변수 ===== */
static TIM_HandleTypeDef *buzzer_tim = NULL;
static uint32_t buzzer_channel = 0;

static const BuzzerNote_t *current_melody = NULL;
static uint16_t melody_length = 0;
static uint16_t melody_index = 0;
static uint8_t buzzer_state = 0;  // 0: 정지, 1: 재생, 2: 쉼표
static uint32_t buzzer_tick = 0;

/* ===== 마리오 테마 ===== */
static const BuzzerNote_t mario_theme[] = {
    // 첫 번째 구간
    {NOTE_E7, EIGHTH}, {NOTE_E7, EIGHTH}, {REST, EIGHTH}, {NOTE_E7, EIGHTH},
    {REST, EIGHTH}, {NOTE_C7, EIGHTH}, {NOTE_E7, EIGHTH}, {REST, EIGHTH},
    {NOTE_G7, QUARTER}, {REST, QUARTER}, {NOTE_G6, QUARTER}, {REST, QUARTER},

    // 두 번째 구간
    {NOTE_C7, QUARTER}, {REST, EIGHTH}, {NOTE_G6, EIGHTH}, {REST, EIGHTH},
    {NOTE_E6, QUARTER}, {REST, EIGHTH}, {NOTE_A6, EIGHTH}, {REST, EIGHTH},
    {NOTE_B6, EIGHTH}, {REST, EIGHTH}, {NOTE_B6, EIGHTH}, {NOTE_A6, QUARTER},

    // 세 번째 구간
    {NOTE_G6, EIGHTH}, {NOTE_E7, EIGHTH}, {NOTE_G7, EIGHTH}, {NOTE_A7, QUARTER},
    {NOTE_F7, EIGHTH}, {NOTE_G7, EIGHTH}, {REST, EIGHTH}, {NOTE_E7, EIGHTH},
    {REST, EIGHTH}, {NOTE_C7, EIGHTH}, {NOTE_D7, EIGHTH}, {NOTE_B6, QUARTER}
};

/* ===== 경고음 ===== */
static const BuzzerNote_t alert_melody[] = {
    {NOTE_C7, 150},
    {REST, 100},
    {NOTE_C7, 150},
    {REST, 100},
    {NOTE_C7, 150},
    {REST, 0}  // 종료
};

/* ===== 멈춤음 ===== */
static const BuzzerNote_t stop_melody[] = {
{NOTE_E5, 180},
{NOTE_C5, 220},
{NOTE_G4, 260},
{REST, 80},
{NOTE_C4, 400}, // 낮게 길게 = 완전 정지 느낌
{REST, 0}
};

/* ===== 오토소리 ===== */
static const BuzzerNote_t start_melody[] = {
    {NOTE_G5, 120},
    {NOTE_C6, 120},
    {NOTE_E6, 140},   // 점점 밝아짐
    {REST,    60},

    {NOTE_E6, 120},
    {NOTE_G6, 160},   // 기대감 상승
    {REST,    60},

    {NOTE_C7, 260},   // 출발 확정! 밝게 마무리
    {REST, 0}
};

/* ===== 리셋음 ===== */
static const BuzzerNote_t reset_melody[] = {
{NOTE_C5, 120},
{NOTE_E5, 120},
{NOTE_G5, 150},
{REST, 80},
{NOTE_C6, 250}, // 위로 마무리 = 리셋 완료 느낌
{REST, 0}
};

/* ===== 엘리제를 위하여 (짧은 버전) ===== */
static const BuzzerNote_t elise_melody[] = {
    {NOTE_E5, EIGHTH}, {NOTE_D5, EIGHTH},
    {NOTE_E5, EIGHTH}, {NOTE_D5, EIGHTH},
    {NOTE_E5, EIGHTH}, {NOTE_B4, EIGHTH},
    {NOTE_D5, EIGHTH}, {NOTE_C5, EIGHTH},
    {NOTE_A4, QUARTER},
    {REST, 0}
};

/* ===== 확인 소리 (OK!) ===== */
static const BuzzerNote_t ok_melody[] = {
    {NOTE_C6, 80},
    {NOTE_E6, 80},
    {NOTE_G6, 120},
    {NOTE_C7, 200},
    {REST, 0}  // 종료
};

/* ===== 랜덤 소리 (OK!) ===== */
static const BuzzerNote_t idle_ping_melody[] = {
    {NOTE_E6, 120},   // 주인님
    {NOTE_G6, 120},
    {NOTE_E6, 120},   // 뭐하세요
    {NOTE_C6, 150},
    {REST, 100},      // 살짝 쉬고
    {NOTE_D6, 120},   // 저
    {NOTE_E6, 120},   // 여기
    {NOTE_G6, 150},   // 있어요~
    {NOTE_C7, 250},   // (강조)
    {REST, 0}         // 종료
};


/* ===== 초기화 ===== */
void Buzzer_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    buzzer_tim = htim;
    buzzer_channel = channel;

    // PWM 시작 (초기 상태는 무음)
    HAL_TIM_PWM_Start(buzzer_tim, buzzer_channel);
    Buzzer_Stop();
}

/* ===== PWM 주파수 설정 ===== */
static void Buzzer_SetFrequency(uint16_t freq)
{
    if (freq == 0 || buzzer_tim == NULL) {
        // 무음 - PWM 정지
        HAL_TIM_PWM_Stop(buzzer_tim, buzzer_channel);
        return;
    }

    // 주파수 → ARR 계산
    // TIM3: Prescaler = 1279 → 1MHz (1μs 단위)
    // ARR = 1000000 / frequency - 1
    uint32_t arr_value = 1000000 / freq - 1;

    // ARR 업데이트
    __HAL_TIM_SET_AUTORELOAD(buzzer_tim, arr_value);

    // 듀티 50% (음질 향상)
    __HAL_TIM_SET_COMPARE(buzzer_tim, buzzer_channel, arr_value / 2);

    // PWM 시작
    HAL_TIM_PWM_Start(buzzer_tim, buzzer_channel);
}

/* ===== 톤 재생 (블로킹) ===== */
void Buzzer_PlayTone(uint16_t freq, uint16_t duration)
{
    Buzzer_SetFrequency(freq);
    HAL_Delay(duration);
    Buzzer_Stop();
    HAL_Delay(30);  // 음표 간 간격
}



/* ===== 멜로디 재생 (논블로킹 시작) ===== */
void Buzzer_PlayMelody(const BuzzerNote_t *melody, uint16_t length)
{
    if (melody == NULL || length == 0)
        return;

    current_melody = melody;
    melody_length = length;
    melody_index = 0;
    buzzer_state = 1;  // 재생 상태
    buzzer_tick = HAL_GetTick();

    // 첫 음표 시작
    Buzzer_SetFrequency(melody[0].frequency);
}

/* ===== 정지 ===== */
void Buzzer_Stop(void)
{
    if (buzzer_tim != NULL) {
        HAL_TIM_PWM_Stop(buzzer_tim, buzzer_channel);
    }
    current_melody = NULL;
    melody_length = 0;
    melody_index = 0;
    buzzer_state = 0;
}

/* ===== 재생 중 확인 ===== */
uint8_t Buzzer_IsPlaying(void)
{
    return (buzzer_state != 0);
}

/* ===== 업데이트 (메인 루프에서 호출) ===== */
void Buzzer_Update(void)
{
    if (buzzer_state == 0 || current_melody == NULL)
        return;

    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - buzzer_tick;

    if (buzzer_state == 1) {  // 음표 재생 중
        if (elapsed >= current_melody[melody_index].duration) {
            // 음표 끝 - 무음으로 전환
            Buzzer_SetFrequency(0);
            buzzer_state = 2;  // 쉼표 상태
            buzzer_tick = now;
        }
    }
    else if (buzzer_state == 2) {  // 쉼표 중
        if (elapsed >= 30) {  // 30ms 간격
            melody_index++;

            // 다음 음표 확인
            if (melody_index >= melody_length ||
                (current_melody[melody_index].frequency == 0 &&
                 current_melody[melody_index].duration == 0)) {
                // 멜로디 종료
                Buzzer_Stop();
            }
            else {
                // 다음 음표 재생
                Buzzer_SetFrequency(current_melody[melody_index].frequency);
                buzzer_state = 1;
                buzzer_tick = now;
            }
        }
    }
}

/* ===== 미리 정의된 멜로디 ===== */
void Buzzer_PlayMario(void)
{
    Buzzer_PlayMelody(mario_theme, sizeof(mario_theme) / sizeof(mario_theme[0]));
}

void Buzzer_PlayAlert(void)
{
    Buzzer_PlayMelody(alert_melody, sizeof(alert_melody) / sizeof(alert_melody[0]));
}

void Buzzer_PlayElise(void)
{
    Buzzer_PlayMelody(elise_melody, sizeof(elise_melody) / sizeof(elise_melody[0]));
}

void Buzzer_PlayReset(void)
{
    Buzzer_PlayMelody(reset_melody, sizeof(reset_melody) / sizeof(reset_melody[0]));
}

void Buzzer_PlayStop(void)
{
    Buzzer_PlayMelody(stop_melody, sizeof(stop_melody) / sizeof(stop_melody[0]));
}

void Buzzer_PlayStart(void)
{
    Buzzer_PlayMelody(reset_melody, sizeof(reset_melody) / sizeof(reset_melody[0]));
}

void Buzzer_PlayOk(void)
{
    Buzzer_PlayMelody(ok_melody, sizeof(ok_melody) / sizeof(ok_melody[0]));
}

void Buzzer_PlayPing(void)
{
    Buzzer_PlayMelody(idle_ping_melody, sizeof(idle_ping_melody) / sizeof(idle_ping_melody[0]));
}


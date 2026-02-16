/**
 * @file buzzer.h
 * @brief PWM 부저 드라이버 헤더 (TIM3 기반)
 */

#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

/* ===== 음표 주파수 정의 (Hz) - 전체 음계 ===== */
// 4옥타브
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494

// 5옥타브
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988

// 6옥타브
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976

// 7옥타브
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951

#define REST 0

/* ===== 음표 길이 (ms) ===== */
#define WHOLE     1400
#define HALF      700
#define QUARTER   350
#define EIGHTH    175
#define SIXTEENTH 90

/* ===== 음표 구조체 ===== */
typedef struct {
    uint16_t frequency;  // 주파수 (Hz)
    uint16_t duration;   // 재생 시간 (ms)
} BuzzerNote_t;

/* ===== API ===== */
void Buzzer_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void Buzzer_PlayTone(uint16_t freq, uint16_t duration);
void Buzzer_PlayMelody(const BuzzerNote_t *melody, uint16_t length);
void Buzzer_PlayElise(void);
void Buzzer_PlayAlert(void);
void Buzzer_PlayMario(void);
void Buzzer_Stop(void);
void Buzzer_Update(void);
uint8_t Buzzer_IsPlaying(void);

#endif /* __BUZZER_H */

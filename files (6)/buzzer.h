/**
 * @file buzzer.h
 * @brief 논블로킹 부저 드라이버 헤더
 */

#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>

/* ===== 음표 구조체 ===== */
typedef struct {
    uint16_t on_ms;
    uint16_t off_ms;
} BuzzerNote_t;

/* ===== 기본 API ===== */
void Buzzer_On(void);
void Buzzer_Off(void);

/* ===== 논블로킹 멜로디 API ===== */
void Buzzer_PlayMelody(const BuzzerNote_t *melody, uint8_t length);
void Buzzer_PlayElise(void);
void Buzzer_PlayAlert(void);
void Buzzer_Stop(void);
void Buzzer_Update(void);       // main loop에서 매번 호출
uint8_t Buzzer_IsPlaying(void);

#endif /* __BUZZER_H */

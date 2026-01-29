#ifndef __RGB_LED_H
#define __RGB_LED_H

#include "main.h"

/* RGB LED 함수 선언 */
void RGB_Init(void);
void RGB_Set(int color);    // ★ int 타입으로 선언!
void RGB_Off(void);

#endif /* __RGB_LED_H */

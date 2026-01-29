#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include "stm32f1xx_hal.h"

/* ===============================
 * Ultrasonic Sensor (HC-SR04)
 * =============================== */
#define US_TRIG_PORT   GPIOA
#define US_TRIG_PIN    GPIO_PIN_0

#define US_ECHO_PORT   GPIOA
#define US_ECHO_PIN    GPIO_PIN_1

/* ===============================
 * Timer Configuration
 * =============================== */
/* CubeMX에서 생성된 TIM3 사용 */
extern TIM_HandleTypeDef htim3;
#define US_TIM         htim3

/* ===============================
 * Distance Threshold (cm)
 * =============================== */
#define DIST_DANGER_CM 40    // 위험
#define DIST_SAFE_CM   60    // 안전

/* ===============================
 * Servo Configuration
 * =============================== */
#define SERVO_MIN_DEG  0
#define SERVO_MAX_DEG  180
#define SERVO_STEP_DEG 5

/* ===============================
 * Motor Configuration
 * =============================== */
#define MOTOR_BASE_SPEED 60   // %
#define MOTOR_TURN_SPEED 40   // %

#endif /* ROBOT_CONFIG_H */

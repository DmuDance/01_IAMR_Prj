#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include "main.h"   // CubeMX 핀 정의 사용

/* =====================================================
 * Robot Hardware Logical Mapping
 * ===================================================== */

/* ===============================
 * DC Motor (TC118S)
 * =============================== */

/* Right Front */
#define MOTOR_RFF_PORT   RFF_GPIO_Port
#define MOTOR_RFF_PIN    RFF_Pin
#define MOTOR_RFB_PORT   RFB_GPIO_Port
#define MOTOR_RFB_PIN    RFB_Pin

/* Right Back */
#define MOTOR_RBF_PORT   RBF_GPIO_Port
#define MOTOR_RBF_PIN    RBF_Pin
#define MOTOR_RBB_PORT   RBB_GPIO_Port
#define MOTOR_RBB_PIN    RBB_Pin

/* Left Front */
#define MOTOR_LFF_PORT   LFF_GPIO_Port
#define MOTOR_LFF_PIN    LFF_Pin
#define MOTOR_LFB_PORT   LFB_GPIO_Port
#define MOTOR_LFB_PIN    LFB_Pin

/* Left Back */
#define MOTOR_LBF_PORT   LBF_GPIO_Port
#define MOTOR_LBF_PIN    LBF_Pin
#define MOTOR_LBB_PORT   LBB_GPIO_Port
#define MOTOR_LBB_PIN    LBB_Pin


/* ===============================
 * Ultrasonic Sensor
 * =============================== */
#define ULTRASONIC_TRIG_PORT  TRIG_GPIO_Port
#define ULTRASONIC_TRIG_PIN   TRIG_Pin
#define ULTRASONIC_ECHO_PORT  ECHO_GPIO_Port
#define ULTRASONIC_ECHO_PIN   ECHO_Pin


/* ===============================
 * Servo
 * =============================== */
#define SERVO_CENTER_ANGLE  90
#define SERVO_MIN_ANGLE     30
#define SERVO_MAX_ANGLE     150
#define SERVO_STEP_ANGLE    10


/* ===============================
 * Buzzer
 * =============================== */
#define BUZZER_PORT  BUZZER_GPIO_Port
#define BUZZER_PIN   BUZZER_Pin


/* ===============================
 * Distance Threshold (cm)
 * =============================== */
#define DIST_SAFE        40
#define DIST_WARNING     20
#define DIST_DANGER      10

#endif /* ROBOT_CONFIG_H */

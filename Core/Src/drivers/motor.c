#include "drivers/motor.h"
#include "robot_config.h"

/* ===============================
 * 내부 모터 제어 유틸
 * =============================== */

static void RF(MotorDir_t dir)
{
    if (dir == MOTOR_FORWARD) {
        HAL_GPIO_WritePin(MOTOR_RFF_PORT, MOTOR_RFF_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_RFB_PORT, MOTOR_RFB_PIN, GPIO_PIN_RESET);
    }
    else if (dir == MOTOR_BACKWARD) {
        HAL_GPIO_WritePin(MOTOR_RFF_PORT, MOTOR_RFF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_RFB_PORT, MOTOR_RFB_PIN, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(MOTOR_RFF_PORT, MOTOR_RFF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_RFB_PORT, MOTOR_RFB_PIN, GPIO_PIN_RESET);
    }
}

static void RB(MotorDir_t dir)
{
    if (dir == MOTOR_FORWARD) {
        HAL_GPIO_WritePin(MOTOR_RBF_PORT, MOTOR_RBF_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_RBB_PORT, MOTOR_RBB_PIN, GPIO_PIN_RESET);
    }
    else if (dir == MOTOR_BACKWARD) {
        HAL_GPIO_WritePin(MOTOR_RBF_PORT, MOTOR_RBF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_RBB_PORT, MOTOR_RBB_PIN, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(MOTOR_RBF_PORT, MOTOR_RBF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_RBB_PORT, MOTOR_RBB_PIN, GPIO_PIN_RESET);
    }
}

static void LF(MotorDir_t dir)
{
    if (dir == MOTOR_FORWARD) {
        HAL_GPIO_WritePin(MOTOR_LFF_PORT, MOTOR_LFF_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_LFB_PORT, MOTOR_LFB_PIN, GPIO_PIN_RESET);
    }
    else if (dir == MOTOR_BACKWARD) {
        HAL_GPIO_WritePin(MOTOR_LFF_PORT, MOTOR_LFF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_LFB_PORT, MOTOR_LFB_PIN, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(MOTOR_LFF_PORT, MOTOR_LFF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_LFB_PORT, MOTOR_LFB_PIN, GPIO_PIN_RESET);
    }
}

static void LB(MotorDir_t dir)
{
    if (dir == MOTOR_FORWARD) {
        HAL_GPIO_WritePin(MOTOR_LBF_PORT, MOTOR_LBF_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_LBB_PORT, MOTOR_LBB_PIN, GPIO_PIN_RESET);
    }
    else if (dir == MOTOR_BACKWARD) {
        HAL_GPIO_WritePin(MOTOR_LBF_PORT, MOTOR_LBF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_LBB_PORT, MOTOR_LBB_PIN, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(MOTOR_LBF_PORT, MOTOR_LBF_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_LBB_PORT, MOTOR_LBB_PIN, GPIO_PIN_RESET);
    }
}

/* ===============================
 * 외부 API
 * =============================== */

void Motor_Init(void)
{
    Motor_Stop();
}

void Motor_Stop(void)
{
    RF(MOTOR_STOP);
    RB(MOTOR_STOP);
    LF(MOTOR_STOP);
    LB(MOTOR_STOP);
}

void Motor_Forward(void)
{
    RF(MOTOR_FORWARD);
    RB(MOTOR_FORWARD);
    LF(MOTOR_FORWARD);
    LB(MOTOR_FORWARD);
}

void Motor_Backward(void)
{
    RF(MOTOR_BACKWARD);
    RB(MOTOR_BACKWARD);
    LF(MOTOR_BACKWARD);
    LB(MOTOR_BACKWARD);
}

void Motor_Right(void)
{
    LF(MOTOR_BACKWARD);
    LB(MOTOR_BACKWARD);
    RF(MOTOR_FORWARD);
    RB(MOTOR_FORWARD);
}

void Motor_Left(void)
{
    LF(MOTOR_FORWARD);
    LB(MOTOR_FORWARD);
    RF(MOTOR_BACKWARD);
    RB(MOTOR_BACKWARD);
}

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "robot_state.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
/* 함수 선언 추가 - RGB LED 관련 */
void Set_LED_By_State(RobotState_t state);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BUZZER_Pin GPIO_PIN_13
#define BUZZER_GPIO_Port GPIOC
#define TRIG_Pin GPIO_PIN_0
#define TRIG_GPIO_Port GPIOA
#define ECHO_Pin GPIO_PIN_1
#define ECHO_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define RED_Pin GPIO_PIN_5
#define RED_GPIO_Port GPIOC
#define LBB_Pin GPIO_PIN_10
#define LBB_GPIO_Port GPIOB
#define GPIOB_Pin GPIO_PIN_12
#define GPIOB_GPIO_Port GPIOB
#define GREEN_Pin GPIO_PIN_6
#define GREEN_GPIO_Port GPIOC
#define BLUE_Pin GPIO_PIN_8
#define BLUE_GPIO_Port GPIOC
#define GPIOA_Pin GPIO_PIN_8
#define GPIOA_GPIO_Port GPIOA
#define LBF_Pin GPIO_PIN_9
#define LBF_GPIO_Port GPIOA
#define LFB_Pin GPIO_PIN_10
#define LFB_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define GPIOD_Pin GPIO_PIN_2
#define GPIOD_GPIO_Port GPIOD
#define LFF_Pin GPIO_PIN_3
#define LFF_GPIO_Port GPIOB
#define RFF_Pin GPIO_PIN_4
#define RFF_GPIO_Port GPIOB
#define RFB_Pin GPIO_PIN_5
#define RFB_GPIO_Port GPIOB
#define RBF_Pin GPIO_PIN_8
#define RBF_GPIO_Port GPIOB
#define RBB_Pin GPIO_PIN_9
#define RBB_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
/* ============================================
 * RGB LED 드라이버 색상 상수 정의
 * ============================================ */
#define RGB_COLOR_GREEN   0
#define RGB_COLOR_RED     1
#define RGB_COLOR_ORANGE  2

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

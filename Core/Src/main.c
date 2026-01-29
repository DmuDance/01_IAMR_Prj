/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "drivers/ultrasonic.h"
#include "drivers/servo.h"
#include "robot_config.h"
#include "robot_state.h"
#include "drivers/motor.h"
#include "drivers/buzzer.h"
#include "drivers/anim.h"
#include "drivers/lcd_st7735.h"
#include "ui_fsm.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define delay_ms HAL_Delay
#define ADDRESS   0x27 << 1
#define RS1_EN1   0x05
#define RS1_EN0   0x01
#define RS0_EN1   0x04
#define RS0_EN0   0x00
#define BackLight 0x08
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi2;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
int delay = 0;
int value = 0;

static RobotState_t prevState = STATE_IDLE;
int8_t scan_dir = 1;
uint8_t scan_angle = 30;
uint32_t last_scan_tick = 0;
uint8_t bt_rx_char;

uint8_t start_flag = 0;
uint8_t manual_mode = 0;
uint8_t rx_char;

uint16_t min_dist = 999;
uint8_t  min_angle = 90;

uint8_t manual_command = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI2_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
void Set_LED_By_State(RobotState_t state);
void I2C_ScanAddresses(void);

void delay_us(int us){
	value = 3;
	delay = us * value;
	for(int i=0;i < delay;i++);
}

void LCD_DATA(uint8_t data){
	uint8_t temp=(data & 0xF0)|RS1_EN1|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	temp=(data & 0xF0)|RS1_EN0|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	delay_us(4);
	temp=((data << 4) & 0xF0)|RS1_EN1|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	temp = ((data << 4) & 0xF0)|RS1_EN0|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	delay_us(50);
}

void LCD_CMD(uint8_t cmd){
	uint8_t temp=(cmd & 0xF0)|RS0_EN1|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	temp=(cmd & 0xF0)|RS0_EN0|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	delay_us(4);
	temp=((cmd << 4) & 0xF0)|RS0_EN1|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	temp=((cmd << 4) & 0xF0)|RS0_EN0|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	delay_us(50);
}

void LCD_CMD_4bit(uint8_t cmd){
	uint8_t temp=((cmd << 4) & 0xF0)|RS0_EN1|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	temp=((cmd << 4) & 0xF0)|RS0_EN0|BackLight;
	while(HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 1000)!=HAL_OK);
	delay_us(50);
}

void LCD_INIT(void){
	delay_ms(100);
	LCD_CMD_4bit(0x03); delay_ms(5);
	LCD_CMD_4bit(0x03); delay_us(100);
	LCD_CMD_4bit(0x03); delay_us(100);
	LCD_CMD_4bit(0x02); delay_us(100);
	LCD_CMD(0x28);
	LCD_CMD(0x08);
	LCD_CMD(0x01);
	delay_ms(3);
	LCD_CMD(0x06);
	LCD_CMD(0x0C);
}

void LCD_XY(char x, char y){
	if      (y == 0) LCD_CMD(0x80 + x);
	else if (y == 1) LCD_CMD(0xC0 + x);
	else if (y == 2) LCD_CMD(0x94 + x);
	else if (y == 3) LCD_CMD(0xD4 + x);
}

void LCD_CLEAR(void){
	LCD_CMD(0x01);
	delay_ms(2);
}

void LCD_PUTS(char *str){
	while (*str) LCD_DATA(*str++);
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

const char* StateToStr(RobotState_t state)
{
    switch (state)
    {
    case STATE_IDLE:   return "IDLE";
    case STATE_SCAN:   return "SCAN";
    case STATE_DECIDE: return "DECIDE";
    case STATE_MOVE:   return "MOVE";
    case STATE_ALERT:  return "ALERT";
    case STATE_REVERSE: return "REVERSE";
    default:           return "UNKNOWN";
    }
}

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  if (ch == '\n')
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r", 1, 0xFFFF);
  HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 0xFFFF);
  return ch;
}

void I2C_ScanAddresses(void) {
    HAL_StatusTypeDef result;
    uint8_t i;

    printf("Scanning I2C addresses...\r\n");

    for (i = 1; i < 128; i++) {
        result = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 1, 10);
        if (result == HAL_OK) {
            printf("I2C device found at address 0x%02X\r\n", i);
        }
    }

    printf("Scan complete.\r\n");
}

void Handle_Command(uint8_t cmd)
{
    switch (cmd)
    {
    case 't':
    case 'T':
        start_flag  = 1;
        manual_mode = 0;
        Buzzer_Stop();
        printf("AUTO MODE START\r\n");
        RobotState_Set(STATE_SCAN);
        break;

    case 'x':
    case 'X':
        start_flag  = 0;
        manual_mode = 0;
        Motor_Stop();
        Buzzer_Stop();
        printf("STOP\r\n");
        RobotState_Set(STATE_IDLE);
        manual_command = 0;
        break;

    case 'w':
    case 'W':
        manual_mode = 1;
        start_flag  = 0;
        Buzzer_Stop();
        Motor_Forward();
        printf("MANUAL: FORWARD\r\n");
        RobotState_Set(STATE_MOVE);
        manual_command = 1;
        break;

    case 's':
    case 'S':
        manual_mode = 1;
        start_flag  = 0;
        Motor_Backward();
        Buzzer_PlayMario();
        RobotState_Set(STATE_REVERSE);
        printf("MANUAL: BACKWARD\r\n");
        manual_command = 2;
        break;

    case 'a':
    case 'A':
        manual_mode = 1;
        start_flag  = 0;
        Buzzer_Stop();
        Motor_Left();
        printf("MANUAL: LEFT\r\n");
        manual_command = 3;
        break;

    case 'd':
    case 'D':
        manual_mode = 1;
        start_flag  = 0;
        Buzzer_Stop();
        Motor_Right();
        printf("MANUAL: RIGHT\r\n");
        manual_command = 4;
        break;

    case 'r':
    case 'R':
        Servo_SetAngle(90);
        printf("SERVO RESET (90 deg)\r\n");
        manual_command = 5;
        break;
    }
}

void Set_LED_By_State(RobotState_t state)
{
    switch (state)
    {
    case STATE_SCAN:
    case STATE_MOVE:
        RGB_Set(RGB_COLOR_GREEN);
        break;
    case STATE_REVERSE:
        break;
    case STATE_ALERT:
        RGB_Set(RGB_COLOR_RED);
        break;
    case STATE_DECIDE:
        RGB_Set(RGB_COLOR_ORANGE);
        break;
    default:
        RGB_Off();
        break;
    }
}

/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_SPI2_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  I2C_ScanAddresses();

  LCD_INIT();

  Motor_Init();
  Ultrasonic_Init(&htim1);
  Servo_Init(&htim2, TIM_CHANNEL_1);

  Buzzer_Off();

  RobotState_Init();
  RobotState_Set(STATE_IDLE);

  Servo_SetAngle(90);
  HAL_Delay(500);

  printf("시작하시려면 t 키를 눌러주세요.\r\n");
  HAL_UART_Receive_IT(&huart2, &rx_char, 1);

  LCD_Init();
  LCD_Clear(COLOR_BLACK);
  Anim_Init();
  UI_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
      RobotState_t currentState = RobotState_Get();
      Set_LED_By_State(currentState);

      Buzzer_Update();

      static uint32_t ui_tick = 0;
      uint32_t now = HAL_GetTick();

      if (now - ui_tick >= 50)
      {
          ui_tick = now;
          UI_Update();
          Anim_Update();
      }

      if (currentState != prevState)
      {
          switch (currentState)
          {
          case STATE_REVERSE:
          {
              Motor_Backward();
              static uint32_t reverse_blink_tick = 0;
              uint32_t now = HAL_GetTick();

              if (now - reverse_blink_tick >= 500)
              {
                  reverse_blink_tick = now;
                  RGB_Set(RGB_COLOR_ORANGE);
              }
              else if (now - reverse_blink_tick >= 250)
              {
                  RGB_Off();
              }
              break;
          }

          case STATE_ALERT:
              Buzzer_PlayAlert();
              break;

          default:
              break;
          }
          prevState = currentState;
      }

      if (!start_flag)
      {
          continue;
      }

      switch (currentState)
      {
      case STATE_IDLE:
          break;

      case STATE_SCAN:
      {
          if (HAL_GetTick() - last_scan_tick < 40)
              break;

          last_scan_tick = HAL_GetTick();

          Servo_SetAngle(scan_angle);

          uint16_t dist = Ultrasonic_GetDistance();

          printf("STATE:%s | angle=%3d | dist=%3d cm\r\n",
                 StateToStr(currentState), scan_angle, dist);

          if (dist > 0 && dist < min_dist)
          {
              min_dist  = dist;
              min_angle = scan_angle;
          }

          scan_angle += scan_dir * 10;

          if (scan_angle >= 150)
          {
              scan_angle = 150;
              scan_dir = -1;
              RobotState_Set(STATE_DECIDE);
          }
          else if (scan_angle <= 30)
          {
              scan_angle = 30;
              scan_dir = 1;
              RobotState_Set(STATE_DECIDE);
          }

          break;
      }

      case STATE_DECIDE:
          printf("STATE:%s | min_angle=%d | min_dist=%d cm\r\n",
                 StateToStr(currentState), min_angle, min_dist);

          if (min_dist > DIST_SAFE)
              RobotState_Set(STATE_MOVE);
          else
              RobotState_Set(STATE_ALERT);
          break;

      case STATE_MOVE:
      {
          printf("STATE:%s | FORWARD\r\n", StateToStr(currentState));
          Motor_Forward();
          RobotState_Set(STATE_SCAN);
          break;
      }

      case STATE_REVERSE:
      {
          Motor_Backward();
          static uint32_t reverse_blink_tick = 0;
          uint32_t now = HAL_GetTick();

          if (now - reverse_blink_tick >= 500)
          {
              reverse_blink_tick = now;
              RGB_Set(RGB_COLOR_ORANGE);
          }
          else if (now - reverse_blink_tick >= 250)
          {
              RGB_Off();
          }
          break;
      }

      case STATE_ALERT:
      {
          static uint32_t avoid_start = 0;
          static uint8_t  avoiding = 0;

          if (!avoiding)
          {
              if (min_angle < 90) Motor_Right();
              else Motor_Left();

              avoid_start = HAL_GetTick();
              avoiding = 1;
          }

          if (avoiding && HAL_GetTick() - avoid_start > 120)
          {
              Motor_Stop();
              avoiding = 0;
              min_dist = 999;
              RobotState_Set(STATE_SCAN);
          }
          break;
      }
      }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }

  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_SPI2_Init(void)
{
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 63;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 1279;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 1279;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, BUZZER_Pin|RED_Pin|GREEN_Pin|BLUE_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, TRIG_Pin|GPIOA_Pin|LBF_Pin|LFB_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LBB_Pin|GPIOB_Pin|LFF_Pin|RFF_Pin|RFB_Pin|RBF_Pin|RBB_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD_GPIO_Port, GPIOD_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = BUZZER_Pin|RED_Pin|GREEN_Pin|BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TRIG_Pin|LBF_Pin|LFB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = ECHO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ECHO_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LBB_Pin|LFF_Pin|RFF_Pin|RFB_Pin|RBF_Pin|RBB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIOB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIOA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIOD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD_GPIO_Port, &GPIO_InitStruct);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        Handle_Command(rx_char);
        HAL_UART_Receive_IT(&huart2, &rx_char, 1);
    }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif

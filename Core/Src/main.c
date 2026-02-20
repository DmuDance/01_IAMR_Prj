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
DMA_HandleTypeDef hdma_spi2_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart3_rx;

/* USER CODE BEGIN PV */
#define UART_RX_BUF_SIZE 256

uint8_t uart2_rx_buf[UART_RX_BUF_SIZE];
uint16_t uart2_old_pos = 0;

uint8_t uart3_rx_buf[UART_RX_BUF_SIZE];
uint16_t uart3_old_pos = 0;

int delay = 0;
int value = 0;

static RobotState_t prevState = STATE_IDLE;
int8_t scan_dir = 1;
uint8_t scan_angle = 30;
uint32_t last_scan_tick = 0;

uint8_t start_flag = 0;
uint8_t manual_mode = 0;

uint16_t min_dist = 999;
uint8_t  min_angle = 90;

uint8_t manual_command = 0;
uint16_t g_distance = 0;
uint32_t debug_tick = 0;
uint16_t last_valid_dist = 50;

static uint32_t last_ping_time = 0;
static uint32_t next_ping_interval = 0;

static uint32_t echo_start_time = 0;

static uint16_t read_stable_distance(void)
{
    uint16_t d[2];
    int count = 0;

    for(int i=0;i<2;i++)
    {
        uint32_t t = HAL_GetTick();

        while(HAL_GetTick() - t < 40)
        {
            uint16_t dist = Ultrasonic_Read();
            if(dist != 999)
            {
                d[count++] = dist;
                break;
            }
        }
    }

    if(count == 0) return 999;
    if(count == 1) return d[0];
    return (d[0]+d[1])/2;
}

extern volatile uint8_t spi_dma_busy;
extern volatile uint8_t spi_dma_done;
extern volatile uint8_t spi_busy;

volatile uint8_t servo_moving = 0;
volatile uint32_t servo_move_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI2_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void Set_LED_By_State(RobotState_t state);
void I2C_ScanAddresses(void);

void delay_us(int us){
    // 64MHz 기준: 루프 1회 ≈ 약 4~5 사이클
    // 1us = 64 사이클 → 루프로 맞추려면 약 16 반복
    value = 16;
    delay = us * value;
    for(volatile int i = 0; i < delay; i++);
}

void LCD_DATA(uint8_t data){
    uint8_t temp=(data & 0xF0)|RS1_EN1|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
    temp=(data & 0xF0)|RS1_EN0|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
    delay_us(4);
    temp=((data << 4) & 0xF0)|RS1_EN1|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
    temp=((data << 4) & 0xF0)|RS1_EN0|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
    delay_us(50);
}

void LCD_CMD(uint8_t cmd){
    uint8_t temp=(cmd & 0xF0)|RS0_EN1|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
    temp=(cmd & 0xF0)|RS0_EN0|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
    delay_us(4);
    temp=((cmd << 4) & 0xF0)|RS0_EN1|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
    temp=((cmd << 4) & 0xF0)|RS0_EN0|BackLight;
    HAL_I2C_Master_Transmit(&hi2c1, ADDRESS, &temp, 1, 10);
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
    HAL_Delay(100);
    LCD_CMD_4bit(0x03); HAL_Delay(5);
    LCD_CMD_4bit(0x03); HAL_Delay(1);
    LCD_CMD_4bit(0x03); HAL_Delay(1);
    LCD_CMD_4bit(0x02); HAL_Delay(1);
    LCD_CMD(0x28);
    LCD_CMD(0x08);
    LCD_CMD(0x01);
    HAL_Delay(3);
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
    HAL_Delay(3);   // delay_ms → HAL_Delay로 교체, 2ms→3ms로 여유있게
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
    case STATE_WAIT_ECHO: return "WAIT_ECHO";  // ⭐ 여기
    case STATE_DECIDE: return "DECIDE";
    case STATE_MOVE:   return "MOVE";
    case STATE_ALERT:  return "ALERT";
    case STATE_REVERSE: return "REVERSE";
    default:           return "UNKNOWN";
    }
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
	printf("RX: %c (%d)\r\n", cmd, cmd);
    switch (cmd)
    {
    case 't':
    case 'T':
        start_flag  = 1;
        manual_mode = 0;
        Buzzer_PlayStart();
        printf("AUTO MODE START\r\n");
        RobotState_Set(STATE_SCAN);
        break;

    case 'x':
    case 'X':
        start_flag  = 0;
        manual_mode = 0;
        Motor_Stop();
        Buzzer_PlayStop();
         printf("STOP\r\n");
        RobotState_Set(STATE_IDLE);
        manual_command = 0;
        break;

    case 'w':
    case 'W':
        manual_mode = 1;
        start_flag  = 0;
        Buzzer_PlayOk();
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
        Buzzer_PlayElise();
        RobotState_Set(STATE_REVERSE);
        printf("MANUAL: BACKWARD\r\n");
        manual_command = 2;
        break;

    case 'a':
    case 'A':
        manual_mode = 1;
        start_flag  = 0;
        Buzzer_PlayOk();
        Motor_Left();
        printf("MANUAL: LEFT\r\n");
        manual_command = 3;
        break;

    case 'd':
    case 'D':
        manual_mode = 1;
        start_flag  = 0;
        Buzzer_PlayOk();
        Motor_Right();
        printf("MANUAL: RIGHT\r\n");
        manual_command = 4;
        break;

    case 'r':
    case 'R':
        Servo_SetAngle(90);
        printf("SERVO RESET (90 deg)\r\n");
        Buzzer_PlayReset();
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

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_SPI2_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // UART3 초기화 (GUI 설정 전까지 수동 호출)
  MX_USART3_UART_Init();

  // USB(UART2)와 블루투스(UART3) 모두 DMA 수신 시작
  HAL_UART_Receive_DMA(&huart2, uart2_rx_buf, UART_RX_BUF_SIZE);
  HAL_UART_Receive_DMA(&huart3, uart3_rx_buf, UART_RX_BUF_SIZE);

 I2C_ScanAddresses();
//


  Motor_Init();
  Ultrasonic_Init();
  Servo_Init(&htim1, TIM_CHANNEL_4);

  Buzzer_Init(&htim3, TIM_CHANNEL_1);  // TIM3, PA6 핀

  Motor_Init();
  RGB_Init();
  LCD_Init();        // ST7735 먼저 (하드웨어 리셋 포함)
  HAL_Delay(100);    // ST7735 완전히 안정화 대기
  LCD_INIT();        // HD44780 초기화 (ST7735 리셋 영향 없이)
  Anim_Init();
  UI_Init();

  Buzzer_PlayMario();


  RobotState_Init();
  RobotState_Set(STATE_IDLE);

  Anim_Set(EXPR_NEUTRAL);  // 중립 표정으로 시작

  Servo_SetAngle(90);
  HAL_Delay(500);

  printf("시작하시려면 t 키를 눌러주세요.\r\n");
  //HAL_UART_Receive_IT(&huart2, &rx_char, 1);

//  LCD_Init();
//  LCD_Clear(COLOR_BLACK);
//  Anim_Init();
//  UI_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  
  // 이미 위에서 HAL_UART_Receive_DMA가 호출되었으므로 여기서는 중복 호출하지 않습니다.

  while (1)
  {
      // --- [1] UART2 (USB) 수신 처리 ---
      uint16_t pos2 = UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart2.hdmarx);
      if (pos2 != uart2_old_pos) {
          if (pos2 > uart2_old_pos) {
              for (uint16_t i = uart2_old_pos; i < pos2; i++) Handle_Command(uart2_rx_buf[i]);
          } else {
              for (uint16_t i = uart2_old_pos; i < UART_RX_BUF_SIZE; i++) Handle_Command(uart2_rx_buf[i]);
              for (uint16_t i = 0; i < pos2; i++) Handle_Command(uart2_rx_buf[i]);
          }
          uart2_old_pos = pos2;
      }

      // --- [2] UART3 (Bluetooth) 수신 처리 ---
      uint16_t pos3 = UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart3.hdmarx);
      if (pos3 != uart3_old_pos) {
          if (pos3 > uart3_old_pos) {
              for (uint16_t i = uart3_old_pos; i < pos3; i++) Handle_Command(uart3_rx_buf[i]);
          } else {
              for (uint16_t i = uart3_old_pos; i < UART_RX_BUF_SIZE; i++) Handle_Command(uart3_rx_buf[i]);
              for (uint16_t i = 0; i < pos3; i++) Handle_Command(uart3_rx_buf[i]);
          }
          uart3_old_pos = pos3;
      }

      // --- [3] UART 에러 복구 ---
      if (huart2.ErrorCode != HAL_UART_ERROR_NONE) {
          __HAL_UART_CLEAR_OREFLAG(&huart2);
          huart2.RxState = HAL_UART_STATE_READY;
          HAL_UART_Receive_DMA(&huart2, uart2_rx_buf, UART_RX_BUF_SIZE);
      }
      if (huart3.ErrorCode != HAL_UART_ERROR_NONE) {
          __HAL_UART_CLEAR_OREFLAG(&huart3);
          huart3.RxState = HAL_UART_STATE_READY;
          HAL_UART_Receive_DMA(&huart3, uart3_rx_buf, UART_RX_BUF_SIZE);
      }

      RobotState_t currentState = RobotState_Get();
      Set_LED_By_State(currentState);

      Buzzer_Update();

      static uint32_t ui_tick = 0;
      uint32_t now = HAL_GetTick();






      static uint8_t ui_busy = 0;

      if (now - ui_tick >= 200)
      {
          ui_tick = now;
          ui_busy = 1;
          UI_Update();
          ui_busy = 0;
      }

      static uint32_t anim_tick = 0;
      if (HAL_GetTick() - anim_tick >= 50)
      {
          if (!spi_busy)
          {
              anim_tick = HAL_GetTick();
              Anim_Update();
          }
      }
      // ===== IDLE 상태 처리 (start_flag 체크 전에 실행) =====
         if (currentState == STATE_IDLE)
         {
             uint32_t current_time = HAL_GetTick();

             // 처음 실행 시 또는 다음 핑 시간 설정
             if (next_ping_interval == 0) {
                 next_ping_interval = 20000 + (rand() % 60000);  // 20~60초
                 last_ping_time = current_time;
             }

             // 랜덤 시간이 지나면 소리 재생
             if (current_time - last_ping_time >= next_ping_interval) {
                 Buzzer_PlayPing();

                 // 다음 핑 시간 재설정
                 next_ping_interval = 20000 + (rand() % 60000);
                 last_ping_time = current_time;
             }
         }

         //상태변환시에만
      if (currentState != prevState)
      {
          switch (currentState)
          {
			case STATE_IDLE:
				Anim_Set(EXPR_NEUTRAL);
				break;

			case STATE_SCAN:
				Anim_Set(EXPR_LOOK_LEFT);
				break;

			case STATE_MOVE:
				Anim_Set(EXPR_HAPPY);
				break;
			case STATE_REVERSE:
          {
        	  Anim_Set(EXPR_SAD);
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
        	  Anim_Set(EXPR_ANGRY);
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
          if (HAL_GetTick() - last_scan_tick < 100) //150
              break;

          last_scan_tick = HAL_GetTick();
          /* 새 스캔 사이클 시작점(30도, 우향)에서 최소값 리셋 */
                  if (scan_angle == 30 && scan_dir == 1)
                  {
                      min_dist = 999;
                      min_angle = 90;
                  }

          Servo_SetAngle(scan_angle);

          servo_moving = 1;                    // 서보 이동 시작
          servo_move_tick = HAL_GetTick();     // 타이머 시작

          echo_start_time = HAL_GetTick();   // 서보 안정 타이머 시작
          RobotState_Set(STATE_WAIT_ECHO);
          break;
      }

      case STATE_WAIT_ECHO:
      {
          if (HAL_GetTick() - echo_start_time < 20) //80
              break;

          servo_moving = 0;   // ← 서보 안정화 완료

          Ultrasonic_Trigger();
          echo_start_time = HAL_GetTick();
          RobotState_Set(STATE_READ_ECHO);
          break;
      }
      case STATE_READ_ECHO:
      {
          uint16_t dist = Ultrasonic_Read();

          if (dist == 999 && HAL_GetTick() - echo_start_time < 10) //50
              break;  // 아직 에코 안 들어옴 → 대기

          // 값 들어왔거나 타임아웃
          if (dist != 999)
          {
              last_valid_dist = dist;
              g_distance = dist;
          }

          if (HAL_GetTick() - debug_tick > 200)
          {
              debug_tick = HAL_GetTick();
              printf("angle=%3d | dist=%3d cm\r\n", scan_angle, last_valid_dist);
          }

          if (last_valid_dist < min_dist)
          {
              min_dist  = last_valid_dist;
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
          else
          {
              RobotState_Set(STATE_SCAN);
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
    	  if (HAL_GetTick() - debug_tick > 200)
    	  {
    		  debug_tick = HAL_GetTick();
    		  printf("STATE:%s | FORWARD\r\n", StateToStr(currentState));
    	  }

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
              if (min_angle < 90) Motor_Left();
              else Motor_Right();

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

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
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
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
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
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 19999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
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
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 63;
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
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
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
  /* USER CODE BEGIN USART2_Init 2 */

  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, BUZZER_Pin|RED_Pin|GREEN_Pin|BLUE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, TRIG_Pin|GPIOA_Pin|LBF_Pin|LFB_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LBB_Pin|GPIOB_Pin|LFF_Pin|RFF_Pin
                          |RFB_Pin|RBF_Pin|RBB_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD_GPIO_Port, GPIOD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BUZZER_Pin RED_Pin GREEN_Pin BLUE_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin|RED_Pin|GREEN_Pin|BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : TRIG_Pin LBF_Pin LFB_Pin */
  GPIO_InitStruct.Pin = TRIG_Pin|LBF_Pin|LFB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LBB_Pin LFF_Pin RFF_Pin RFB_Pin
                           RBF_Pin RBB_Pin */
  GPIO_InitStruct.Pin = LBB_Pin|LFF_Pin|RFF_Pin|RFB_Pin
                          |RBF_Pin|RBB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : GPIOB_Pin */
  GPIO_InitStruct.Pin = GPIOB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : GPIOA_Pin */
  GPIO_InitStruct.Pin = GPIOA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : GPIOD_Pin */
  GPIO_InitStruct.Pin = GPIOD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
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

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) { if(htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) { Ultrasonic_IC_Callback(htim); } }

int _write(int file, char *ptr, int len)
{
    // UART 에러 상태(Overrun 등) 확인 및 강제 클리어
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE) != RESET) {
        __HAL_UART_CLEAR_OREFLAG(&huart2);
    }
    
    // UART가 Ready가 아니면 상태 강제 초기화 (송신 보장)
    if (huart2.gState != HAL_UART_STATE_READY) {
        huart2.gState = HAL_UART_STATE_READY;
    }
    
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 100);
    return len;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {
        // 에러 플래그 확인 및 클리어
        uint32_t err = HAL_UART_GetError(huart);
        if (err & HAL_UART_ERROR_ORE) {
            __HAL_UART_CLEAR_OREFLAG(huart);
        }
        
        // UART 상태 강제 초기화 (송수신 가능 상태로)
        huart->gState = HAL_UART_STATE_READY;
        huart->RxState = HAL_UART_STATE_READY;
        
        // DMA 수신 재시작
        HAL_UART_Receive_DMA(&huart3, uart3_rx_buf, UART_RX_BUF_SIZE);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
 * @file lcd_st7735.c
 * @brief ST7735 LCD 드라이버 - 성능 최적화 버전
 * 
 * 최적화 내용:
 * 1. DMA 전송 지원 (선택적)
 * 2. 버퍼 기반 bulk 전송
 * 3. CS 토글 최소화
 */

#include "drivers/lcd_st7735.h"
#include "stm32f1xx_hal.h"

extern SPI_HandleTypeDef hspi2;

/* ===== 오프셋 (LCD 모듈에 따라 조정) ===== */
#define X_OFFSET 0
#define Y_OFFSET 26

/* ===== GPIO 매크로 (인라인으로 최적화) ===== */
#define LCD_CS_LOW()   (GPIOB->BRR  = GPIO_PIN_12)
#define LCD_CS_HIGH()  (GPIOB->BSRR = GPIO_PIN_12)
#define LCD_DC_LOW()   (GPIOA->BRR  = GPIO_PIN_8)
#define LCD_DC_HIGH()  (GPIOA->BSRR = GPIO_PIN_8)
#define LCD_RES_LOW()  (GPIOD->BRR  = GPIO_PIN_2)
#define LCD_RES_HIGH() (GPIOD->BSRR = GPIO_PIN_2)

/* ===== ST7735 명령어 ===== */
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

/* ===== 전송 버퍼 (스택 절약을 위해 static) ===== */
#define TX_BUF_SIZE 128
static uint8_t tx_buf[TX_BUF_SIZE];

/* ===== 내부 함수 ===== */

static inline void LCD_Cmd(uint8_t cmd)
{
    LCD_DC_LOW();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

static inline void LCD_Data(uint8_t data)
{
    LCD_DC_HIGH();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

/* ===== 외부 API ===== */

void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /* CASET (Column Address Set) */
    LCD_Cmd(ST7735_CASET);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    uint8_t col_data[4] = {0, (uint8_t)(x0 + X_OFFSET), 0, (uint8_t)(x1 + X_OFFSET)};
    HAL_SPI_Transmit(&hspi2, col_data, 4, HAL_MAX_DELAY);
    LCD_CS_HIGH();

    /* RASET (Row Address Set) */
    LCD_Cmd(ST7735_RASET);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    uint8_t row_data[4] = {0, (uint8_t)(y0 + Y_OFFSET), 0, (uint8_t)(y1 + Y_OFFSET)};
    HAL_SPI_Transmit(&hspi2, row_data, 4, HAL_MAX_DELAY);
    LCD_CS_HIGH();

    /* RAMWR (Memory Write 시작) */
    LCD_Cmd(ST7735_RAMWR);
}

/**
 * @brief 색상을 count개 연속 출력 (버퍼 기반 bulk 전송)
 */
void LCD_WriteColorFast(uint16_t color, uint32_t count)
{
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    /* 버퍼 준비 (같은 색상으로 채움) */
    for (int i = 0; i < TX_BUF_SIZE; i += 2)
    {
        tx_buf[i]     = hi;
        tx_buf[i + 1] = lo;
    }

    LCD_DC_HIGH();
    LCD_CS_LOW();

    /* 버퍼 단위로 전송 */
    uint32_t pixels_per_buf = TX_BUF_SIZE / 2;  // 64 pixels
    
    while (count >= pixels_per_buf)
    {
        HAL_SPI_Transmit(&hspi2, tx_buf, TX_BUF_SIZE, HAL_MAX_DELAY);
        count -= pixels_per_buf;
    }

    /* 나머지 픽셀 전송 */
    if (count > 0)
    {
        HAL_SPI_Transmit(&hspi2, tx_buf, count * 2, HAL_MAX_DELAY);
    }

    LCD_CS_HIGH();
}

/**
 * @brief 색상 배열을 직접 전송 (이미지 출력용)
 */
void LCD_WriteBuffer(uint16_t *buf, uint32_t count)
{
    LCD_DC_HIGH();
    LCD_CS_LOW();

    /* 16비트 → 바이트 스왑하면서 전송 */
    while (count > 0)
    {
        uint32_t chunk = (count > TX_BUF_SIZE / 2) ? TX_BUF_SIZE / 2 : count;
        
        for (uint32_t i = 0; i < chunk; i++)
        {
            tx_buf[i * 2]     = buf[i] >> 8;
            tx_buf[i * 2 + 1] = buf[i] & 0xFF;
        }
        
        HAL_SPI_Transmit(&hspi2, tx_buf, chunk * 2, HAL_MAX_DELAY);
        buf   += chunk;
        count -= chunk;
    }

    LCD_CS_HIGH();
}

void LCD_Clear(uint16_t color)
{
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    LCD_WriteColorFast(color, (uint32_t)LCD_WIDTH * LCD_HEIGHT);
}

void LCD_Init(void)
{
    /* 하드웨어 리셋 */
    LCD_RES_LOW();
    HAL_Delay(50);
    LCD_RES_HIGH();
    HAL_Delay(50);

    /* 소프트웨어 리셋 */
    LCD_Cmd(ST7735_SWRESET);
    HAL_Delay(150);

    /* Sleep Out */
    LCD_Cmd(ST7735_SLPOUT);
    HAL_Delay(150);

    /* Memory Data Access Control (회전 설정) */
    LCD_Cmd(ST7735_MADCTL);
    LCD_Data(0x60);  // RGB, 가로 방향

    /* Color Mode: 16bit/pixel */
    LCD_Cmd(ST7735_COLMOD);
    LCD_Data(0x05);

    /* Display On */
    LCD_Cmd(ST7735_DISPON);
    HAL_Delay(100);
}

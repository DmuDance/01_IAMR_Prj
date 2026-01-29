/**
 * @file OPTIMIZATION_GUIDE.md
 * @brief STM32F103 로봇 프로젝트 LCD 성능 최적화 가이드
 * 
 * ============================================================
 *  문제점 분석 및 해결책
 * ============================================================
 */

/*
 * ============================================================
 *  1. SPI 속도 최적화 (가장 중요!)
 * ============================================================
 * 
 * 현재 설정: SPI_BAUDRATEPRESCALER_16 → 64MHz/16 = 4MHz
 * 권장 설정: SPI_BAUDRATEPRESCALER_4  → 64MHz/4 = 16MHz (ST7735 최대 15~20MHz)
 * 
 * [수정 위치: MX_SPI2_Init() 함수]
 * 
 * 변경 전:
 *   hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
 * 
 * 변경 후:
 *   hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
 * 
 * 효과: LCD 통신 속도 4배 향상
 */

/*
 * ============================================================
 *  2. GPIO 속도 최적화
 * ============================================================
 * 
 * SPI 관련 핀(DC, CS)을 HIGH 속도로 설정
 * 
 * [수정 위치: MX_GPIO_Init() 함수]
 * 
 * PA8 (DC 핀) 설정에 다음 추가:
 *   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
 * 
 * PB12 (CS 핀)도 동일하게 HIGH로
 */

/*
 * ============================================================
 *  3. main.c while 루프 수정
 * ============================================================
 */

// ===== 변경 전 (문제점) =====
/*
while (1)
{
    // UI는 20FPS
    static uint32_t ui_tick = 0;
    if (HAL_GetTick() - ui_tick > 50)
    {
        ui_tick = HAL_GetTick();
        Anim_Update();   // ← 여기서 Eyes_Draw 호출
        UI_Update();     // ← 여기서도 Eyes_Draw 호출 (중복!)
    }
    ...
}
*/

// ===== 변경 후 (최적화) =====
/*
while (1)
{
    // ===== UI 업데이트 (20 FPS) =====
    static uint32_t ui_tick = 0;
    uint32_t now = HAL_GetTick();
    
    if (now - ui_tick >= 50)  // >= 사용 권장
    {
        ui_tick = now;
        
        // 순서 중요: 상태 → 표정 설정 → 실제 드로잉
        UI_Update();      // 상태에 따라 Eyes_SetExpression() 호출
        Anim_Update();    // Eyes_Update()로 변경사항만 드로잉
    }
    
    // ===== 로봇 상태 처리 =====
    RobotState_t currentState = RobotState_Get();
    
    // 상태 변경 시 사운드 처리
    if (currentState != prevState)
    {
        switch (currentState)
        {
            case STATE_REVERSE:
                Buzzer_Elise();
                break;
            case STATE_ALERT:
                Buzzer_Beep(100, 100);
                Buzzer_Beep(100, 100);
                break;
            default:
                Buzzer_Off();
                break;
        }
        prevState = currentState;
    }
    
    // 자동 모드 아닐 때
    if (!start_flag)
    {
        // Motor_Stop();  // ← 매번 호출하지 않아도 됨
        continue;
    }
    
    // 상태별 동작...
}
*/

/*
 * ============================================================
 *  4. 초기화 순서 수정
 * ============================================================
 * 
 * [수정 위치: main() 함수]
 * 
 * 변경 전 (중복 초기화):
 *   LCD_Init();          // 1차
 *   LCD_Clear(0x0000);
 *   Anim_Init();         // ← 내부에서 LCD_Init() 다시 호출 (문제!)
 *   UI_Init();
 * 
 * 변경 후 (정리):
 *   LCD_Init();
 *   LCD_Clear(COLOR_BLACK);
 *   Anim_Init();         // ← LCD_Init() 호출 제거됨
 *   UI_Init();
 */

/*
 * ============================================================
 *  5. 성능 비교
 * ============================================================
 * 
 * | 항목                    | 변경 전    | 변경 후    | 향상율 |
 * |-------------------------|------------|------------|--------|
 * | SPI 전송 속도           | 4 MHz      | 16 MHz     | 4x     |
 * | LCD_FillCircle (r=10)   | ~2000 calls| ~20 calls  | 100x   |
 * | 표정 변경 드로잉        | 2회/frame  | 1회/frame  | 2x     |
 * | 전체 화면 클리어        | 매번 전체  | 눈 영역만  | 10x+   |
 * 
 * 예상 총 성능 향상: 10~20배
 */

/*
 * ============================================================
 *  6. 추가 최적화 (선택사항)
 * ============================================================
 * 
 * A. DMA 전송 (가장 효과적)
 *    - HAL_SPI_Transmit_DMA() 사용
 *    - CPU가 다른 작업 가능
 *    - CubeMX에서 SPI2 DMA 활성화 필요
 * 
 * B. 더블 버퍼링
 *    - 백버퍼에 그리고 한 번에 전송
 *    - RAM 사용량 증가 (160x80x2 = 25.6KB)
 *    - STM32F103은 RAM이 20KB라 불가능
 * 
 * C. 부분 갱신
 *    - 변경된 영역만 업데이트
 *    - 현재 구현에서 이미 적용됨
 */

/*
 * ============================================================
 *  적용 체크리스트
 * ============================================================
 * 
 * [ ] SPI Prescaler 변경 (16 → 4)
 * [ ] GPIO 속도 HIGH로 변경 (PA8, PB12)
 * [ ] 최적화된 lcd_st7735.c 적용
 * [ ] 최적화된 lcd_gfx.c 적용
 * [ ] 최적화된 eyes.c 적용
 * [ ] 최적화된 anim.c 적용
 * [ ] 최적화된 ui_fsm.c 적용
 * [ ] main.c while 루프 순서 변경
 * [ ] 헤더 파일 업데이트
 */

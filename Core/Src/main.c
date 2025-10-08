#include "main.h"

#include <stdio.h>

#include "stm32_hal_legacy.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_gpio_ex.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_usart.h"

#ifndef __ARM_ACLE
#error "ACLE intrinsics support not enabled."
#endif

USART_HandleTypeDef m_uh;

void LED_init() {
  GPIO_InitTypeDef gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_5, //
      .Mode = GPIO_MODE_AF_PP,
      .Speed = GPIO_SPEED_HIGH, //
      .Pull = GPIO_NOPULL,
  };

  __HAL_RCC_GPIOB_CLK_ENABLE(); // led0-pb5-timer3channel2

  HAL_GPIO_Init(GPIOB, &gi);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

TIM_HandleTypeDef m_timer3h;
void timer3_init(int period, int psc, float duty) {
  TIM_Base_InitTypeDef timer_base = (TIM_Base_InitTypeDef){
      .Period = period - 1,
      .Prescaler = psc - 1,
      .ClockDivision = TIM_CLOCKDIVISION_DIV1,
      .CounterMode = TIM_COUNTERMODE_UP,
      .RepetitionCounter = 0,
      .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE,
  };
  m_timer3h = (TIM_HandleTypeDef){
      .Instance = TIM3,
      .Init = timer_base,
  };
  TIM_OC_InitTypeDef timer_compare = (TIM_OC_InitTypeDef){
      .OCMode = TIM_OCMODE_PWM1,
      .Pulse = (float)period * duty,
      .OCPolarity = TIM_OCPOLARITY_LOW,
      .OCFastMode = TIM_OCFAST_DISABLE,
      .OCIdleState = TIM_OCIDLESTATE_RESET,
  };
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_TIM3_PARTIAL();
  HAL_StatusTypeDef res = HAL_OK;
  res = HAL_TIM_Base_Init(&m_timer3h);
  if (res != HAL_OK) {
    while (1)
      HAL_USART_Transmit(&m_uh, (void *)"err1", 4, -1);
  }
  res = HAL_TIM_PWM_Init(&m_timer3h);
  if (res != HAL_OK) {
    while (1)
      HAL_USART_Transmit(&m_uh, (void *)"err2", 4, -1);
  }

  res = HAL_TIM_PWM_ConfigChannel(&m_timer3h, &timer_compare, TIM_CHANNEL_2);
  if (res != HAL_OK) {
    while (1)
      HAL_USART_Transmit(&m_uh, (void *)"err3", 4, -1);
  }

  res = HAL_TIM_PWM_Start(&m_timer3h, TIM_CHANNEL_2);
  if (res != HAL_OK) {
    while (1)
      HAL_USART_Transmit(&m_uh, (void *)"err4", 4, -1);
  }
}

// gpio-uart1 init
void usart1_init() {
  __HAL_RCC_USART1_CLK_ENABLE();
  m_uh = (USART_HandleTypeDef){
      .Instance = USART1,
      .Init =
          (USART_InitTypeDef){
              .BaudRate = 57600,
              .WordLength = USART_WORDLENGTH_8B,
              .StopBits = USART_STOPBITS_1,
              .Parity = USART_PARITY_NONE,
              .Mode = USART_MODE_TX_RX,
          },
  };
  HAL_USART_Init(&m_uh);
}
void HAL_USART_MspInit(USART_HandleTypeDef *husart) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (husart->Instance == USART1) {
    __HAL_RCC_GPIOA_CLK_ENABLE(); // Enable GPIO clock (USART1 usually on
                                  // PA9/PA10)

    // Configure TX Pin (usually PA9 for USART1)
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // Alternate Function Push-Pull
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure RX Pin (usually PA10 for USART1) - even if you're only
    // transmitting
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT; // Or GPIO_MODE_INPUT for RX
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

void SystemClock_Config(void);
void dump_registers();

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  HAL_Init();
  SystemClock_Config();

  usart1_init();
  LED_init();
  timer3_init(1e3, 1e3, 0.01);

  // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);

  while (1) {
    HAL_Delay(1000);
    HAL_USART_Transmit(&m_uh, (void *)"here ", 6, -1);
    dump_registers();
    // test without timer
    // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
  }

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  // #define __disable_irq(...) ((int)(0 && ""))
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

void dump_registers(void) {
  char buf[128];
  uint32_t ccer = TIM3->CCER;   // channel enable bits
  uint32_t ccmr1 = TIM3->CCMR1; // CH1/CH2 mode
  uint32_t ccr2 = TIM3->CCR2;   // compare value
  uint32_t cr1 = TIM3->CR1;
  uint32_t gpiob_crl = GPIOB->CRL; // PB0..PB7 config (PB5 in CRL)
  int n =
      snprintf(buf, sizeof(buf),
               "TIM3 CR1=0x%08lX CCMR1=0x%08lX CCER=0x%08lX CCR2=0x%08lX "
               "GPIOB_CRL=0x%08lX\r\n",
               (unsigned long)cr1, (unsigned long)ccmr1, (unsigned long)ccer,
               (unsigned long)ccr2, (unsigned long)gpiob_crl);
  HAL_USART_Transmit(&m_uh, (uint8_t *)buf, n, -1);

  n = snprintf(buf, sizeof(buf),
               "TIM3->ARR=0x%08lX TIM3->PSC=0x%08lX TIM3->CNT=0x%08lX\r\n",
               (unsigned long)TIM3->ARR, (unsigned long)TIM3->PSC,
               (unsigned long)TIM3->CNT);
  HAL_USART_Transmit(&m_uh, (uint8_t *)buf, n, -1);
}

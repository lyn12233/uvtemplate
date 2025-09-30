// local
#include "main.h"

// system
#include "stddef.h"
#include "stm32_hal_legacy.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_usart.h"

#ifndef __ARM_ACLE
#error "ACLE intrinsics support not enabled."
#endif

// 3rd party
#include "FreeRTOS.h"
#include "task.h"

// GPIO-led init
void LED_init() {
  GPIO_InitTypeDef gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_5, .Mode = GPIO_MODE_OUTPUT_PP, .Speed = GPIO_SPEED_LOW};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_Init(GPIOB, &gi);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

// timer-TIM4 init and start
TIM_HandleTypeDef m_th;
void timer4_init(int period, int psc) {
  __HAL_RCC_TIM4_CLK_ENABLE();

  TIM_Base_InitTypeDef tb =
      (TIM_Base_InitTypeDef){.Period = period,
                             .Prescaler = psc,
                             .ClockDivision = TIM_CLOCKDIVISION_DIV1,
                             .CounterMode = TIM_COUNTERMODE_UP};
  TIM_HandleTypeDef th = (TIM_HandleTypeDef){.Instance = TIM4, .Init = tb};
  HAL_TIM_Base_Init(&th);

  // start
  HAL_TIM_Base_Start(&th);

  m_th = th;
}

// gpio-uart1 init
USART_HandleTypeDef m_uh;
void printer_init() {
  __HAL_RCC_USART1_CLK_ENABLE();
  m_uh = (USART_HandleTypeDef){
      .Instance = USART1,
      .Init =
          (USART_InitTypeDef){
              .BaudRate = 9600,
              .WordLength = USART_WORDLENGTH_8B,
              .StopBits = USART_STOPBITS_1,
              .Parity = USART_PARITY_NONE,
              .Mode = USART_MODE_TX_RX,
          },
  };
  HAL_StatusTypeDef res = HAL_USART_Init(&m_uh);
  if (res != HAL_OK) {
    while (1) {
      // unreached
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    }
  }
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

//
//
void SystemClock_Config(void);

static void BlinkTask(void *p) {
  unsigned char i = 0;
  while (1) {
    HAL_USART_Transmit(&m_uh, (void *)"tasking\\n", 9, -1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    i++;
  }
}

//
// mainloop

int main(void) {
  int a = A + A + A;
  register int b __asm__("r0") = a;

  HAL_Init(); // HAL lib init

  SystemClock_Config(); // config sys clock

  //
  // initialize all configured peripherals
  LED_init();
  timer4_init(0xffff, 1e2);
  printer_init();

  //
  // freertos mainloop
  BaseType_t res = xTaskCreate(BlinkTask, "blinkTask", configMINIMAL_STACK_SIZE,
                               NULL, configMAX_PRIORITIES - 2, NULL);

  if (res != pdPASS) {
    while (1) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
      HAL_USART_Transmit(&m_uh, (void *)"taskcreate failed\\n", 20, -1);
    }
  } else {
    HAL_USART_Transmit(&m_uh, (void *)"taskcreate complete\\n", 22, -1);
  }
  vTaskStartScheduler();
  while (1) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_USART_Transmit(&m_uh, (void *)"task all done\\n", 9, -1);
  }

  while (1) {
    if (__HAL_TIM_GET_FLAG(&m_th, TIM_FLAG_UPDATE)) {
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
      __HAL_TIM_CLEAR_FLAG(&m_th, TIM_FLAG_UPDATE);
      const void *data = "here\\n";
      HAL_USART_Transmit(&m_uh, data, 6, -1);
    }
  }
  // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
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

#include "main.h"

#include "stm32_hal_legacy.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_gpio_ex.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_tim.h"

#ifndef __ARM_ACLE
#error "ACLE intrinsics support not enabled."
#endif

void LED_init() {
  GPIO_InitTypeDef gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_5, //
      .Mode = GPIO_MODE_AF_PP,
      .Speed = GPIO_SPEED_HIGH, //
      .Pull = GPIO_NOPULL,
  };

  __HAL_RCC_GPIOB_CLK_ENABLE(); // led0-pb5-timer3channel2

  HAL_GPIO_Init(GPIOB, &gi);
  // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

TIM_HandleTypeDef m_timer3h;
void timer3_init(int period, int psc, float duty) {
  TIM_Base_InitTypeDef timer_base = (TIM_Base_InitTypeDef){
      .Period = period - 1,
      .Prescaler = psc - 1,
      .ClockDivision = TIM_CLOCKDIVISION_DIV1,
      .CounterMode = TIM_COUNTERMODE_UP,
      .RepetitionCounter = 0,
  };
  m_timer3h = (TIM_HandleTypeDef){
      .Instance = TIM3,
      .Init = timer_base,
  };
  TIM_OC_InitTypeDef timer_compare = (TIM_OC_InitTypeDef){
      .OCMode = TIM_OCMODE_PWM1,
      .Pulse = (float)period * duty,
      .OCPolarity = TIM_OCPOLARITY_HIGH,
      .OCNPolarity = TIM_OCNPOLARITY_HIGH,
      .OCFastMode = TIM_OCFAST_DISABLE,
      .OCIdleState = TIM_OCIDLESTATE_RESET,
      .OCNIdleState = TIM_OCNIDLESTATE_RESET,
  };
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_TIM3_PARTIAL();
  HAL_TIM_Base_Init(&m_timer3h);
  HAL_TIM_PWM_Init(&m_timer3h);
  HAL_TIM_PWM_ConfigChannel(&m_timer3h, &timer_compare, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&m_timer3h, TIM_CHANNEL_2);
}

void SystemClock_Config(void);

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  HAL_Init();
  SystemClock_Config();

  LED_init();
  timer3_init(1e2, 1e2, 0.5);

  // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);

  while (1) {
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

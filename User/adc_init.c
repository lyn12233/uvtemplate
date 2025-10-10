
#include "stm32_hal_legacy.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_usart.h"

ADC_HandleTypeDef m_adch;

void adc_init() {
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);

  ADC_InitTypeDef adc_init = (ADC_InitTypeDef){
      .DataAlign = ADC_DATAALIGN_RIGHT,
      .ScanConvMode = ADC_SCAN_DISABLE,
      .ContinuousConvMode = DISABLE,
      .ExternalTrigConv = ADC_EXTERNALTRIGCONVEDGE_NONE,
  };
  m_adch = (ADC_HandleTypeDef){.Instance = ADC1, .Init = adc_init};

  HAL_ADC_Init(&m_adch);
  HAL_ADC_Start(&m_adch);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gi = (GPIO_InitTypeDef){
        .Pin = GPIO_PIN_1,
        .Mode = GPIO_MODE_ANALOG,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    HAL_GPIO_Init(GPIOA, &gi);
  }
}
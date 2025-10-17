#include "user_main.h"
#include "main.h"

#include "blink_task.h"
#include "initors.h"
#include "wiz_test.h"

// system
#include <stdio.h>

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

// 3rd party
#include "FreeRTOS.h"
#include "task.h"

#include "log.h"

// testing
int get_adc_value(int channel) {
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_0 + channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
  HAL_ADC_ConfigChannel(&m_adch, &sConfig);

  HAL_ADC_Start(&m_adch);
  HAL_ADC_PollForConversion(&m_adch, 10);
  int value = HAL_ADC_GetValue(&m_adch);
  // HAL_ADC_Stop(&m_adch);
  return value;
}

void SampleTask(void *p) {
  int value = 0;
  float voltage = 0.0;
  while (1) {
    value = get_adc_value(1);
    voltage = (value * 3.3) / 4096;
    printf("voltage: %f\r\n", voltage);
    vTaskDelay(1000);
  }
}

//
// mainloop

int user_main() {

  HAL_Init(); // HAL lib init

  SystemClock_Config(); // config sys clock

  //
  // initialize all configured peripherals
  usart1_init();
  adc_init();

  //
  // create task
  BaseType_t res = xTaskCreate(                                      //
      SampleTask, "SampleTask",                                      //
      configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, NULL //
  );

  //
  // freertos mainloop
  vTaskStartScheduler();

  while (1) {
    HAL_USART_Transmit(&m_uh, (void *)"task all done\\n", 9, -1);
    HAL_Delay(1000);
  }
}
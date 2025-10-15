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
#include "wizspi_init.h"

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

//
// mainloop

int user_main() {

  HAL_Init(); // HAL lib init

  SystemClock_Config(); // config sys clock

  //
  // initialize all configured peripherals
  LED_init();
  usart1_init();

  puts("[test printf]");
  printf("a string: %s\r\na char: %c\r\n", "HelloWorld", 'A');
  printf("a int: %d\r\na double: %lf\r\n", (int)12345, (double)6789);

  while (1) {
    // CHECK_FAIL(1 + 1 == 2);
    wizspi_test_mainloop();
    HAL_Delay(500);
  }

  int value = 0;
  float voltage = 0.0;
  adc_init();
  while (1) {
    HAL_Delay(1000);
    value = get_adc_value(1);
    voltage = (value * 3.3) / 4096;
    printf("voltage: %f\r\n", voltage);
  }

  //
  // create task
  BaseType_t res = xTaskCreate(BlinkTask, "blinkTask", configMINIMAL_STACK_SIZE,
                               NULL, configMAX_PRIORITIES - 2, NULL);
  if (res != pdPASS) {
    while (1) {
      HAL_USART_Transmit(&m_uh, (void *)"taskcreate failed\r\n", 20, -1);
      HAL_Delay(1000);
    }
  } else {
    HAL_USART_Transmit(&m_uh, (void *)"taskcreate complete\r\n", 22, -1);
  }

  //
  // freertos mainloop
  vTaskStartScheduler();
  while (1) {
    HAL_USART_Transmit(&m_uh, (void *)"task all done\\n", 9, -1);
    HAL_Delay(1000);
  }
}
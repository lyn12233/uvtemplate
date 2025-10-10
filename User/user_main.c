#include "user_main.h"
#include "main.h"

#include "blink_task.h"
#include "initors.h"

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

//
// mainloop

int user_main() {

  HAL_Init(); // HAL lib init

  SystemClock_Config(); // config sys clock

  //
  // initialize all configured peripherals
  LED_init();
  usart1_init();

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
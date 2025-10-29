#include "user_main.h"
#include "main.h"

#include "blink_task.h"
#include "portable.h"
#include "projdefs.h"
#include "tcp_echo_task.h"
#include "user_init/initors.h"
#include "wiz/wiz_test.h"
#include "wiz/wizspi_init.h"

// system
#include <stdio.h>

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_uart.h"

// 3rd party
#include "FreeRTOS.h"
#include "task.h"

#include "allocator.h"
#include "log.h"

//
// mainloop

int user_main() {

  user_init();

  // portMalloc() makes systick_handler not available under non task cxt

  //
  // freertos mainloop
  vTaskStartScheduler();
  while (1) {
    HAL_UART_Transmit(&m_uh, (void *)"task all done\\n", 9, -1);
    HAL_Delay(1000);
  }
}
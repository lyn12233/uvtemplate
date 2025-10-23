#include "main.h"
#include "user_init/initors.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_it.h"

#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"

#include "log.h"

/* Called if a stack overflow is detected. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  printf("Stack overflow in task: %s\n", pcTaskName);
  /* to halt execution safely */
  taskDISABLE_INTERRUPTS();
  /* log or blink an LED here */
  __BKPT(0);
  for (;;)
    ;
}

// from port.c
void xPortSysTickHandler();

int systick_count = 0;
// system clock interrupt handler hardwritten in vector table
void SysTick_Handler() {

  // logger
  static int cnt = 0;
  cnt = (cnt + 1) % 1000;
  systick_count++;
  if (!cnt) {
    LED_TOGGLE();
  }

  // HAL tick increment
  HAL_IncTick();

  // freertos tick management
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    xPortSysTickHandler();
  }
}

void vApplicationMallocFailedHook() { //
  uint32_t prim = __get_PRIMASK();
  while (1) {
    printf("vPortMallocFailed, pre=%x\r\n", prim);
    HAL_Delay(1000);
  }
}
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_it.h"

#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"


/* Called if a stack overflow is detected. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  /* to halt execution safely */
  taskDISABLE_INTERRUPTS();

  /* log or blink an LED here */
  // printf("Stack overflow in task: %s\n", pcTaskName);

  __BKPT(0);

  /* Stay here */
  for (;;)
    ;
}

// void vApplicationTickHook(){
//   static uint32_t ctr = 0;
//   if (++ctr >= 1000) {               // roughly 1 second if tick=1kHz
//     HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
//     ctr = 0;
//   }
// }

// from port.c
void xPortSysTickHandler();

void SysTick_Handler() {
  // HAL tick increment
  HAL_IncTick();

  // freertos tick management
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    xPortSysTickHandler();
  }
}
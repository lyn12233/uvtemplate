#include "FreeRTOS.h"
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

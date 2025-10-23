#include "blink_task.h"

#include "stm32f103xe.h"
#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "portable.h"
#include "task.h"

#include "log.h"

void BlinkTask(void *p) {
  while (1) {
    // puts("blink");
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
    pvPortMalloc(1);
    printf("free size: %d\r\n", (int)xPortGetFreeHeapSize());

    vTaskDelay(700); // fine
    // HAL_Delay(700); // fine
  }
}
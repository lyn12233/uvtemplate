#include "blink_task.h"

#include "stm32f103xe.h"
#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

void BlinkTask(void *p) {
  while (1) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
    vTaskDelay(500);
  }
}
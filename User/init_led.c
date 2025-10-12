#include "initors.h"

// GPIO-led init
void LED_init() {
  GPIO_InitTypeDef led0_init = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_5,
      .Mode = GPIO_MODE_OUTPUT_PP,
      .Speed = GPIO_SPEED_HIGH,
  };
  GPIO_InitTypeDef led1_init = led0_init;

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_Init(GPIOB, &led0_init); // pb5
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}
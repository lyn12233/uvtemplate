#pragma once

#include "stm32f103xe.h"
#include "stm32f1xx_hal_usart.h"

void LED_init();
extern USART_HandleTypeDef m_uh;
void usart1_init();
void HAL_USART_MspInit(USART_HandleTypeDef *husart);
extern ADC_HandleTypeDef m_adch;
void adc_init();

#define LED_TOGGLE() HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5)

extern int systick_count; // implemented at _hooks.c
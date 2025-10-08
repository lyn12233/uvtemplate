#pragma once

#include "stm32f103xe.h"
#include "stm32f1xx_hal_usart.h"

void LED_init();
extern USART_HandleTypeDef m_uh;
void usart1_init();
void HAL_USART_MspInit(USART_HandleTypeDef *husart);
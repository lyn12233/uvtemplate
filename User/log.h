///@brief overrides printf from microlib
// #pragma once // to override

#include "initors.h"
#include "stm32f1xx_hal_usart.h"

#include <stdio.h>
#include <string.h>

static int printf_v2(const char *s, ...) { //
  HAL_USART_Transmit(&m_uh, (void *)s, strlen(s), -1);
  return 0;
}
static int puts_v2(const char *s) { //
  HAL_USART_Transmit(&m_uh, (void *)s, strlen(s), -1);
  return 0;
}

#define printf printf_v2
#define puts puts_v2
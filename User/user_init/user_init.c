///@file user_init.c
#include "initors.h"
#include "main.h" //for system clock config

#include "stm32f1xx_hal.h"
#include "user_init/initors.h"

#include "log.h"

#ifdef USE_ESP8266_NETWORK

void user_init() {
  HAL_Init();
  SystemClock_Config();
  LED_init();
  usart1_init();
  // -- logging enabled --
  puts("==========logging-enaled==========");
  usart3_init();
  puts("===user-defined-initialization-done===");
  puts("=user-deinfed-backgrounds-all-started=");
}

#else
#error "unimpl"
#endif
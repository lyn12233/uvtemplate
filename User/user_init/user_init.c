///@file user_init.c
#include "FreeRTOSConfig.h"
#include "initors.h"

#include "esp/exec.h"
#include "esp/parser.h"
#include "main.h" //for system clock config


#include "stm32f1xx_hal.h"

#include "log.h"

#ifdef USE_ESP8266_NETWORK

static void parser_task(void *p) {
  (void)p;
  assert(atc_parser_init_done);
  atc_parser_loop();
}
static void exec_task(void *p) {
  (void)p;
  assert(atc_exec_init_done);
  atc_exec_loop();
}

void user_init() {
  HAL_Init();
  SystemClock_Config();
  LED_init();
  usart1_init();
  // -- logging enabled --
  puts("==========logging-enaled==========");
  usart3_init();

  atc_parser_init();
  xTaskCreate(                                   //
      parser_task, "at command parser",          //
      1024, NULL, configMAX_PRIORITIES - 2, NULL //
  );

  atc_exec_init();
  xTaskCreate(                                   //
      exec_task, "at command executor",          //
      1024, NULL, configMAX_PRIORITIES - 2, NULL //
  );

  puts("===user-defined-initialization-done===");
  puts("=user-deinfed-backgrounds-all-started=");
}

#else
#error "unimpl"
#endif
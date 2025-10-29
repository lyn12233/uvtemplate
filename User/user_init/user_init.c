///@file user_init.c
#include "FreeRTOSConfig.h"
#include "initors.h"

#include "blink_task.h"
#include "tcp_echo_task.h"

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
  debug("exec_task: enter\r\n");
  atc_exec_loop();
}

void user_init() {
  HAL_Init();
  SystemClock_Config();
  LED_init();
  uart1_init();
  // -- logging enabled --
  puts("==========logging-enaled==========");
  uart3_init();
  puts("uart3 init done");
  // sdio_init();
  // puts("sdio init done");

  atc_parser_init();
  puts("parser init done");
  xTaskCreate(                                   //
      parser_task, "at command parser",          //
      1024, NULL, configMAX_PRIORITIES - 2, NULL //
  );

  atc_exec_init();
  puts("exec init done");
  xTaskCreate(                                   //
      exec_task, "at command executor",          //
      1024, NULL, configMAX_PRIORITIES - 2, NULL //
  );

  puts("===user-defined-initialization-done===");
  puts("=user-deinfed-backgrounds-all-started=");

  // task
  //
  // create task
  BaseType_t res;

  res = xTaskCreate(BlinkTask, "blinkTask", configMINIMAL_STACK_SIZE, NULL,
                    configMAX_PRIORITIES - 2, NULL);
  assert(res == pdPASS);
  // res = xTaskCreate(tcp_echo_task, "tcpEchoTask", 512, NULL,
  //                   configMAX_PRIORITIES - 2, NULL);
  // assert(res == pdPASS);
}

#else
#error "unimpl"
#endif
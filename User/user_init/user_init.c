///@file user_init.c
#include "FreeRTOSConfig.h"
#include "initors.h"

#include "blink_task.h"
#include "projdefs.h"
#include "tcp_sshd_task.h"

#include "esp/exec.h"
#include "esp/parser.h"
#include "main.h" //for system clock config

#include "stm32f1xx_hal.h"

#include "ff.h"

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

FATFS m_fso = {0};
uint8_t fs_init_done = 0;

static void sdio_init_task(void *p) {
  (void)p;
loop:
  puts("try init sdio");
  if (sdio_init()) {
    puts("sdio init success");
  } else {
    puts("sdio init retry");
    vTaskDelay(pdMS_TO_TICKS(1000));
    goto loop;
  }
loop2:
  puts("filesystem: mounting");
  FRESULT fr = f_mount(&m_fso, "", 1);
  if (fr == FR_OK) {
    puts("filesystem: mounted");
    fs_init_done = 1;
  } else {
    printf("filesystem: mount error: %d\r\n", fr);
    puts("filesystem: mount retry...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    goto loop;
  }

  vTaskDelete(NULL);
};

void user_init() {
  HAL_Init();
  SystemClock_Config();
  LED_init();
  uart1_init();
  // -- logging enabled --
  puts("==========logging-enaled==========");
  uart3_init();
  puts("uart3 init done");

  // postpone init sdio
  xTaskCreate(sdio_init_task, "", 128, NULL, configMAX_PRIORITIES - 2, NULL);

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

  res = xTaskCreate(BlinkTask, "blinkTask", 128, NULL, configMAX_PRIORITIES - 2,
                    NULL);
  assert(res == pdPASS);
  res = xTaskCreate(tcp_sshd_task, "tcpSSHDTask", 1024 * 4, NULL,
                    configMAX_PRIORITIES - 1, NULL);
  assert(res == pdPASS);
}

#else
#error "unimpl"
#endif
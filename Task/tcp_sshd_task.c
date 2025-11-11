#include "tcp_sshd_task.h"
#include "esp/espsock.h"
#include "esp/exec.h"
#include "esp/parser.h"
#include "tcp_echo_task.h"
#include "types/vo.h"
#include "user_init/initors.h"

#include "portmacro.h"
#include "projdefs.h"

#include "stm32f1xx_hal_uart.h"

#include <stdint.h>

#include "log.h"

void tcp_sshd_task(void *params) {
  atc_cmd_t cmd;
  QueueHandle_t exec_res = xQueueCreate(10, sizeof(int));
  assert(exec_res);
  int res;
  BaseType_t pass;
  int cnt = 0;
  atc_cmd_type_t options[4] = {atc_start, atc_cwmode, atc_cipmux,
                               atc_cipserver};

  int lstnfd;
  while (1) {
    lstnfd = sock_init();
    if (lstnfd < 0) {
      printf("sockinit failed with %d, retry\r\n", lstnfd);
      vTaskDelay(pdMS_TO_TICKS(200));
      puts("retrying...");
    } else {
      break;
    }
  }
  vTaskDelay(pdMS_TO_TICKS(500));
  puts("===================sock init done===================\r\n");

  while (1) {
    int acpt = -1;
    while (acpt < 0) {
      acpt = sock_accept(lstnfd, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("socket accepted: %d, running ssh session\r\n", acpt);

    uint8_t res = sshd_acpt(acpt);

    printf("sshd service ends with status %d\r\n", res);

    res = sock_close(acpt);
    printf("acpt close (%d)\r\n", res);
  }
}
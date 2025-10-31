#include "tcp_echo_task.h"
#include "esp/espsock.h"
#include "esp/exec.h"
#include "esp/parser.h"

#include <stdint.h>

#include "portmacro.h"
#include "projdefs.h"

#include "log.h"
#include "stm32f1xx_hal_uart.h"
#include "types/vo.h"
#include "user_init/initors.h"

void tcp_echo_task(void *params) {
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

  // while (1) {
  //   int acpt = sock_accept(lstnfd, 0);
  //   vTaskDelay(pdMS_TO_TICKS(100));
  //   printf("socket accepted: %d\r\n", acpt);
  // }
  int acpt = -1;
  while (acpt < 0) {
    acpt = sock_accept(lstnfd, 0);
  }
  vTaskDelay(pdMS_TO_TICKS(100));
  printf("socket accepted: %d\r\n", acpt);

  vstr_t *buff = vstr_create(0);
  vstr_t *tmp = vstr_create(0);
  while (1) {
    int res = sock_recv(acpt, buff, 1, 0);
    if (res >= 1) {
      vbuff_iadd(tmp, buff->data, 1);
      // printf("recv err: %d\r\n", res);
    }

    if (buff->data[0] == '\n') {
      buff->data[0] = 0;
      vTaskDelay(pdMS_TO_TICKS(100));
      HAL_UART_Transmit(&m_uh, (void *)tmp->data, tmp->len, -1);
      puts("");

      puts("echoing");
      sock_send(acpt, tmp, tmp->len, 0);

      // cleanup
      tmp->len = 0;
    }
  }
}
#include "tcp_echo_task.h"
#include "esp/espsock.h"
#include "esp/exec.h"
#include "esp/parser.h"


#include <stdint.h>

#include "portmacro.h"
#include "projdefs.h"


#include "log.h"

void tcp_echo_task(void *params) {
  atc_cmd_t cmd;
  QueueHandle_t exec_res = xQueueCreate(10, sizeof(int));
  assert(exec_res);
  int res;
  BaseType_t pass;
  int cnt = 0;
  atc_cmd_type_t options[4] = {atc_start, atc_cwmode, atc_cipmux,
                               atc_cipserver};

  puts("tcp_echo_task: enter\r\n");
  while (1) {

    debug("=======task: exec %d=======\r\n", cnt);

    cnt = (cnt + 1) % 4;
    cmd.type = options[cnt];
    cmd.exec_res = exec_res;
    atc_exec(&cmd);

    vTaskDelay(pdMS_TO_TICKS(1500));

    res = -1;
    pass = xQueueReceive(exec_res, &res, portMAX_DELAY);
    xQueueReset(exec_res);

    debug("=======task: result %d (recv:%d)\r\n", res, (int)pass);

    vTaskDelay(pdMS_TO_TICKS(1500));
  }
  //   int sockfd = sock_init();
  //   printf("tcp_echo_task: sock_init=%d\r\n", sockfd);
  //   assert(sockfd == LSTN_SK_FD);

  //   int res = sock_listen(sockfd);
  //   printf("tcp_echo_task: sock_listen %d\r\n", res);
  //   assert(res == 0);

  //   int connfd = sock_accept(sockfd, 0);
  //   printf("tcp_echo_task: sock_accept=%d\r\n", connfd);
  //   assert(connfd >= 0);

  //   uint8_t recvbyte;
  //   while (1) {
  //     int rcvd = sock_recv(connfd, (vstr_t *)&recvbyte, 1, 0);
  //     if (rcvd > 0) {
  //       sock_send(connfd, (const vstr_t *)&recvbyte, 1, 0);
  //     } else {
  //       printf("recv error %d\r\n", rcvd);
  //       vTaskDelay(pdMS_TO_TICKS(1000));
  //     }
  //   }
}
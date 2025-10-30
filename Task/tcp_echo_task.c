#include "tcp_echo_task.h"

#include "esp/espsock.h"
#include "esp/exec.h"

#include <stdint.h>

#include "log.h"
#include "projdefs.h"

void tcp_echo_task(void *params) {
  atc_cmd_t cmd;
  puts("tcp_echo_task: enter\r\n");
  while (1) {
    debug("tcp_echo_task: exec 'at'\r\n");
    cmd.type = atc_start;
    cmd.exec_res = NULL;
    atc_exec(&cmd);
    vTaskDelay(pdMS_TO_TICKS(3000));
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
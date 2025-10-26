#include "espsock.h"

#include "exec.h"
#include "parser.h"
#include "types/vo.h"

#include <stdint.h>

#include "log.h"
#include "port_errno.h"

volatile uint8_t esk_init_done = 0;
QueueHandle_t esk_lstn_res;
QueueHandle_t esk_conn_res[NB_SOCK];
vstr_t *esk_recv_buff[NB_SOCK];

static atc_cmd_type_t init_cmds[] = {
    atc_start,  //
    atc_cwmode, //
    atc_cipmux, //
};

static int is_sock_conn(int id) {
  if (id < 0 || id > 4)
    return 0;
  if (conn_state[id])
    return 1;
  if (uxQueueMessagesWaiting(conn_recv[id]) > 0)
    return 1;
  return 0;
}

int sock_init() {
  if (esk_init_done)
    return LSTN_SK_FD;

  esk_lstn_res = xQueueCreate(20, sizeof(int));
  assert(esk_lstn_res);
  for (int i = 0; i < NB_SOCK; i++) {
    esk_conn_res[i] = xQueueCreate(20, sizeof(int));
    assert(esk_conn_res[i]);
    esk_recv_buff[i] = vstr_create(512);
    assert(esk_recv_buff[i]);
  }

  // TODO: AT, RST, CWMODE, CWJAP, CIPMUX
  // TODO: wifi loging in pppoe
  atc_cmd_t cmd;
  int res;

  cmd.exec_res = esk_lstn_res;

  for (int i = 0; i < 3; i++) {
    cmd.type = init_cmds[i];
    atc_exec(&cmd);
    res = xQueueReceive(esk_lstn_res, &res, portMAX_DELAY);
    if (res < 0)
      return res;
  }

  esk_init_done = 1;
  return LSTN_SK_FD;
}

int sock_listen(int sockfd) {
  if (sockfd != LSTN_SK_FD)
    return EBADF;

  atc_cmd_t cmd;
  int res;

  cmd.type = atc_cipserver;
  cmd.exec_res = esk_lstn_res;
  atc_exec(&cmd);
  res = xQueueReceive(esk_lstn_res, &res, portMAX_DELAY);
  if (res < 0)
    return res;

  return 0;
}

int sock_accept(int sockfd, int flags) {
  if (sockfd != LSTN_SK_FD)
    return EBADF;

  int conn_fd;
  BaseType_t pass =
      xQueueReceive(esk_lstn_res, &conn_fd, flags ? 0 : portMAX_DELAY);
  if (pass != pdTRUE) {
    return flags ? EAGAIN : ENOTCONN;
  }

  return conn_fd;
}

int sock_send(int sockfd, const vstr_t *buff, uint16_t size, int flags) {
  if (sockfd < 0 || sockfd > 4)
    return EBADF;
  if (!is_sock_conn(sockfd))
    return ENOTCONN;
  if (size > buff->len)
    size = buff->len;
  if (size == 0)
    return 0;

  int senderr;
  atc_cmd_t cmd;
  cmd.type = atc_cipsend;
  cmd.id = (uint8_t)sockfd;
  cmd.len = (uint16_t)size;
  cmd.buff = buff->data;
  cmd.exec_res = esk_conn_res[sockfd];
  atc_exec(&cmd);

  BaseType_t pass =
      xQueueReceive(esk_conn_res[sockfd], &senderr, flags ? 0 : portMAX_DELAY);
  if (pass != pdTRUE) {
    return flags ? EAGAIN : ENOTCONN;
  }
  return senderr;
}

int sock_recv(int sockfd, vstr_t *buff, size_t size, int flags) {
  if (sockfd < 0 || sockfd > 4)
    return EBADF;
  if (!is_sock_conn(sockfd))
    return ENOTCONN;
  if (size > buff->len)
    size = buff->len;
  if (size == 0)
    return 0;

  buff->len = 0;

  while (1) {
    uint32_t recv_len = esk_recv_buff[sockfd]->len;
    if (size <= recv_len) {
      vstr_t *tmp = vstr_create(0);
      vbuff_iadd(tmp, &esk_recv_buff[sockfd]->data[size], recv_len - size);
      vbuff_iadd(buff, esk_recv_buff[sockfd]->data, size);

      vstr_delete(esk_recv_buff[sockfd]);
      esk_recv_buff[sockfd] = tmp;
      break;
    } else {
      vbuff_iadd(buff, esk_recv_buff[sockfd]->data, recv_len);
      size -= recv_len;

      atc_msg_t msg;
      xQueueReceive(conn_recv[sockfd], &msg, portMAX_DELAY);
      assert(msg.type == atc_conn_recv);

      vstr_t *tmp = msg.pdata;
      vbuff_iadd(esk_recv_buff[sockfd], tmp->data, tmp->len);
      vstr_delete(tmp);

      // undone
    }
  }
  return buff->len;
}
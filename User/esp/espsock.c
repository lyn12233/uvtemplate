#include "espsock.h"

#include "exec.h"
#include "parser.h"
#include "portmacro.h"
#include "projdefs.h"
#include "types/vo.h"

#include <stdint.h>

#include "log.h"
#include "port_errno.h"

volatile uint8_t esk_init_done = 0;
QueueHandle_t esk_lstn_res;
QueueHandle_t esk_conn_res[NB_SOCK];
vstr_t *esk_recv_buff[NB_SOCK];

static atc_cmd_type_t init_cmds[] = {
    atc_start,     //
    atc_cwmode,    //
    atc_cipmux,    //
    atc_cipserver, //
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
  debug("sock_init: enter\r\n");
  if (esk_init_done)
    return LSTN_SK_FD;

  debug("sock_init: starting\r\n");
  esk_lstn_res = xQueueCreate(1, sizeof(int));
  assert(esk_lstn_res);
  debug("sock_init: lstn res queue created\r\n");

  for (int i = 0; i < NB_SOCK; i++) {
    esk_conn_res[i] = xQueueCreate(1, sizeof(int));
    assert(esk_conn_res[i]);
    esk_recv_buff[i] = vstr_create(512);
    assert(esk_recv_buff[i]);
  }
  debug("sock_init: conn res queues and recv buffs created\r\n");

  // TODO: AT, RST, CWMODE, CWJAP, CIPMUX (mode, jap are stored, mux not)
  // TODO: wifi loging in pppoe (not supported)
  atc_cmd_t cmd;
  int res;

  cmd.exec_res = esk_lstn_res;

  for (int i = 0; i < sizeof(init_cmds) / sizeof(atc_cmd_type_t); i++) {
    cmd.type = init_cmds[i];
    debug("sock_init: init command: %d\r\n", (int)init_cmds[i]);
    atc_exec(&cmd);
    xQueueReceive(esk_lstn_res, &res, portMAX_DELAY);
    if (res < 0)
      return res;
  }

  esk_init_done = 1;
  return LSTN_SK_FD;
}

int sock_listen(int sockfd) {
  if (sockfd != LSTN_SK_FD)
    return EBADF;

  return 0;
}

int sock_accept(int sockfd, int flags) {
  if (sockfd != LSTN_SK_FD)
    return EBADF;

  int conn_fd = 0;
  BaseType_t pass =
      xQueueReceive(conn_preaccepted, &conn_fd, flags ? 0 : portMAX_DELAY);
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
  cmd.buff = (uint8_t *)buff->data;
  cmd.exec_res = esk_conn_res[sockfd];
  debug("sock_send: sending %d bytes, p:%x\r\n", size,
        (int)(uint64_t)((void *)buff->data));
  atc_exec(&cmd);
  debug("sock_send: exec done\r\n");

  BaseType_t pass =
      xQueueReceive(esk_conn_res[sockfd], &senderr, flags ? 0 : portMAX_DELAY);
  debug("sock_send: recv res done\r\n");
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
  // if (size > buff->len)
  //   size = buff->len;
  if (flags)
    return EINVAL;
  if (size == 0)
    return 0;

  debug("sock_recv: start\r\n");
  // trivial clear, no calls to free(&buff->data)
  buff->len = 0;

  while (1) {
    // size: remaining required size to be put into buff
    // esk_recv_buff: (vstr*)[8], pre-received data popped from queue

    uint32_t recv_len = esk_recv_buff[sockfd]->len;
    debug("sock_recv: checking pre-recv: remaining-req:%d - pre:%d\r\n", size,
          recv_len);

    if (size <= recv_len) {
      // recv enough, head to buff, tail remain
      vstr_t *tmp = vstr_create(0);
      vbuff_iadd(tmp, &esk_recv_buff[sockfd]->data[size], recv_len - size);
      vbuff_iadd(buff, esk_recv_buff[sockfd]->data, size);

      vstr_delete(esk_recv_buff[sockfd]);
      esk_recv_buff[sockfd] = tmp;

      debug("sock_recv: done. buff:%d, pre:%d\r\n", buff->len,
            esk_recv_buff[sockfd]->len);
      break;

    } else {
      // recv not enough, buff <- pre-recv <- queue
      vbuff_iadd(buff, esk_recv_buff[sockfd]->data, recv_len);
      size -= recv_len;
      debug("sock_recv: buff:%d still need %d (-%d)\r\n", buff->len, size,
            recv_len);

      atc_msg_t msg;
      BaseType_t pass;
      pass = xQueueReceive(conn_recv[sockfd], &msg, portMAX_DELAY);
      if (pass == pdFALSE)
        return EAGAIN;
      assert(msg.type == atc_conn_recv);

      // truncate pre-recv buff
      vstr_t *tmp = msg.pdata;
      esk_recv_buff[sockfd]->len = 0;
      vbuff_iadd(esk_recv_buff[sockfd], tmp->data, tmp->len);
      vstr_delete(tmp);
      debug("sock_recv: connection queue popped %d\r\n",
            esk_recv_buff[sockfd]->len);

      // undone
    }
  }
  return buff->len;
}
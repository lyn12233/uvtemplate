#include "espsock.h"

#include "exec.h"

#include "log.h"
#include "port_errno.h"

volatile uint8_t esk_init_done = 0;
QueueHandle_t esk_lstn_res;

#define RETURN_FAIL(exp)                                                       \
  {                                                                            \
    if (exp < 0) {                                                             \
      return exp;                                                              \
    }                                                                          \
  }

static atc_cmd_type_t init_cmds[] = {
    atc_start, atc_reset, atc_cwmode, atc_cwjap, atc_cipmux,
};

int sock_init() {
  if (esk_init_done)
    return LSTN_SK_FD;

  esk_lstn_res = xQueueCreate(20, sizeof(int));
  assert(esk_lstn_res);

  // TODO: AT, RST, CWMODE, CWJAP, CIPMUX
  // TODO: wifi loging in pppoe
  atc_cmd_t cmd;
  int res;

  cmd.exec_res = esk_lstn_res;
  for (int i = 0; i < 5; i++) {
    cmd.type = init_cmds[i];
    atc_exec(&cmd);
    res = xQueueReceive(esk_lstn_res, &res, portMAX_DELAY);
    RETURN_FAIL(res);
  }

  esk_init_done = 1;
  return LSTN_SK_FD;
}

int sock_listen(int sockfd) {}

int sock_accept(int sockfd, int flags);

int sock_send(int sockfd, const vstr_t *buff, int flags);

int sock_recv(int sockfd, vstr_t *buff, size_t size, int flags);
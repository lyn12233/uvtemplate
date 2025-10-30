#include "exec.h"

#include "parser.h" // for ATC_SENDRES_TIMEOUT
#include "projdefs.h"
#include "stm32f1xx_hal_def.h"
#include "types/vo.h"

#include <stdint.h>

#include "portmacro.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"

#include "log.h"
#include "port_errno.h"
#include "user_init/initors.h"

void atc_send(const void *buff, uint32_t bufflen) {
  HAL_StatusTypeDef res;
  debug("atc_send: sending %lu bytes\r\n", bufflen);

  res = HAL_UART_Transmit_IT(&m_u3h, buff, bufflen);

  if (res != HAL_OK) {
    printf("atc_send: HAL_UART_Transmit error %d\r\n", res);
    // assert(res == HAL_OK);
    return;
  }
  if (atc_parser_init_done)
    xSemaphoreTake(m_esp8266_senddone, pdMS_TO_TICKS(50));

  debug("atc_send: sent\r\n");
}

static uint16_t str_valid_len(const vstr_t *str) {
  uint16_t res;
  for (res = 0; res < str->len; res++) {
    const char c = str->data[res];
    if (c == '\r' || c == '\n' || c == 0 || res > 255) {
      return res;
    }
  }
  return res;
}

static atc_msg_type_t get_sendres() {
  atc_msg_type_t res;
  BaseType_t pass = pdTRUE;
  if (atc_sendres)
    pass = xQueueReceive(atc_sendres, &res, ATC_SENDRES_TIMEOUT);
  return pass == pdTRUE ? res : atc_unknown;
}

void atc_exec(const atc_cmd_t *cmd) {
  if (!atc_parser_init_done) {
    debug("atc_exec: parser not initialized\r\n");
    return;
  }

  uint16_t len_ssid, len_pwd;
  const vstr_t *s_ssid, *s_pwd;
  buff20_t ibuff;

  atc_msg_type_t res = atc_ok, res2 = atc_send_ok;

  switch (cmd->type) {
  case atc_start:
    debug("exec: atc_start\r\n");
    atc_send("AT\r\n", 4);
    break;
  case atc_reset:
    debug("exec: atc_reset\r\n");
    atc_send("AT+RST\r\n", 8);
    break;
  case atc_cwmode:
    debug("exec: atc_cwmode\r\n");
    atc_send("AT+CWMODE=2\r\n", 13);
    break;
  case atc_cwjap:
    s_ssid = cmd->s_ssid, s_pwd = cmd->s_pwd;
    len_ssid = str_valid_len(s_ssid), len_pwd = str_valid_len(s_pwd);
    atc_send("AT+CWJAP=", 9);
    atc_send(s_ssid->data, len_ssid);
    atc_send(",", 1);
    atc_send(s_pwd->data, len_pwd);
    atc_send("\r\n", 2);
    break;
  case atc_cipmux:
    debug("exec: atc_cipmux\r\n");
    atc_send("AT+CIPMUX=1\r\n", 13);
    break;
  case atc_cipserver:
    debug("exec: atc_cipserver\r\n");
    atc_send("AT+CIPSERVER=1,8080\r\n", 21);
    break;

  case atc_cipsend: {
    int offs = 0, cur_len;

    while (offs < cmd->len) {
      cur_len = offs + 1024 < cmd->len ? 1024 : (cmd->len - offs);

      atc_send("AT+CIPSEND=", 11);
      ibuff = itoa(cmd->id, 10);
      atc_send(ibuff.str, strnlen(ibuff.str, 20));
      atc_send(",", 1);
      ibuff = itoa(cur_len, 10);
      atc_send(ibuff.str, strnlen(ibuff.str, 20));
      atc_send("\r\n", 2);

      // wait wonna state conditionally, force send upon failure
      res = get_sendres();
      if (res == atc_ok && atc_wonna) {
        xSemaphoreTake(atc_wonna, portMAX_DELAY);
      }
      atc_send(&cmd->buff[offs], cur_len);

      if (res != atc_ok)
        break;

      offs += cur_len;
    }

  } break;

  case atc_cipclose:
    atc_send("AT+CIPCLOSE=", 12);
    ibuff = itoa(cmd->id, 10);
    atc_send(ibuff.str, strnlen(ibuff.str, 20));
    atc_send("\r\n", 2);
    break;
  default:
    assert(0);
  } // switch

  if (cmd->type != atc_cipsend) {
    res = get_sendres();
  } else if (res == atc_ok) {
    res2 = get_sendres();
  }
  debug("exec: cmd result: %d\r\n", res);

  // report queue exists, report error; also reset(?)
  int out_msg = 0;
  if (res != atc_ok || cmd->type == atc_cipsend && res2 != atc_send_ok) {
    // error result
    if (cmd->exec_res) {
      out_msg = ENOTCONN;
      xQueueSend(cmd->exec_res, &out_msg, 0);
    }

  } else {
    if (cmd->exec_res) {
      xQueueSend(cmd->exec_res, &out_msg, 0);
    }
  }

  // clear send result queue
  xQueueReset(atc_sendres);
  debug("exec: cmd exec done\r\n");
}

QueueHandle_t atc_cmd_in;
volatile uint8_t atc_exec_init_done = 0;

void atc_exec_init() {
  if (atc_exec_init_done)
    return;

  // data fields are reference
  atc_cmd_in = xQueueCreate(20, sizeof(atc_cmd_t));
  assert(atc_cmd_in);

  atc_exec_init_done = 1;
}
void atc_exec_loop() {
  debug("atc_exec_loop: enter\r\n");
  while (1) {
    if (!atc_cmd_in) {
      debug("atc_exec_loop: waiting for atc_cmd_in\r\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    atc_cmd_t cmd;
    debug("exec: waiting for cmd\r\n");
    BaseType_t pass = xQueueReceive(atc_cmd_in, &cmd, portMAX_DELAY);
    if (pass == pdTRUE) {
      atc_exec(&cmd);
    }
  }
}
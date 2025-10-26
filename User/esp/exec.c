#include "exec.h"

#include "types/vo.h"
#include "user_init/initors.h"

#include <stdint.h>

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_usart.h"

#include "log.h"

void atc_send(const void *buff, uint32_t bufflen) {
  if (bufflen > 100) {
    HAL_USART_Transmit_IT(&m_u3h, buff, bufflen);
    xSemaphoreTake(m_esp8266_senddone, portMAX_DELAY);
  } else {
    HAL_USART_Transmit(&m_u3h, buff, bufflen, -1);
  }
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

void atc_exec(const atc_cmd_t *cmd) {
  uint16_t len_ssid, len_pwd;
  const vstr_t *s_ssid, *s_pwd;
  buff20_t ibuff;

  switch (cmd->type) {
  case atc_start:
    atc_send("AT\r\n", 4);
    break;
  case atc_reset:
    atc_send("AT+RST\r\n", 8);
    break;
  case atc_cwmode:
    atc_send("AT+CWMODE=1\r\n", 13);
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
    atc_send("AT+CIPMUX=1\r\n", 13);
    break;
  case atc_cipserver:
    atc_send("AT+CIPSERVER=1,8080\r\n", 21);
    break;
  case atc_cipsend:
    atc_send("AT+CIPSEND=", 11);
    ibuff = itoa(cmd->id, 10);
    atc_send(ibuff.str, strnlen(ibuff.str, 20));
    atc_send(",", 1);
    ibuff = itoa(cmd->len, 10);
    atc_send(ibuff.str, strnlen(ibuff.str, 20));
    atc_send("\r\n", 2);
    break;
  case atc_cipclose:
    atc_send("AT+CIPCLOSE=", 12);
    ibuff = itoa(cmd->id, 10);
    atc_send(ibuff.str, strnlen(ibuff.str, 20));
    atc_send("\r\n", 2);
    break;
  default:
    assert(0);
  }
}

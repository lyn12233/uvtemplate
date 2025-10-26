///@file User/esp/exec.h
#pragma once

#include "types/vo.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <stdint.h>

extern USART_HandleTypeDef m_u3h;

extern SemaphoreHandle_t m_esp8266_senddone;
extern SemaphoreHandle_t atc_wonna; // impl in parser

typedef enum {
  atc_start,     // AT
  atc_reset,     // AT+RST
  atc_cwmode,    // AT+CWMODE=1
  atc_cwjap,     // AT+CWJAP=ssid:str,pwd:str
  atc_cipmux,    // AT+CIPMUX=1
  atc_cipserver, // AT+CIPSERVER=1,8080
  atc_cipsend,   // AT+CIPSEND=id:u8,len:u16 then wait >, or many times
  atc_cipclose,  // AT+CIPCLOSE=id:u8
} atc_cmd_type_t;

typedef struct {
  atc_cmd_type_t type;
  union {
    struct {
      uint8_t id;
      uint16_t len;
    };
    struct {
      const vstr_t *s_ssid;
      const vstr_t *s_pwd; // pass by reference
    };
  };
} atc_cmd_t;

void atc_send(const void *buff, uint32_t bufflen);
void atc_exec(const atc_cmd_t *cmd);

// tcp req ser/deser scheme: qin {result_q, cmd} result to result_q

// int atc_sock_init();
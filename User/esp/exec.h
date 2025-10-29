///@file User/esp/exec.h
#pragma once

#include "parser.h"
#include "types/vo.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include <stdint.h>

extern UART_HandleTypeDef m_u3h;

extern SemaphoreHandle_t atc_cansend;
extern SemaphoreHandle_t atc_wonna; // impl in parser

typedef enum {
  atc_start,     // AT
  atc_reset,     // AT+RST
  atc_cwmode,    // AT+CWMODE=3 (station+softap)
  atc_cwjap,     // AT+CWJAP=ssid:str,pwd:str
  atc_cipmux,    // AT+CIPMUX=1
  atc_cipserver, // AT+CIPSERVER=1,8080
  atc_cipsend,   // AT+CIPSEND=id:u8,len:u16 then wait >, or many times
  atc_cipclose,  // AT+CIPCLOSE=id:u8
} atc_cmd_type_t;

// note: for cwjap, ssid/pwd are pass by reference; for cipsend, buff, id, len
// is pass by reference; for cipclose, id is pass by reference; len is not
// restricted by hardware, exec splits it if needed

typedef struct {
  atc_cmd_type_t type;
  union {
    struct {
      uint8_t id;
      uint16_t len;
      const void *buff;
    };
    struct {
      const vstr_t *s_ssid;
      const vstr_t *s_pwd; // pass by reference
    };
  };
  QueueHandle_t exec_res;
} atc_cmd_t;

void atc_send(const void *buff, uint32_t bufflen);
void atc_exec(const atc_cmd_t *cmd);

extern QueueHandle_t atc_cmd_in;
extern volatile uint8_t atc_exec_init_done;
void atc_exec_init();

void atc_exec_loop();

// tcp req ser/deser scheme: qin {result_q, cmd} result to result_q

// int atc_sock_init();
///@file parser.h
///@brief esp8266 AT command response parser
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include <stdint.h>

// number of sock entries, larger that physically supported
#define NB_SOCK 8
// sendres timeout
#define ATC_SENDRES_TIMEOUT 20

typedef enum {
  atc_unknown,
  // response, always follows specific cmd
  atc_ok,
  atc_error,
  atc_send_ok,
  atc_send_fail,
  // report
  atc_ready, // ready
  atc_busy,  // busy
  atc_wifi_connected,
  atc_wifi_got_ip,
  atc_wifi_disconnect,
  atc_conn_accepted, // x,CONNECT
  atc_conn_closed,   // x,CLOSED
  atc_conn_recv,     //+IPD<id>,<len>:<data>, where id<5 and len<=2048

  // parser
  atc_parse_error,
  atc_inval,
} atc_msg_type_t;

typedef struct {
  atc_msg_type_t type;
  union {
    struct {
      uint8_t id;
      uint16_t len;
    };
    void *pdata;
    const char *msg;
    struct {
      uint16_t buff_len;
      uint8_t *pbuff;
    };
  };
} atc_msg_t;

// bkgd parsing utils

void atc_parser_clear();
uint8_t atc_parse_char(uint8_t c);
void atc_dispatch(atc_msg_t *msg);

// rtos-level api's leading to socket api
void atc_parser_loop();
void atc_parser_init();
extern volatile uint8_t atc_parser_init_done;
extern volatile uint8_t conn_state[NB_SOCK];
extern QueueHandle_t conn_preaccepted;   // 20*1
extern QueueHandle_t conn_recv[NB_SOCK]; // 20*<msg>
extern SemaphoreHandle_t atc_wonna;      // data transfer ready state
extern SemaphoreHandle_t atc_cansend;    // send clear state
extern QueueHandle_t atc_sendres;        // 20*<msg_type>
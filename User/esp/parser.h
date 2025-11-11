///@file parser.h
///@brief esp8266 AT command response parser
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "types/vo.h"

#include <stdint.h>

// number of sock entries, larger that physically supported
#define NB_SOCK 8
// sendres timeout
#define ATC_SENDRES_TIMEOUT pdMS_TO_TICKS(1000)

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

// note: when receiving x,CONNECTED/CLOSED, id is set; when receiving
// +IPD,id,len,pdata is set, caller responsible for deleting pdata

typedef struct {
  atc_msg_type_t type;
  union {
    struct {
      // parm for +IPD(data receive) or x,CONNECT|CLOSED
      uint8_t id;
      uint16_t len;
      vstr_t *pdata;
    };
    // parm for error message, null terminated
    const char *msg;
  };
} atc_msg_t;

// bkgd parsing utils

void atc_parser_clear();
uint8_t atc_parse_char(uint8_t c);
void atc_dispatch(const atc_msg_t *msg);

// rtos-level api's leading to socket api
void atc_parser_loop();
void atc_parser_init();
extern volatile uint8_t atc_parser_init_done;
extern volatile uint8_t conn_state[NB_SOCK];
extern QueueHandle_t conn_preaccepted;   // 20*1
extern QueueHandle_t conn_recv[NB_SOCK]; // 20*<msg>
extern QueueHandle_t atc_sendres;        // 20*<msg_type>

// to replace wonna, cansend. 1 cansend, 2 wonna, 4 flow from peri, 0 flow to
// peri
extern volatile uint8_t atc_peri_state;
int atc_consume_transfer_ready(); // impl in exec
void atc_consume_cmd_ready();     // impl in exec
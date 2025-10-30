///@file parser.c
#include "parser.h"

#include "portmacro.h"
#include "projdefs.h"
#include "types/vo.h"

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "log.h"
#include "port_errno.h"

// behavior: parsing AT command streams, recognizing format:
//     - <MSG>\r\n where MSG is a subset of messages that may appear
//     - <id>,CONNECTED\r\n
//     - <id>,CLOSED\r\n
//     - +IPD,<id>,<len>:.{<len>}\r\n
//     - error sequences may be either with invalid chars or with invalid vals
// where error sequences either end with space or newline/linefeed
// where operation codes, both MSG and CONNECTED/CLOSED, size restricted by
// OP_MAXLEN(excluding); where id restricted by ID_MAX, len restricted by
// BUFF_MAXLEN
// recommend: op related fields (id,len,data) is cleared right after op is
// determined

#define OP_MAXLEN 32     // included
#define BUFF_MAXLEN 2048 // included
#define ID_MAX 5         // excluded

#define STATE_CLEAR 0
#define STATE_MSG_OP 1
#define STATE_CONN_ID 2
#define STATE_CONN_OP 3
#define STATE_IPD_ID 4
#define STATE_IPD_LEN 5
#define STATE_IPD_DAT 6
#define STATE_ERROR_CHAR 7
#define STATE_ECHOING 8

static char msg_op[OP_MAXLEN];
static vstr_t *msg_buff = NULL; // will not used after init
static uint16_t op_len;
static uint16_t conn_id;
static uint16_t buff_len;
static uint16_t buff_offs;
static uint8_t state;
static atc_msg_t msg;

static inline uint8_t isalpha(uint8_t c) { //
  return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}
static inline uint8_t isdigit(uint8_t c) { //
  return c >= '0' && c <= '9';
}
static inline uint8_t isnewline(uint8_t c) { //
  return c == '\r' || c == '\n';
}
static inline uint8_t isspace(uint8_t c) { //
  return c == ' ' || c == '\t';
}

uint8_t state_clear_local = 0;

void atc_parser_clear() {
  //
  op_len = 0;
  conn_id = 0;
  buff_len = 0;
  buff_offs = 0;
  state = STATE_CLEAR;
  msg_op[0] = 0;
  state_clear_local = 0;
}

// entering ensures atc_parser_init_done=1

uint8_t atc_parse_char(uint8_t c) {
  switch (state) {

    // clear
  case STATE_CLEAR: {
    if (isnewline(c) || isspace(c)) {

    } else if (c == '>') {
      // data transfer ready
      xSemaphoreGive(atc_wonna);

    } else if (isdigit(c)) {
      // x,CONNECTED/CLOSED
      // clear vars
      conn_id = c - '0', buff_len = 0, buff_offs = 0, op_len = 0;
      state = STATE_CONN_ID;

    } else if (isalpha(c) || c == '+') {
      msg_op[0] = c, op_len = 1;
      state = STATE_MSG_OP;

    } else {
      state = STATE_ERROR_CHAR;
    }
  } break;

  // expecting op
  // before entering this mode ensures op_len is set
  case STATE_MSG_OP: {
    if (c == ',') {
      if (op_len == 4 && strncmp(msg_op, "+IPD", op_len) == 0) {
        // clear vars
        op_len = 0, buff_len = 0, buff_offs = 0, conn_id = 0;
        vstr_clear(msg_buff);
        state = STATE_IPD_ID;
      } else {
        msg_op[op_len >= OP_MAXLEN ? OP_MAXLEN - 1 : op_len] = 0;
        msg.type = atc_parse_error, msg.msg = msg_op;
        atc_dispatch(&msg);
        state = STATE_ERROR_CHAR;
      }
    } else if (isalpha(c) || isspace(c)) {

      if (op_len >= OP_MAXLEN) {
        msg_op[OP_MAXLEN - 1] = 0;
        msg.type = atc_parse_error, msg.msg = msg_op;
        atc_dispatch(&msg);
        state = STATE_ERROR_CHAR;
      } else {
        msg_op[op_len] = c;
        op_len++;
      }

    } else if (isnewline(c)) {
      if (op_len >= 2 && strncmp(msg_op, "AT", 2) == 0) {
        debug("atc_parser: command echo\r\n");
        state = STATE_ECHOING;
      } else if (op_len == 2 && strncmp(msg_op, "OK", op_len) == 0) {
        debug("atc_parser: OK\r\n");
        msg.type = atc_ok;
        atc_dispatch(&msg);
      } else if (op_len == 5 && strncmp(msg_op, "ERROR", op_len) == 0) {
        debug("atc_parser: ERROR\r\n");
        msg.type = atc_error;
        atc_dispatch(&msg);
      } else if (op_len == 7 && strncmp(msg_op, "SEND OK", op_len) == 0) {
        debug("atc_parser: SEND OK\r\n");
        msg.type = atc_send_ok;
        atc_dispatch(&msg);
      } else if (op_len == 9 && strncmp(msg_op, "SEND FAIL", op_len) == 0) {
        debug("atc_parser: SEND FAIL\r\n");
        msg.type = atc_send_fail;
        atc_dispatch(&msg);
      } else if (op_len == 5 && strncmp(msg_op, "ready", op_len) == 0) {
        msg.type = atc_ready;
        atc_dispatch(&msg);
      } else if (op_len >= 9 && strncmp(msg_op, "busy p...", 9) == 0) {
        msg.type = atc_busy;
        atc_dispatch(&msg);
      } else if ( //
          op_len == 14 && strncmp(msg_op, "WIFI CONNECTED", op_len) == 0) {
        msg.type = atc_wifi_connected;
        atc_dispatch(&msg);
      } else if ( //
          op_len == 15 && strncmp(msg_op, "WIFI DISCONNECT", op_len) == 0) {
        msg.type = atc_wifi_disconnect;
        atc_dispatch(&msg);
      } else if (op_len == 11 && strncmp(msg_op, "WIFI_GOT_IP", op_len) == 0) {
        msg.type = atc_wifi_got_ip;
        atc_dispatch(&msg);
      } else {
        msg_op[op_len >= OP_MAXLEN ? OP_MAXLEN - 1 : op_len] = 0;
        msg.type = atc_parse_error, msg.msg = msg_op;
        atc_dispatch(&msg);
      }
      state = STATE_CLEAR;
    } else {
      msg_op[op_len >= OP_MAXLEN ? OP_MAXLEN - 1 : op_len] = 0;
      msg.type = atc_parse_error, msg.msg = msg_op;
      atc_dispatch(&msg);
      state = STATE_ERROR_CHAR;
    }
  } break;

  // "x,CONNECTION" id part OR "+IPD," id part OR "+IPD," len part
  // entering these states ensures conn_id, data_len and op_len set proper
  case STATE_IPD_LEN:
  case STATE_IPD_ID:
  case STATE_CONN_ID: {
    if (isdigit(c)) {
      // c is part of number

      // sufficient (65535 > 2048)
      if (state == STATE_IPD_LEN)
        buff_len = buff_len * 10 + c - '0';
      else
        conn_id = conn_id * 10 + c - '0';

      // check number threshold
      if (state == STATE_IPD_LEN && buff_len <= BUFF_MAXLEN ||
          state != STATE_IPD_LEN && conn_id < ID_MAX) {
        // nop
      } else {
        // length/id overflow
        msg.type = atc_inval, msg.id = conn_id, msg.len = buff_len;
        atc_dispatch(&msg);
        state = STATE_ERROR_CHAR;
      }
    } else if (state == STATE_IPD_LEN && c == ':' ||
               state != STATE_IPD_LEN && c == ',') {
      // end of the number
      debug("atc_parser: ipd/conn_id next step\r\n");
      if (state == STATE_IPD_LEN && buff_len == 0) {
        // zero len: silent
        state = STATE_CLEAR;
      } else {
        state = state == STATE_IPD_LEN  ? STATE_IPD_DAT
                : state == STATE_IPD_ID ? STATE_IPD_LEN
                                        : STATE_CONN_OP;
      }
    } else {
      // invalid val
      msg.type = atc_inval, msg.id = conn_id, msg.len = 0;
      atc_dispatch(&msg);
      state = isnewline(c) ? STATE_CLEAR : STATE_ERROR_CHAR;
    }
  } break;

  // x,CONNECT op part
  case STATE_CONN_OP: {
    if (isalpha(c)) {
      // buffer sufficient (31 > 9)
      msg_op[op_len] = c;
      op_len++;
      if (op_len == 7 && strncmp(msg_op, "CONNECT", op_len) == 0) {
        msg.type = atc_conn_accepted, msg.id = conn_id;
        atc_dispatch(&msg);
        state = STATE_CLEAR;
      } else if (op_len == 6 && strncmp(msg_op, "CLOSED", op_len) == 0) {
        msg.type = atc_conn_closed, msg.id = conn_id;
        atc_dispatch(&msg);
        state = STATE_CLEAR;
      } else if (op_len >= 9) {
        // op too long unmatched
        msg_op[op_len] = 0;
        msg.type = atc_parse_error, msg.msg = msg_op;
        atc_dispatch(&msg);
        state = STATE_ERROR_CHAR;
      } else {
      }
    } else {
      // error char in op
      msg_op[op_len == OP_MAXLEN ? OP_MAXLEN - 1 : op_len] = 0;
      msg.type = atc_parse_error, msg.msg = msg_op;
      atc_dispatch(&msg);
      state = isnewline(c) ? STATE_CLEAR : STATE_ERROR_CHAR;
    }
  } break;

  //+ipd:data
  // entering this state ensures data_len>0 and data_offs=0
  case STATE_IPD_DAT: {
    // msg_buff[buff_offs] = c;
    vbuff_iaddc(msg_buff, c);
    buff_offs++;

    if (buff_offs == buff_len) {
      // end of +IPD: create new buff to construct msg
      debug("atc_parser: end of ipd with len=%d\r\n", buff_len);

      vstr_t *tmp = msg_buff;
      msg_buff = vstr_create(0);

      msg.type = atc_conn_recv, msg.pdata = tmp;
      msg.id = conn_id, msg.len = buff_len;

      atc_dispatch(&msg);
      state = STATE_CLEAR;
    }

  } break;

  // error char
  case STATE_ECHOING:
  case STATE_ERROR_CHAR: {
    if (isnewline(c) || isspace(c)) {
      debug("atc_parser: echo/err state clear\r\n");
      state = STATE_CLEAR;
    }
  } break;

  default:
    break;

  } // switch

  if (!state_clear_local && state == STATE_CLEAR) {
    debug("atc_parser: cansend semaphore give\r\n");
    xSemaphoreGive(atc_cansend);
  }
  state_clear_local = state == STATE_CLEAR;

  return state;
}

volatile uint8_t atc_parser_init_done = 0;
volatile uint8_t conn_state[NB_SOCK]; // 0 for close, 1 for open
QueueHandle_t conn_preaccepted;       // <u8>
QueueHandle_t conn_recv[NB_SOCK] = {0};
SemaphoreHandle_t atc_wonna = NULL;
SemaphoreHandle_t atc_cansend = NULL;
QueueHandle_t atc_sendres = NULL;

void atc_parser_init() {
  debug("atc_parser_init: enter\r\n");
  if (atc_parser_init_done)
    return;

  for (int i = 0; i < NB_SOCK; i++) {
    conn_state[i] = 0;
    conn_recv[i] = xQueueCreate(20, sizeof(atc_msg_t));
    assert(conn_recv[i]);
  }
  conn_preaccepted = xQueueCreate(20, 1);
  assert(conn_preaccepted);
  atc_wonna = xSemaphoreCreateBinary();
  assert(atc_wonna);
  atc_cansend = xSemaphoreCreateBinary();
  assert(atc_cansend);
  atc_sendres = xQueueCreate(1, sizeof(atc_msg_type_t));
  assert(atc_sendres);

  msg_buff = vstr_create(0);

  atc_parser_clear();

  xSemaphoreGive(atc_cansend);

  debug("atc_parser_init: done\r\n");

  atc_parser_init_done = 1;
}

// dispatch

void atc_dispatch(atc_msg_t *msg) {
  switch (msg->type) {

  // messages send to sendres
  case atc_parse_error:
  case atc_inval:
    // hint the message
    if (msg->type == atc_parse_error) {
      printf("atc_dispatch: parser error: %s\r\n", msg->msg);
    } else {
      printf("atc_dispatch: invalid values: id=%d, len=%d\r\n", msg->id,
             msg->len);
    }
    break;
  case atc_unknown: // unused
  case atc_ok:
  case atc_error:
  case atc_send_ok:
  case atc_send_fail: {
    debug("atc_dispatch: sendres message: %d\r\n", msg->type);
    xQueueSend(atc_sendres, &msg->type, ATC_SENDRES_TIMEOUT);
  } break;

  // hint messages
  case atc_ready:
  case atc_busy:
  case atc_wifi_connected:
  case atc_wifi_got_ip:
  case atc_wifi_disconnect: {
    printf("atc_dispatch: message: %d\r\n", msg->type);
  } break;

    // conn opts, unimpl

  case atc_conn_accepted: {
    debug("atc_dispatch: conn accepted id=%d\r\n", msg->id);
    if (conn_state[msg->id])
      break;
    conn_state[msg->id] = 1;
    xQueueReset(conn_recv[msg->id]);
    xQueueSend(conn_preaccepted, &msg->id, portMAX_DELAY);
  } break;

  case atc_conn_closed: {
    // from the socket view, conn state is deter by conn_state and queue
    conn_state[msg->id] = 0;
  } break;

  case atc_conn_recv: {
    debug("atc_dispatch: received id=%d,len=%d\r\n", msg->id, msg->len);
    // if (!conn_state[msg->id])
    //   break;
    xQueueSend(conn_recv[msg->id], msg, portMAX_DELAY);
  } break;

  default:
    assert(0 && EUNREACHABLE);
  }
}

extern QueueHandle_t m_esp8266_qin; // in uart init

void atc_parser_loop() {
  puts("atc_parser_loop: enter");
  HAL_UART_Receive_IT(&m_u3h, (void *)&m_esp8266_recvbyte, 1);
  debug("atc_parser_loop: started receiving\r\n");

  while (1) {
    uint8_t recvbyte;
    if (!m_esp8266_qin) {
      puts("atc_parser_loop: waiting for m_esp8266_qin init");
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // move assign
    BaseType_t pass = xQueueReceive(m_esp8266_qin, &recvbyte, portMAX_DELAY);

    if (pass == pdTRUE) {
      if (!isspace(recvbyte)) {
        // debug("recv: %c\r\n", recvbyte);
      } else {
        // debug("atc_parser_loop: received byte: 0x%x\r\n", recvbyte);
      }
      atc_parse_char(recvbyte);
    } else {
      debug("atc_parser_loop: xQueueReceive failed\r\n");
    }
  }
}
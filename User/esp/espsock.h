///@file espsock.h
///@brief non standard socket api upon esp8266
#pragma once

#include "esp/parser.h"
#include "types/vo.h"

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "port_errno.h"

#define LSTN_SK_FD 255

extern volatile uint8_t esk_init_done;
extern QueueHandle_t esk_lstn_res;
extern QueueHandle_t esk_conn_res[NB_SOCK]; // NB_SOCK > 5
extern vstr_t *esk_recv_buff[NB_SOCK];

///@brief low level socket(),create singleton listener socket, consists of
/// network setup
///@return LSTN_SK_FD or negative errno
int sock_init();

///@brief low level bind() + listen()
///@param sockfd socket fd from sock_init, must be LSTN_SK_FD
///@return 0 or negative errno
int sock_listen(int sockfd);

///@brief low level accept()
///@param sockfd socket fd from sock_init, must be LSTN_SK_FD
///@param flags !=0 for nonblocking accept
///@return conn socket fd (actual id of sock 0..4, not considered mapping into
/// fd table) or negative errno
int sock_accept(int sockfd, int flags);

///@brief low level send()
///@param sockfd socket fd from sock_init, 0..4
///@param buff data to send
///@param len length to send, actual len is less than both buff->len and len
///@param flags >0 for nonblocking send
///@return 0 or negative errno
int sock_send(int sockfd, const vstr_t *buff, uint16_t len, int flags);

///@brief low level recv()
///@param sockfd socket fd from sock_init, 0..4
///@param buff buffer to receive data into, reference
///@param size max size to receive
///@param flags unused, default blocking, MBZ
///@return number of bytes received or negative errno
int sock_recv(int sockfd, vstr_t *buff, size_t size, int flags);

int sock_close(int sockfd);

// non-canonical func
int sock_is_conn(int sockfd);

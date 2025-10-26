///@file espsock.h
///@brief non standard socket api upon esp8266
#pragma once

#include "types/vo.h"

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "port_errno.h"

#define LSTN_SK_FD 255

extern volatile uint8_t esk_init_done;
extern QueueHandle_t esk_lstn_res;

int sock_init(); // singleton sock listener; consists of network setup

int sock_listen(int sockfd); // bind+listen

int sock_accept(int sockfd, int flags);

int sock_send(int sockfd, const vstr_t *buff, int flags);

int sock_recv(int sockfd, vstr_t *buff, size_t size, int flags);
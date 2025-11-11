///@file task/tcp_echo_task.h
#pragma once

#include <stdint.h>

void tcp_sshd_task(void *params);

uint8_t sshd_acpt(int listen_sock); // impl in ssh/acpt_loop.c
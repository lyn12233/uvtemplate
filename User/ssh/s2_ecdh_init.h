#pragma once

#include "ssh/ssh_context.h"
#include "types/vo.h"

void consume_ecdh_init(int sock, ssh_context *ctx, int *done);

void send_ecdh_reply(int sock, ssh_context *ctx, int *done);

void exchange_msg_newkey(int sock, int *done);
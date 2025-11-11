#pragma once
#include "ssh_context.h"
#include "types/vo.h"

#include <stdint.h>

void exchange_version_header(int sock, ssh_context *ctx, int *done);
void send_kexinit(int sock, ssh_context *ctx);
void consume_kexinit(int sock, ssh_context *ctx);

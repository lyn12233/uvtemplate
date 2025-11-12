#pragma once

#include "ssh_context.h"
#include "types/vo.h"

#include <stdint.h>

void consume_openchannel(int sock, ssh_context *ctx, int *done);
void respond_chnlreq(int sock, ssh_context *ctx, int *done);
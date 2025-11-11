#pragma once

#include "ssh_context.h"
#include "types/vo.h"

#include <stdint.h>

void respond_auth_service(int sock, ssh_context *ctx, int *done);

void respond_userauth(int sock, ssh_context *ctx, int *done, int *half);
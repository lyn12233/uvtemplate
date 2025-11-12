#pragma once

#include "packet.h"
#include "ssh_context.h"
#include "types/vo.h"

#include "FreeRTOS.h"
#include "queue.h"

#define SSH_FXP_INIT 1
#define SSH_FXP_VERSION 2
#define SSH_FXP_OPEN 3
#define SSH_FXP_CLOSE 4
#define SSH_FXP_READ 5
#define SSH_FXP_WRITE 6
#define SSH_FXP_LSTAT 7
#define SSH_FXP_FSTAT 8
#define SSH_FXP_SETSTAT 9
#define SSH_FXP_FSETSTAT 10
#define SSH_FXP_OPENDIR 11
#define SSH_FXP_READDIR 12
#define SSH_FXP_REMOVE 13
#define SSH_FXP_MKDIR 14
#define SSH_FXP_RMDIR 15
#define SSH_FXP_REALPATH 16
#define SSH_FXP_STAT 17
#define SSH_FXP_RENAME 18
#define SSH_FXP_READLINK 19
#define SSH_FXP_SYMLINK 20

#define SSH_FXP_STATUS 101
#define SSH_FXP_HANDLE 102
#define SSH_FXP_DATA 103
#define SSH_FXP_NAME 104
#define SSH_FXP_ATTRS 105
#define SSH_FXP_EXTENDED 200
#define SSH_FXP_EXTENDED_REPLY 201

#define SFTP_CHUNK 1024

void sftp_parser_init();

void sftp_parse(int sock, ssh_context *ctx, QueueHandle_t q, int *should_close);

void sftp_dispatch(int sock, ssh_context *ctx, uint8_t c, int *should_close);

void sftp_dispatch_spkt(int sock, ssh_context *ctx, const vstr_t *pkt);
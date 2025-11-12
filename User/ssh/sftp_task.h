#pragma once

#include "packet.h"
#include "packet_def.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdint.h>

void sftp_task(void *p);

void sftp_recv(QueueHandle_t q, vstr_t *pre_recv, vstr_t *buff, uint32_t len);

void sftp_start_task(QueueHandle_t q);
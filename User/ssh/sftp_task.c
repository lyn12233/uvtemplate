#include "sftp_task.h"
#include "FreeRTOSConfig.h"
#include "port_socket.h"
#include "portmacro.h"
#include "types/vo.h"

#include "port_unistd.h"

#include <stdint.h>

#include "allocator.h"
#include "log.h"

typedef struct {
  QueueHandle_t q;
  vstr_t *pre_recv;
} sftp_task_parm;

void sftp_task(void *p) {
  sftp_task_parm *p2 = p;
  QueueHandle_t q = p2->q;
  vstr_t *pre_recv = p2->pre_recv; // move assign

  free(p);
  (void)p, (void)q;

  // option b: receive a whole sftp chunk and parse it
  vstr_t buff;
  vstr_init(&buff, 0);

  puts("sftp task start");

  while (1) {
    vstr_clear(&buff);
    sftp_recv(q, pre_recv, &buff, 4);
    uint32_t len = ntohl(*(uint32_t *)buff.buff);

    printf("sftp task: packet len: %u\r\n", len);

    if (len > 3000) {
      puts("warn: packet len exceeds");
      assert(0);
    }

    vstr_clear(&buff);
    sftp_recv(q, pre_recv, &buff, len);
    puts("sftp: packet:");
    vbuff_dump(&buff);

    // dispatch
  }
}

void sftp_recv(QueueHandle_t q, vstr_t *pre_recv, vstr_t *buff, uint32_t len) {
  //
  uint32_t rem = len;
  while (1) {
    if (rem <= pre_recv->len) {
      // tmp:tail; buff: append the pre_recv head
      vstr_t *tmp = vstr_create(0);
      vbuff_iadd(tmp, &pre_recv->data[rem], pre_recv->len - rem);
      vbuff_iadd(buff, pre_recv->data, rem);

      vstr_clear(pre_recv);
      vbuff_iadd(pre_recv, tmp->data, tmp->len);
      vstr_delete(tmp);

      break;
    } else {

      vbuff_iadd(buff, pre_recv->data, pre_recv->len);
      rem -= pre_recv->len;

      vstr_clear(pre_recv); // cleared(.data=NULL), ok for move assign
      xQueueReceive(q, pre_recv, portMAX_DELAY);

      puts("sftp task (pre) got:");
      vbuff_dump(pre_recv);
    }
  }
}

void sftp_start_task(QueueHandle_t q) {
  sftp_task_parm *parm = malloc(sizeof(sftp_task_parm));

  *parm = (sftp_task_parm){.q = q, .pre_recv = vstr_create(0)};

  xTaskCreate(sftp_task, "sftp manager task", 256, &parm,
              configMAX_PRIORITIES - 2, NULL);
}
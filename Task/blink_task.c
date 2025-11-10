#include "blink_task.h"

#include "projdefs.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "portable.h"
#include "task.h"

#include "ff.h"

#include <stdint.h>

#include "log.h"

void BlinkTask(void *p) {

  while (!fs_init_done) {
    puts("blink task: wait fs init");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  puts("blink start");

  int cnt = 0;
  while (1) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);

    cnt = (cnt + 1) % 10;
    // st[130 - cnt] *= cnt; // stack overflow test

    if (!cnt) {
      printf("blink task: free size: %d\r\n", (int)xPortGetFreeHeapSize());

      // test: writing to file
      {
        continue;
        FIL fp;
        FRESULT fr;
        UINT bw;
        fr = f_open(&fp, "log.txt", FA_WRITE | FA_OPEN_APPEND);
        if (fr != FR_OK) {
          printf("blink task: open file failed with %d\r\n", (int)fr);
          continue;
        }
        fr = f_write(&fp, "Hello World", 11, &bw);
        if (fr != FR_OK) {
          printf("blink task: write file failed with %d\r\n", (int)fr);
          f_close(&fp);
          continue;
        }
        puts("blink task: write file success!");
        fr = f_sync(&fp);
        printf("%d", fr);
        fr = f_close(&fp);
        printf("%d", fr);
        puts("");
      }
    }

    // delay

    vTaskDelay(700); // fine
    // HAL_Delay(700); // fine
  }
}
#include "wiz_test.h"

#include "loopback.h"
#include "socket.h"
#include "wizspi_init.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_spi.h"

#include "wizchip_conf.h"

#include <stdint.h>
#include <string.h>

#include "log.h"

uint8_t ethernet_buff[1024 * 2];
void wizspi_test_mainloop() {
  int res;
  puts("start wiztest mainloop");

init:
  res = wizspi_w5500_init();
  if (res != 0) {
    puts("wizspi init failed, reinit");
    HAL_Delay(1000);
    goto init;
  }
  puts("wizspi init success");
  wizspi_test_print();
  puts("");
  int cnt = 0;
  while (1) {
    HAL_Delay(1);
    // puts("loopback");
    res = loopback_tcps(0, ethernet_buff, 8080);
    if (res == SOCKERR_SOCKINIT) {
      puts("loopback failed with SCOKERR_SOCKINIT, reinit");
      goto init;
    }
    WIZSPI_RST_1();

    cnt = (cnt + 1) % 10000; // routine log per 10s
    if (!cnt) {
      wizspi_test_print();
    }
  }
}
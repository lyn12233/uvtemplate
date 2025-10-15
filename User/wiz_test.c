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

void wizspi_test_print() {
  wiz_NetInfo ni = {0};
  wizchip_getnetinfo(&ni);
  printf("ip    : %u.%u.%u.%u\r\n", ni.ip[0], ni.ip[1], ni.ip[2], ni.ip[3]);
}

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
  while (1) {
    HAL_Delay(1000);
    puts("loopback");
    res = loopback_tcps(0, ethernet_buff, 5000);
    if (res == SOCKERR_SOCKINIT) {
      puts("loopback failed with SCOKERR_SOCKINIT, reinit");
      goto init;
    }
  }
}
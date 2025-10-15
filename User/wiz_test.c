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

void wizspi_phy_print() {
  uint8_t phycfgr = getPHYCFGR();
  // wiz_PhyConf conf;
  printf("PHYCFGR: 0x%02X\r\n", phycfgr);

  // Check individual bits
  printf("Link: %s\r\n", (phycfgr & 0x01) ? "UP" : "DOWN");
  printf("Speed: %s\r\n", (phycfgr & 0x02) ? "100M" : "10M");
  printf("Duplex: %s\r\n", (phycfgr & 0x04) ? "Full" : "Half");
  printf("Auto-neg: %s\r\n", (phycfgr & 0x08) ? "Done" : "Not done");
  printf("Power: %s\r\n", (phycfgr & 0x10) ? "Down" : "Normal");
  printf("Reset: %s\r\n", (phycfgr & 0x80) ? "In reset" : "Normal");
}

void wizspi_test_print() {
  wiz_NetInfo ni = {0};
  wizchip_getnetinfo(&ni);
  printf("ip     \t: %u.%u.%u.%u\r\n", //
         ni.ip[0], ni.ip[1], ni.ip[2], ni.ip[3]);
  printf("mac    \t: %x:%x:%x:%x:%x:%x\r\n", //
         ni.mac[0], ni.mac[1], ni.mac[2], ni.mac[3], ni.mac[4], ni.mac[5]);
  printf("gateway\t: %u.%u.%u.%u\r\n", //
         ni.gw[0], ni.gw[1], ni.gw[2], ni.gw[3]);
  printf("subnet \t: %u.%u.%u.%u\r\n", //
         ni.sn[0], ni.sn[1], ni.sn[2], ni.sn[3]);
  printf("dns    \t: %u.%u.%u.%u\r\n", //
         ni.dns[0], ni.dns[1], ni.dns[2], ni.dns[3]);
  printf("dhcp   \t: %s\r\n", ni.dhcp == 1 ? "Static" : "DHCP");

  printf("phy link\t: %s\r\n",
         wizphy_getphylink() == PHY_LINK_ON ? "PHY_LINK_ON" : "PHY_LINK_OFF");
  wizspi_phy_print();
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
  puts("");
  while (1) {
    HAL_Delay(100);
    // puts("loopback");
    res = loopback_tcps(0, ethernet_buff, 5000);
    if (res == SOCKERR_SOCKINIT) {
      puts("loopback failed with SCOKERR_SOCKINIT, reinit");
      goto init;
    }
  }
}
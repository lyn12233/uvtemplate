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

SPI_HandleTypeDef m_wizspi_spih;
void wizspi_spi_init() {
  // enable clock for GPIOG,GPIOB,SPI2,AF
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  // register pin mode for cs, sclk, miso, mosi

  // cs. ce
  GPIO_InitTypeDef gi;
  gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_7 | GPIO_PIN_8,
      .Mode = GPIO_MODE_OUTPUT_PP,
      .Speed = GPIO_SPEED_HIGH,
  };
  HAL_GPIO_Init(GPIOG, &gi);
  WIZSPI_CS_1();
  WIZSPI_RST_1();

  // sclk miso mosi
  gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
      .Mode = GPIO_MODE_AF_PP,
      .Speed = GPIO_SPEED_HIGH,
  };
  HAL_GPIO_Init(GPIOB, &gi);

  //
  // spi config
  SPI_InitTypeDef spi_init = (SPI_InitTypeDef){
      .Direction = SPI_DIRECTION_2LINES,
      .Mode = SPI_MODE_MASTER,
      .DataSize = SPI_DATASIZE_8BIT,
      .CLKPolarity = SPI_POLARITY_LOW,
      .CLKPhase = SPI_PHASE_1EDGE,
      .NSS = SPI_NSS_SOFT, // software chip select
      .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2,
      .FirstBit = SPI_FIRSTBIT_MSB, // little endian
      .CRCCalculation = SPI_CRCCALCULATION_DISABLED,
      .CRCPolynomial = 7, // MBNZ
  };
  m_wizspi_spih = (SPI_HandleTypeDef){.Instance = SPI2, .Init = spi_init};
  CHECK_FAIL(HAL_SPI_Init(&m_wizspi_spih) == HAL_OK);
}
void wizspi_writebyte(uint8_t tx_data) {
  // puts("wiz: writebyte");
  CHECK_FAIL(HAL_SPI_Transmit(&m_wizspi_spih, &tx_data, 1, 10) == HAL_OK);
}
uint8_t wizspi_readbyte() {
  // puts("wiz: readbyte");
  uint8_t val;
  CHECK_FAIL(HAL_SPI_Receive(&m_wizspi_spih, &val, 1, 10) == HAL_OK);
  return val;
}
void wizspi_enter() { __disable_irq(); }
void wizspi_exit() { __enable_irq(); }
void wizspi_select() { WIZSPI_CS_0(); }
void wizspi_deselect() { WIZSPI_CS_1(); }
//?
void wizspi_register() {
  reg_wizchip_cris_cbfunc(wizspi_enter, wizspi_exit);
  reg_wizchip_cs_cbfunc(wizspi_select, wizspi_deselect);
  reg_wizchip_spi_cbfunc(wizspi_readbyte, wizspi_writebyte);
  reg_wizchip_spiburst_cbfunc(NULL, NULL);
}
void wizspi_reset() {
  WIZSPI_RST_0();
  HAL_Delay(500);
  WIZSPI_RST_1();
}
//?
// set network attrs
void wizspi_network_init() {
  uint8_t chip_id[6];
  wiz_NetInfo net_info;
  wiz_NetTimeout net_timeout;

  // possible valid netinfo
  net_info = (wiz_NetInfo){
      .mac = {0, 8, 0xdc, 0x11, 0x11, 0x11}, // must be unique
      .ip = {192, 168, 31, 218},             // subnet should be compliant
      .sn = {255, 255, 255, 0},              // subnet mask for 192.168.x
      .gw = {192, 168, 31, 1},               // gateway, by default
      .dns = {192, 168, 31, 1},              // dns server or 8.8.8.8
      .dhcp = NETINFO_DHCP, // or static; when static, ip should be out of dhcp
                            // effective range
  };

  CHECK_FAIL(ctlnetwork(CN_SET_NETINFO, &net_info) == 0);
  CHECK_FAIL(ctlnetwork(CN_GET_NETINFO, &net_info) == 0);
  CHECK_FAIL(ctlwizchip(CW_GET_ID, &chip_id) == 0);

  net_timeout = (wiz_NetTimeout){.retry_cnt = 50, .time_100us = 1000};
  wizchip_settimeout(&net_timeout);
}

int wizspi_w5500chip_init() {
  int res = wizchip_init(0, 0); // specify tx/rx buff size as default
  wizphy_reset();
  return res;
}

int wizspi_w5500phy_init() {
  // least quality config trading for validity
  wiz_PhyConf phy_conf = (wiz_PhyConf){
      .by = PHY_CONFBY_SW,       // PHYCFGR[6]
      .duplex = PHY_DUPLEX_HALF, // or full
      .mode = PHY_MODE_MANUAL,   // or negotiate
      .speed = PHY_SPEED_10,     // or 100M
  };
  wizphy_setphyconf(&phy_conf); // chip reset is called within

  return 0;
}
int wizspi_w5500_init() {
  wizspi_spi_init();
  wizspi_reset();
  wizspi_register();
  if (wizspi_w5500chip_init()) {
    puts("chip init failed");
    return -1;
  }
  if (wizspi_w5500phy_init())
    return -1;
  wizspi_network_init();
  return 0;
}

void wizspi_phy_print() {
  uint8_t phycfgr = getPHYCFGR();
  // wiz_PhyConf conf;
  printf("PHYCFGR: 0x%02X\r\n", phycfgr);

  // Check individual bits
  printf("Link: %s\r\n", (phycfgr & 0x01) ? "UP" : "DOWN");
  printf("Speed: %s\r\n", (phycfgr & 0x02) ? "100M" : "10M");
  printf("Duplex: %s\r\n", (phycfgr & 0x04) ? "Full" : "Half");
  printf("OPMDC: [5,4,3]:%d,%d,%d\r\n", (phycfgr & 0x20) != 0,
         (phycfgr & 0x10) != 0, (phycfgr & 0x08) != 0);
  printf("OPMD: %s\r\n", (phycfgr & 0x40) ? "by sw" : "by hw");
  printf("Reset: %s\r\n", (phycfgr & 0x80) ? "after reset" : "reset");
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
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

  // cs
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
  HAL_GPIO_Init(GPIOA, &gi);

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
  HAL_Delay(50);
  WIZSPI_RST_1();
  HAL_Delay(10);
}
//?
// set network attrs
void wizspi_network_init() {
  uint8_t chip_id[6];
  wiz_NetInfo net_info;
  wiz_NetTimeout net_timeout;

  net_info = (wiz_NetInfo){
      .mac = {0, 8, 0xdc, 0x11, 0x11, 0x11},
      .ip = {192, 168, 86, 150},
      .sn = {255, 255, 255, 0},
      .gw = {192, 168, 86, 1},
      .dns = {8, 8, 8, 6},
      .dhcp = NETINFO_STATIC,
  };

  CHECK_FAIL(ctlnetwork(CN_SET_NETINFO, &net_info) == 0);
  CHECK_FAIL(ctlnetwork(CN_GET_NETINFO, &net_info) == 0);
  CHECK_FAIL(ctlwizchip(CW_GET_ID, &chip_id) == 0);

  net_timeout = (wiz_NetTimeout){.retry_cnt = 50, .time_100us = 1000};
  wizchip_settimeout(&net_timeout);
}

int wizspi_w5500chip_init() {
  return wizchip_init(0, 0); // specify tx/rx buff size
}

int wizspi_w5500phy_init() {
  wiz_PhyConf phy_conf = (wiz_PhyConf){
      .by = PHY_CONFBY_SW,
      .duplex = PHY_DUPLEX_FULL,
      .mode = PHY_MODE_AUTONEGO,
      .speed = PHY_SPEED_10,
  };
  return ctlwizchip(CW_SET_PHYCONF, &phy_conf);
}
int wizspi_w5500_init() {
  wizspi_spi_init();
  wizspi_reset();
  wizspi_register();
  if (wizspi_w5500chip_init())
    return -1;
  if (wizspi_w5500phy_init())
    return -1;
  wizspi_network_init();
  return 0;
}

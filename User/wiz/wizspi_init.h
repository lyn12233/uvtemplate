// wiznet spi initialization, setting w5500,
#pragma once

#include "stm32f1xx_hal_gpio.h"

#include <stdint.h>

// chip select/ nrf_cs - pg7; low to select
#define WIZSPI_CS_1() HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_SET)
#define WIZSPI_CS_0() HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_RESET)

// spi reset/ nrf_ce - pg8; low to reset
#define WIZSPI_RST_1() HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_SET)
#define WIZSPI_RST_0() HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_RESET)

///@note nrf: a wireless comm peri

// spi SCLK spi2_sck - pb13
#define WIZSPI_SCLK_1() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET)
#define WIZSPI_SCLK_0() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET)

// spi MISO spi2_miso - pb14
#define WIZSPI_MISO_1() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET)
#define WIZSPI_MISO_0() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET)

// spi MOSI spi2_mosi - pb15
#define WIZSPI_MOSI_1() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET)
#define WIZSPI_MOSI_0() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET)

// hardware-dependent interface for iolibrary
void wizspi_spi_init();
void wizspi_writebyte(uint8_t tx_data);
uint8_t wizspi_readbyte();
void wizspi_enter();
void wizspi_exit();
void wizspi_select();
void wizspi_deselect();
// register interfaces above to iolibrary
void wizspi_register();

/// toggle spi reset pin
void wizspi_reset();

// set network layer attrs
void wizspi_network_init();

// main initor
int wizspi_w5500_init();

// chip buff size settings. right after spi init
int wizspi_w5500chip_init();

// physical layer init, prior to network layer
int wizspi_w5500phy_init();

// logger
void wizspi_phy_print();
void wizspi_test_print();

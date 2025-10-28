///@file User/user_init/init_sdio.h
#include "initors.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_sd.h"

#include "log.h"

SD_HandleTypeDef m_sdh;

void sdio_init() {
  __HAL_RCC_SDIO_CLK_ENABLE();
  m_sdh = (SD_HandleTypeDef){
      .Instance = SDIO,
      .Init =
          (SDIO_InitTypeDef){
              .ClockEdge = SDIO_CLOCK_EDGE_RISING,
              .ClockBypass = SDIO_CLOCK_BYPASS_DISABLE,
              .ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE,
              .BusWide = SDIO_BUS_WIDE_1B,
              .HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE,
              .ClockDiv = 3, // 24MHz
          },
  };
  assert(HAL_SD_Init(&m_sdh) == HAL_OK);

  // todo: config bus op 4 bits
}
void HAL_SD_MspInit(SD_HandleTypeDef *hsd) {
  // pc8-pc11: d0-d3, pc12: sck, pd2: cmd
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  GPIO_InitTypeDef gi = {0};

  gi.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  gi.Mode = GPIO_MODE_AF_PP;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &gi);

  gi.Pin = GPIO_PIN_2;
  gi.Mode = GPIO_MODE_AF_PP;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &gi);
}
///@brief This file provides code for the MSP Initialization

#include "stm32f1xx_hal.h"

void HAL_MspInit(void) {
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /** DISABLE: JTAG-DP Disabled and SW-DP Disabled
   */
  __HAL_AFIO_REMAP_SWJ_DISABLE();
}

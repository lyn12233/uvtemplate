#include "wiz_test.h"

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

void wizspi_test_mainloop() {
  int res = wizspi_w5500_init();
  if (res != 0)
    puts("wizspi init failed");
  else
    puts("wizspi init success");
}
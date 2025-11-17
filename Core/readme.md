## description of `Core/`

files in this dir is gen'd by cubemx except freertos_hooks.c

- `Inc`
  - `main.h`: header for `main.c`
  - `stm32f1xx_hal_conf.h`: hal configuration. manipulate this file to enable/disable hal modules
  - `stm32f1xx_it.h`: export interrupt handlers described in NVIC table
- `Src`
  - `freertos_hooks.c`: final systick_handler impl and freertos callback not provided in freertos ports
  - `main.c`: system clock setup and c main entry
  - `startup_stm32f103xe.s`: instruction entry and NIVC table
  - ...

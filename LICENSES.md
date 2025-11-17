# Project License Information

this repository contains both original code and code derived from third party sources
- original code is licensed under mit license, see [license](./LICENSE)
- code derived from third party sources complies to corresponding licenses as described below

## Third-Party Components

### CMSIS (Cortex Microcontroller Software Interface Standard)
- **Source**: ARM Limited
- **License**: Apache 2.0 (or check specific version)
- **Location**: `Drivers/CMSIS/`
- **Notes**: Please refer to the original LICENSE file in the CMSIS directory

### STM32 HAL (Hardware Abstraction Layer)
- **Source**: STMicroelectronics
- **License**: BSD-3-Clause
- **Location**: `Drivers/STM32xx_HAL_Driver/`
- **Notes**: Please refer to the original LICENSE file in the HAL directory

## FatFS
- **Source**: ChaN ([elm-chan.org](http://elm-chan.org))
- **License**: BSD-style (FatFS license)
- **Location**: `FatFS/`
- **Notes**: Please refer to the original licenses provided as leading comments in FatFS source files.

### FreeRTOS Kernel
- **Source**: Amazon Web Services (formerly Real Time Engineers Ltd.)
- **License**: MIT
- **Location**: `FreeRTOS/`
- **Notes**: Please refer to the original LICENSE file in the FreeRTOS directory

### ioLibrary
- **Source**: WIZnet
- **License**: MIT
- **Location**: `IOLibrary/`
- **Notes**: Please refer to the original LICENSE file in the FreeRTOS directory

### OpenSSH
- **Source**: OpenSSH
- **License**: BSD-style
- **Location**: (distributed, as single files)
- **Notes**: Please refer to the original licenses provided as leading comments in OpenSSH derived files. These files mainly include cryptographic functions in `User/crypto/`.

## Usage Requirements
When distributing this project, you must include all original license notices from each component.
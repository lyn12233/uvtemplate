///@file initors.h
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "stm32f103xe.h"
#include "stm32f1xx_hal_sd.h"
#include "stm32f1xx_hal_uart.h"

#include <stdint.h>

///@defgroup user_init
///@brief user-defined peripheral port initialization steps

#define USER_UART1_SPEED 115200
#define USER_UART3_SPEED 115200

void LED_init();

extern UART_HandleTypeDef m_uh;
extern DMA_HandleTypeDef m_u1dma;

void uart1_init();

extern UART_HandleTypeDef m_u3h;
extern QueueHandle_t m_esp8266_qin;
extern volatile uint8_t m_esp8266_recvbyte;
extern SemaphoreHandle_t m_esp8266_senddone;
// extern uint8_t m_esp8266_recvbuff[256];

void uart3_init();

void HAL_UART_MspInit(UART_HandleTypeDef *huart);
void UART3_IRQHandler(); // override NVIC table
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

extern ADC_HandleTypeDef m_adch;
void adc_init();

extern SD_HandleTypeDef m_sdh;
void sdio_init();
void HAL_SD_MspInit(SD_HandleTypeDef *hsd);

// misc

#define LED_TOGGLE() HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5)

extern int systick_count; // implemented at _hooks.c

void user_init();

#if !defined(USE_ESP8266_NETWORK) && !defined(USE_W5500_NETWORK)
#define USE_ESP8266_NETWORK
#endif
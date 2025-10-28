///@file initors.h
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "stm32f103xe.h"
#include "stm32f1xx_hal_sd.h"
#include "stm32f1xx_hal_usart.h"

#include <stdint.h>

///@defgroup user_init
///@brief user-defined peripheral port initialization steps

void LED_init();

extern USART_HandleTypeDef m_uh;
void usart1_init();
extern USART_HandleTypeDef m_u3h;
extern QueueHandle_t m_esp8266_qin;
extern uint8_t m_esp8266_recvbyte;
extern SemaphoreHandle_t m_esp8266_senddone;
// extern uint8_t m_esp8266_recvbuff[256];

void usart3_init();

void HAL_USART_MspInit(USART_HandleTypeDef *husart);
void USART3_IRQHandler(); // override NVIC table
void HAL_USART_RxCpltCallback(USART_HandleTypeDef *husart);

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
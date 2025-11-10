///@file uart_init.c
#include "initors.h"
#include "main.h"

#include "projdefs.h"
#include "user_init/initors.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_uart.h"

#include "FreeRTOS.h"
#include "portmacro.h"
#include "semphr.h"

#include "log.h"
#include <stdint.h>

///@defgroup uart_init
///@ingroup user_init
///@{

//
// USART1 part

// gpio-uart1 init
///@brief uart1 handle
UART_HandleTypeDef m_uh = {0};

#define SZ_DMA1C5_BUF 256
uint8_t m_u1dma_buf[SZ_DMA1C5_BUF];

uint8_t u1recv_byte;
static void u1recv_process(void *p);

///@brief uart1-default logger init, call in the main entry
void uart1_init() {
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  // init pa9-tx pa10-rx
  GPIO_InitTypeDef gi;
  gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_9,
      .Mode = GPIO_MODE_AF_PP,
      .Speed = GPIO_SPEED_FREQ_HIGH,
  };
  HAL_GPIO_Init(GPIOA, &gi);
  gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_10,
      .Mode = GPIO_MODE_INPUT,
      .Pull = GPIO_NOPULL,
  };
  HAL_GPIO_Init(GPIOA, &gi);

  m_uh = (UART_HandleTypeDef){
      .Instance = USART1,
      .Init =
          (UART_InitTypeDef){
              .BaudRate = USER_UART1_SPEED,
              .WordLength = UART_WORDLENGTH_8B,
              .StopBits = UART_STOPBITS_1,
              .Parity = UART_PARITY_NONE,
              .Mode = UART_MODE_TX_RX,
          },
  };
  HAL_UART_Init(&m_uh);

  // nvic config interrupt pri and enable
  HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  // start first receive
  HAL_StatusTypeDef res;
  res = HAL_UART_Receive_IT(&m_uh, &u1recv_byte, 1);
  assert(res == HAL_OK);
}

// uart1 interrupt handlers
// irq handler
void USART1_IRQHandler() {
  HAL_UART_IRQHandler(&m_uh);

  // clear flags
  if (__HAL_UART_GET_FLAG(&m_uh, UART_FLAG_IDLE) ||
      __HAL_UART_GET_FLAG(&m_uh, UART_FLAG_ORE)) {
    volatile uint32_t tmp;
    tmp = m_uh.Instance->SR;
    tmp = m_uh.Instance->DR;
    (void)tmp;
  }
}

// recv complete callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    // puts("here");

    if (m_esp8266_qin) {
      BaseType_t shouldyield = pdFALSE;
      xQueueSendFromISR(m_esp8266_qin, &u1recv_byte, &shouldyield);
      portYIELD_FROM_ISR(shouldyield);
    }

    HAL_UART_Receive_IT(&m_uh, &u1recv_byte, 1);
  } else if (huart->Instance == USART3) {

    if (m_esp8266_qin) {
      BaseType_t shouldyield = pdFALSE;
      xQueueSendFromISR(m_esp8266_qin, &m_esp8266_recvbyte, &shouldyield);
      portYIELD_FROM_ISR(shouldyield);
    }

    HAL_UART_Receive_IT(&m_u3h, &m_esp8266_recvbyte, 1);
  }
}

// error callback
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1 || huart->Instance == USART3) {

    // clear error
    volatile uint32_t tmp;
    tmp = huart->Instance->SR;
    tmp = huart->Instance->DR;
    (void)tmp;

    if (huart->Instance == USART1) {
      HAL_UART_Receive_IT(&m_uh, &u1recv_byte, 1);
    } else {
      HAL_UART_Receive_IT(&m_u3h, &m_esp8266_recvbyte, 1);
    }
  }
}

//
// USART3 part

///@brief uart3 handler
UART_HandleTypeDef m_u3h;
///@brief esp8266-uart recv queue(ringbuff) instance, 256*1
QueueHandle_t m_esp8266_qin = NULL;
uint8_t m_esp8266_recvbyte;
SemaphoreHandle_t m_esp8266_senddone = NULL;

///@brief uart3-esp8266-s wifi port init, consists of peripheral config + queue
/// init + semaphore init + interrupt recv start
void uart3_init() {
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // init pb10-tx pb11-rx
  GPIO_InitTypeDef gi;
  gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_10,
      .Mode = GPIO_MODE_AF_PP,
      .Speed = GPIO_SPEED_FREQ_HIGH,
  };
  HAL_GPIO_Init(GPIOB, &gi);
  gi = (GPIO_InitTypeDef){
      .Pin = GPIO_PIN_11,
      .Mode = GPIO_MODE_INPUT,
      .Pull = GPIO_PULLUP,
  };
  HAL_GPIO_Init(GPIOB, &gi);

  // init uart3

  m_u3h = (UART_HandleTypeDef){
      .Instance = USART3,
      .Init =
          (UART_InitTypeDef){
              .BaudRate = USER_UART3_SPEED,
              .WordLength = UART_WORDLENGTH_8B,
              .StopBits = UART_STOPBITS_1,
              .Parity = UART_PARITY_NONE,
              .Mode = UART_MODE_TX_RX,
          },
  };
  assert(HAL_UART_Init(&m_u3h) == HAL_OK);
  HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);

  // init queue and semaphore

  if (!m_esp8266_qin) {
    m_esp8266_qin = xQueueCreate(256, 1);
    assert(m_esp8266_qin); // alway abort; thus recom init uart1 first
  }
  if (!m_esp8266_senddone) {
    m_esp8266_senddone = xSemaphoreCreateBinary();
    assert(m_esp8266_senddone);
  }

  // start interrupt recv
  puts("starting uart3 recv");
  HAL_StatusTypeDef res;
  res = HAL_UART_Receive_IT(&m_u3h, &m_esp8266_recvbyte, 1);
  assert(res == HAL_OK);
}

///@brief uart3 interrupt handler in NVIC table
void USART3_IRQHandler() { //
  HAL_UART_IRQHandler(&m_u3h);
}

///@brief esp8266 send byte complete callback
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {
    if (!m_esp8266_senddone)
      return;
    BaseType_t shouldyield = pdFALSE;
    xSemaphoreGiveFromISR(m_esp8266_senddone, &shouldyield);
    portYIELD_FROM_ISR(shouldyield);
  }
}

///@}
///@file uart_init.c
#include "initors.h"

#include "main.h"
#include "stm32_hal_legacy.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_dma.h"
#include "user_init/initors.h"

#include "stm32f103xe.h"
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

// gpio-uart1 init
///@brief uart1 handle
UART_HandleTypeDef m_uh = {0};
DMA_HandleTypeDef m_u1dma = {0};

#define SZ_DMA1C5_BUF 256
uint8_t m_u1dma_buf[SZ_DMA1C5_BUF];

SemaphoreHandle_t u1dma_sem = NULL;
static void u1dma_process(void *p);

///@brief uart1-default logger init, call in the main entry
void uart1_init() {
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

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

  // logging enabled here

  // rx dma init
  m_u1dma = (DMA_HandleTypeDef){
      .Instance = DMA1_Channel5,
      .Init =
          (DMA_InitTypeDef){
              .Direction = DMA_PERIPH_TO_MEMORY,
              .PeriphInc = DMA_PINC_DISABLE,
              .MemInc = DMA_MINC_ENABLE,
              .PeriphDataAlignment = DMA_PDATAALIGN_BYTE,
              .MemDataAlignment = DMA_MDATAALIGN_BYTE,
              .Mode = DMA_NORMAL,
              .Priority = DMA_PRIORITY_LOW,
          },
  };
  if (HAL_DMA_Init(&m_u1dma) != HAL_OK) {
    assert(0);
  }

  // relate dma to uart1 rx
  __HAL_LINKDMA(&m_uh, hdmarx, m_u1dma);

  // nvic config interrupt pri and enable
  HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

  // start dma rx
  HAL_StatusTypeDef res =
      HAL_UART_Receive_DMA(&m_uh, m_u1dma_buf, SZ_DMA1C5_BUF);

  __HAL_UART_ENABLE_IT(&m_uh, UART_IT_IDLE);

  // start processing task
  u1dma_sem = xSemaphoreCreateBinary();

  xTaskCreate(u1dma_process, "u1dma_process", 512, NULL,
              configMAX_PRIORITIES - 3, NULL);
}

// uart1 interrupt handlers
// irq handler
void UART1_IRQHandler() {
  HAL_UART_IRQHandler(&m_uh);
  if (__HAL_UART_GET_FLAG(&m_uh, UART_FLAG_IDLE)) {
    volatile uint32_t tmp;
    tmp = m_uh.Instance->SR;
    tmp = m_uh.Instance->DR;
    (void)tmp;

    puts("IDLE");

    if (u1dma_sem && 0) {
      BaseType_t yield_flag = pdFALSE;
      xSemaphoreGiveFromISR(u1dma_sem, &yield_flag);
      portYIELD_FROM_ISR(yield_flag);
    }
  }
}
// dma irq handler
void DMA1_Channel5_IRQHandler() { //
  HAL_DMA_IRQHandler(&m_u1dma);
}
// error callback
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    volatile uint32_t tmp;
    tmp = huart->Instance->SR;
    tmp = huart->Instance->DR;
    (void)tmp;
    HAL_UART_Receive_DMA(&m_uh, m_u1dma_buf, SZ_DMA1C5_BUF);
  }
}

// implementation of dma processing task
static void u1dma_process(void *p) {
  (void)p;
  size_t last_pos = 0;
  while (1) {
    if (xSemaphoreTake(u1dma_sem, portMAX_DELAY) == pdTRUE) {
      puts("u1dma_process: got sem\r\n");
    }
  }
}

///@brief uart3 handler
UART_HandleTypeDef m_u3h;
///@brief esp8266-uart recv queue(ringbuff) instance, 256*1
QueueHandle_t m_esp8266_qin = NULL;
volatile uint8_t m_esp8266_recvbyte;
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
              .BaudRate = 115200,
              .WordLength = UART_WORDLENGTH_8B,
              .StopBits = UART_STOPBITS_1,
              .Parity = UART_PARITY_NONE,
              .Mode = UART_MODE_TX_RX,
          },
  };
  assert(HAL_UART_Init(&m_u3h) == HAL_OK);
  HAL_NVIC_SetPriority(USART3_IRQn, 3, 0);
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
  // HAL_UART_Receive_IT(&m_u3h, (void *)&m_esp8266_recvbyte, 1);
}

///@brief uart3 interrupt handler in NVIC table
void UART3_IRQHandler() { //
  HAL_UART_IRQHandler(&m_u3h);
}

///@brief esp8266 recv byte complete callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {

    uint8_t b = m_esp8266_recvbyte;
    printf("recv byte%02x\r\n", b);

    // send to q
    if (m_esp8266_qin) {
      BaseType_t yeild_flag = pdFALSE;
      xQueueSendFromISR(m_esp8266_qin, &b, &yeild_flag);
      (void)yeild_flag;
    }

    HAL_UART_Receive_IT(&m_u3h, (void *)&m_esp8266_recvbyte, 1);
  }
}

///@brief esp8266 send byte complete callback
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {
    if (!m_esp8266_senddone)
      return;
    BaseType_t yield_flag = pdFALSE;
    xSemaphoreGiveFromISR(m_esp8266_senddone, &yield_flag);
    (void)yield_flag;
  }
}

///@}
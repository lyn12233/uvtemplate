///@file usart_init.c
#include "initors.h"

#include "stm32f1xx_hal_cortex.h"
#include "user_init/initors.h"

#include "stm32f103xe.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_usart.h"

#include "portmacro.h"

#include "log.h"
#include <stdint.h>

///@defgroup usart_init
///@ingroup user_init
///@{

// gpio-uart1 init
///@brief usart1 handle
USART_HandleTypeDef m_uh;
///@brief usart1-default logger init, call in the main entry
void usart1_init() {
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
      .Pull = GPIO_PULLUP,
  };
  HAL_GPIO_Init(GPIOA, &gi);

  m_uh = (USART_HandleTypeDef){
      .Instance = USART1,
      .Init =
          (USART_InitTypeDef){
              .BaudRate = 57600,
              .WordLength = USART_WORDLENGTH_8B,
              .StopBits = USART_STOPBITS_1,
              .Parity = USART_PARITY_NONE,
              .Mode = USART_MODE_TX_RX,
          },
  };
  HAL_USART_Init(&m_uh);
}

///@brief usart3 handler
USART_HandleTypeDef m_u3h;
///@brief esp8266-usart recv queue(ringbuff) instance, 256*1
QueueHandle_t m_esp8266_qin = NULL;
volatile uint8_t m_esp8266_recvbyte;
SemaphoreHandle_t m_esp8266_senddone = NULL;

///@brief usart3-esp8266-s wifi port init, consists of peripheral config + queue
/// init + semaphore init + interrupt recv start
void usart3_init() {
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

  // init usart3

  m_u3h = (USART_HandleTypeDef){
      .Instance = USART3,
      .Init =
          (USART_InitTypeDef){
              .BaudRate = 115200,
              .WordLength = USART_WORDLENGTH_8B,
              .StopBits = USART_STOPBITS_1,
              .Parity = USART_PARITY_NONE,
              .Mode = USART_MODE_TX_RX,
              .CLKPhase = USART_PHASE_1EDGE,
              .CLKPolarity = USART_POLARITY_LOW,
          },
  };
  assert(HAL_USART_Init(&m_u3h) == HAL_OK);
  HAL_NVIC_SetPriority(USART3_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);

  // init queue and semaphore

  if (!m_esp8266_qin) {
    m_esp8266_qin = xQueueCreate(256, 1);
    assert(m_esp8266_qin); // alway abort; thus recom init usart1 first
  }
  if (!m_esp8266_senddone) {
    m_esp8266_senddone = xSemaphoreCreateBinary();
    assert(m_esp8266_senddone);
  }

  // start interrupt recv
  // HAL_USART_Receive_IT(&m_u3h, (void *)&m_esp8266_recvbyte, 1);
}

///@brief usart3 interrupt handler in NVIC table
void USART3_IRQHandler() { //
  HAL_USART_IRQHandler(&m_u3h);
}

///@brief esp8266 recv byte complete callback
void HAL_USART_RxCpltCallback(USART_HandleTypeDef *husart) {
  if (husart->Instance == USART3) {

    uint8_t b = m_esp8266_recvbyte;
    printf("recv byte%02x\r\n", b);

    // send to q
    if (m_esp8266_qin) {
      BaseType_t yeild_flag = pdFALSE;
      xQueueSendFromISR(m_esp8266_qin, &b, &yeild_flag);
      (void)yeild_flag;
    }

    HAL_USART_Receive_IT(&m_u3h, (void *)&m_esp8266_recvbyte, 1);
  }
}

///@brief esp8266 send byte complete callback
void HAL_USART_TxCpltCallback(USART_HandleTypeDef *husart) {
  if (husart->Instance == USART3) {
    if (!m_esp8266_senddone)
      return;
    BaseType_t yield_flag = pdFALSE;
    xSemaphoreGiveFromISR(m_esp8266_senddone, &yield_flag);
    (void)yield_flag;
  }
}

void HAL_USART_ErrorCallback(USART_HandleTypeDef *husart) {
  if (husart->Instance == USART3) {
    debug("usart3 error callback\r\n");
    volatile uint32_t tmp;
    tmp = husart->Instance->SR;
    tmp = husart->Instance->DR;
    (void)tmp;
    HAL_USART_Receive_IT(&m_u3h, (void *)&m_esp8266_recvbyte, 1);
  }
}

///@}
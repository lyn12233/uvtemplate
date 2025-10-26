///@file usart_init.c
#include "initors.h"
#include "portmacro.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_rcc_ex.h"
#include "stm32f1xx_hal_usart.h"
#include "user_init/initors.h"

#include "log.h"

///@defgroup usart_init
///@ingroup user_init
///@{

// gpio-uart1 init
///@brief usart1 handle
USART_HandleTypeDef m_uh;
///@brief usart1-default logger init, call in the main entry
void usart1_init() {
  __HAL_RCC_USART1_CLK_ENABLE();
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
uint8_t m_esp8266_recvbyte;
SemaphoreHandle_t m_esp8266_senddone = NULL;

///@brief usart3-esp8266-s wifi port init, consists of peripheral config + queue
/// init + semaphore init + interrupt recv start
void usart3_init() {
  __HAL_RCC_USART3_CLK_ENABLE();
  m_u3h = (USART_HandleTypeDef){
      .Instance = USART3,
      .Init =
          (USART_InitTypeDef){
              .BaudRate = 115200,
              .WordLength = USART_WORDLENGTH_8B,
              .StopBits = USART_STOPBITS_1,
              .Parity = USART_PARITY_NONE,
              .Mode = USART_MODE_TX_RX,
          },
  };
  assert(HAL_USART_Init(&m_u3h) == HAL_OK);

  // init queue and semaphore
  if (!m_esp8266_qin) {
    m_esp8266_qin = xQueueCreate(256, 1);
    assert(m_esp8266_qin); // alway abort; thus recom init usart1 first
  }
  if (!m_esp8266_senddone) {
    m_esp8266_senddone = xSemaphoreCreateBinary();
    assert(m_esp8266_senddone);
  }
  // start receiving
  HAL_USART_Receive_IT(&m_u3h, &m_esp8266_recvbyte, 1);
}

// usart msp init

///@brief usart pin init, do not call directly
void HAL_USART_MspInit(USART_HandleTypeDef *husart) {
  GPIO_InitTypeDef gi = {0};

  if (husart->Instance == USART1) {
    // usart1: pa9 pa10 - tx rx
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure TX Pin
    gi.Pin = GPIO_PIN_9;
    gi.Mode = GPIO_MODE_AF_PP; // Alternate Function Push-Pull
    gi.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gi);

    // Configure RX Pin
    gi.Pin = GPIO_PIN_10;
    gi.Mode = GPIO_MODE_AF_INPUT; // Or GPIO_MODE_INPUT for RX
    HAL_GPIO_Init(GPIOA, &gi);

  } else if (husart->Instance == USART3) {
    // usart3: pb10 pb11 - tx rx
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gi.Pin = GPIO_PIN_10;
    gi.Mode = GPIO_MODE_AF_PP;
    gi.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gi);
    gi.Pin = GPIO_PIN_11;
    gi.Mode = GPIO_MODE_AF_INPUT;
    HAL_GPIO_Init(GPIOB, &gi);
  }
}

///@brief usart3 interrupt handler in NVIC table
void USART3_IRQHandler() { HAL_USART_IRQHandler(&m_u3h); }

///@brief esp8266 recv byte complete callback
void HAL_USART_RxCpltCallback(USART_HandleTypeDef *husart) {
  if (husart->Instance == USART3) {
    if (!m_esp8266_qin)
      return;
    xQueueSendFromISR(m_esp8266_qin, &m_esp8266_recvbyte, NULL);
    HAL_USART_Receive_IT(&m_u3h, &m_esp8266_recvbyte, 1);
  }
}

///@brief esp8266 send byte complete callback
void HAL_USART_TxCpltCallback(USART_HandleTypeDef *husart) {
  if (husart->Instance == USART3) {
    if (!m_esp8266_senddone)
      return;
    BaseType_t yield_flag = pdFALSE;
    xSemaphoreGiveFromISR(m_esp8266_senddone, &yield_flag);
  }
}

///@}
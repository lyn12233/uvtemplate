#include "initors.h"

// gpio-uart1 init
USART_HandleTypeDef m_uh;
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
void HAL_USART_MspInit(USART_HandleTypeDef *husart) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (husart->Instance == USART1) {
    __HAL_RCC_GPIOA_CLK_ENABLE(); // Enable GPIO clock (USART1 usually on
                                  // PA9/PA10)

    // Configure TX Pin
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // Alternate Function Push-Pull
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure RX Pin
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT; // Or GPIO_MODE_INPUT for RX
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}
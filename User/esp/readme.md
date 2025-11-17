## description on `esp/`

> NOTE: based on stm32f103ze, assume singleton usage, rely on freertos and hal_uart, use USART3(use uart api from hal)

ESP8266-1S module uart manager and socket api. ESP8266 send and receive AT commands via uart in a simplex style

- `espsock`: socket api based on esp-uart parser and executor env
- `exec`: AT command sender to ESP8266 via uart
- `parser`: burst AT message parser fron ESP8266 via uart, as a task, outputs to freertos queues(e.g. conn_recv, preaccepted); also manage esp8266 state and sender result(OK, ERROR)

configurations:
| macro               | current value | description                                                       |
| ------------------- | ------------- | ----------------------------------------------------------------- |
| ATC_SEND_CHUNK_SIZE | 1024          | CIPSEND data to send per time, should be less than 2048           |
| NB_SOCK             | 8             | number of accepted-socket backend components statically allocated |
| ATC_SENDRES_TIMEOUT | 1000ms        | timeout for AT command execution to await a result(OK,ERROR)      |

TODO: add lock in `exec`; fix synchronization issues(less frequent occurrence)
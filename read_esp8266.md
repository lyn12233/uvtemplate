# AT command format
in the format `AT\+[a-zA-Z,0-9]\r\n`
- test AT+<>=?
- query AT+<>?
- set AT+<>=<>
- exec AT+<>

# AT response format
in the format `\r\n<response>\r\n`
- OK
- ERROR
- SEND OK
- SEND FAIL
- +<>:<>
- `>`: wait for input

# example code 
```
AT+CWMODE=1

OK
AT+CWJAP="wifi","password"
...

OK
AT+CIPMUX //multi connection

OK
AT+CIPSERVER=1,<port> //start server

OK

0,CONNECT

+IPD,0,11:hellow world
AT+CIPSEND=0,13
>
hellow client
SEND OK

...

0,CLOSED
```
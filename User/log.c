#include "log.h"
#include "initors.h"
#include "stm32f1xx_hal_usart.h"

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

buff20_t itoa(int val, unsigned char radix) { return lltoa(val, radix); }
buff20_t lltoa(long long val, unsigned char radix) {
  buff20_t res = {0};
  if (radix < 10 || radix > 36) {
    res.str[0] = '?';
    return res;
  }
  if (val == 0) {
    res.str[0] = '0';
    return res;
  }
  uint8_t idx = 0;
  int8_t sign = val > 0;
  while (val != 0) { // log_{10}(2^{63})<19
    int8_t item = val % radix;
    item = item >= 0 ? item : -item;
    res.str[idx] = item <= 9 ? item + '0' : item - 10 + 'a';
    idx++;
    val /= 10;
  }
  if (!sign) {
    res.str[idx] = '-';
    idx++;
  }
  uint8_t start = 0;
  idx--;
  while (start < idx) {
    char tmp = res.str[idx];
    res.str[idx] = res.str[start];
    res.str[start] = tmp;
    idx--, start++;
  }
  return res;
}

buff20_t lftoa(double val, char width, char precision, char fill) {
  buff20_t res = {0};
  const char *ptrn = fill == ' ' ? "%*.*lf" : "%0*.*lf";
  snprintf(res.str, 20, ptrn, width, precision, val);
  return res;
}

int printf_v2(const char *s, ...) { //
  va_list args;
  va_start(args, s);

  char c;
  while (1) {
    c = *s, s++;
    if (c == '\0')
      break;
    if (c == '%' && *s != '%') {
      // format
      uint8_t left_align = 0, is_short = 0, is_l = 0, is_ll = 0;
      char fill = ' ';
      int8_t width = 0, precision = 0;
      // flags
      while (*s == '-' || *s == '+' || *s == ' ' || *s == '#' || *s == '0') {
        if (*s == '-')
          left_align = 1;
        if (*s == '0')
          fill = '0';
        s++;
      }
      // width
      if (*s == '*') {
        width = va_arg(args, int);
        s++;
      } else {
        while (*s >= '0' && *s <= '9') {
          if (width + 1 > INT8_MAX / 10)
            break;
          width = width * 10 + *s - '0';
          s++;
        }
      }
      width = left_align ? -width : width;
      // precision
      if (*s == '.') {
        s++;
        if (*s == '*') {
          precision = va_arg(args, int);
          s++;
        } else {
          precision = 0;
          while (*s >= '0' && *s <= '9') {
            if (precision + 1 > INT8_MAX / 10)
              break;
            precision = precision * 10 + *s - '0';
            s++;
          }
        }
      } else {
        precision = 6;
      }
      // length
      if (*s == 'h')
        is_short = 1, s++;
      if (*s == 'l' || *s == 'L')
        is_l = 1, s++;
      if (*s == 'l')
        is_ll = 1, s++;
      // type; only assures va_arg is accessed with the correct size
      c = *s, s++;
      c = c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
      buff20_t buff;
      if (c == 'c') {
        c = (char)va_arg(args, int);
        HAL_USART_Transmit(&m_uh, (void *)&c, 1, -1);
      } else if (c == 's') {
        const void *pc = va_arg(args, void *);
        pc = pc ? pc : "";
        HAL_USART_Transmit(&m_uh, pc, strnlen(pc, 1024), -1);
      } else if (c == 'p') {
        const void *p = va_arg(args, void *);
        // pass
      } else if (c == 'f' || c == 'e' || c == 'g' || c == 'a') {
        // float double and long double
        buff = is_ll ? lftoa(va_arg(args, long double), width, precision, fill)
                     : lftoa(va_arg(args, double), width, precision, fill);
        HAL_USART_Transmit(&m_uh, (void *)buff.str, strnlen(buff.str, 20), -1);
      } else if (c == 'd' || c == 'o' || c == 'u' || c == 'x') {
        // int long and long-long
        uint8_t radix = (c == 'd' || c == 'u') ? 10 : (c == 'o' ? 8 : 16);
        buff = is_ll  ? lltoa(va_arg(args, long long), radix)
               : is_l ? lltoa(va_arg(args, long), radix)
                      : lltoa(va_arg(args, int), radix);
        HAL_USART_Transmit(&m_uh, (void *)buff.str, strnlen(buff.str, 20), -1);
      }
    } else {
      // c!=0
      if (c == '%') //%%
        s++;
      HAL_USART_Transmit(&m_uh, (void *)&c, 1, -1);
    }
  }
  return 0;
}
int puts_v2(const char *s) { //
  HAL_USART_Transmit(&m_uh, (void *)s, strnlen(s, 1024), -1);
  return 0;
}
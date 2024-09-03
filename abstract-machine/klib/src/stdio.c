#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static void reverse(char *str, int len) {
  int start = 0;
  int end = len - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}

static int itoa(int value, char *buf, int base) {
  if (base < 2 || base > 36) {
    *buf = '\0';
    return 0;
  }
  int i = 0;
  bool isNegative = false;

  // 处理负数
  if (value < 0 && base == 10) {
    isNegative = true;
    value = -value;
  }

  // 转换数字
  do {
    int digit = value % base;
    buf[i++] = (digit > 9) ? (digit - 10) + 'a' : digit + '0';
    value = value / base;
  } while (value != 0);

  // 如果是负数，添加负号
  if (isNegative) {
    buf[i++] = '-';
  }

  // 添加字符串终止符
  buf[i] = '\0';

  // 反转字符串
  reverse(buf, i);

  return i;
}

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *ret = out;
  for (int i = 0; i < strlen(fmt); i++) {
    if (fmt[i] != '%') {
      *ret = fmt[i];
      ret++;
    } else {
      switch (fmt[i + 1]) {
        case '\0': // FIXME
          break;
        case 'd':
          ret += itoa(va_arg(ap, int), ret, 10);
          break;
        case 's': {
          char *s = va_arg(ap, char *);
          strcpy(ret, s);
          ret += strlen(ret);
          break;
        }
        default:
          panic("Not implemented");
      }
      i++;
    }
  }
  *ret = '\0';
  return ret - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  int result = 0;

  va_start(ap, fmt); // 初始化va_list
  result = vsprintf(out, fmt, ap);
  va_end(ap);
  return result;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif

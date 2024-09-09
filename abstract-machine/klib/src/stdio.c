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

#define PRINTF_BUFFER_LENGTH 4096
int printf(const char *fmt, ...) {
  char buffer[PRINTF_BUFFER_LENGTH];
  va_list ap;
  va_start(ap, fmt);
  int cnt = vsprintf(buffer, fmt, ap);
  int i = 0;
  while(buffer[i] != '\0') {
    putch(buffer[i]);
    i++;
  }
  return cnt;

}
#undef PRINTF_BUFFER_LENGTH

// static void strcpyf(char *dst, const char *src, int width) {
//   int len = strlen(dst);
//   int i = 0;
//   while (src[i] != '\0') {
//     dst[len + i] = src[i];
//     i++;
//   }
//   while (i < width) {
//     dst[len + i] = ' ';
//     i++;
//   }
//   dst[len + i] = '\0';
// }

#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_FORMAT 1
int vsprintf(char *out, const char *fmt, va_list ap) {
  char *ret = out;
  int state = PRINTF_STATE_NORMAL;
  int width = 0;
  for (int i = 0, j = 0; fmt[i] != '\0'; i++) {
    switch (state) {
      case(PRINTF_STATE_NORMAL): {
        if (fmt[i] == '%') {
          state = PRINTF_STATE_FORMAT;
        } else {
          *ret = fmt[i];
          ret++;
        }
        break;
      }
      case(PRINTF_STATE_FORMAT): {
        switch (fmt[i]) {
          case 'd': {
            int value = va_arg(ap, int);
            ret += itoa(value, ret, 10);
            state = PRINTF_STATE_NORMAL;
            break;
          }
          case 'x': {
            int value = va_arg(ap, int);
            ret += itoa(value, ret, 16);
            state = PRINTF_STATE_NORMAL;
            break;
          }
          case 's': {
            char *s = va_arg(ap, char *);
            strcpy(ret, s);
            ret += strlen(ret);
            state = PRINTF_STATE_NORMAL;
            break;
          }
          case 'c': {
            char t = va_arg(ap, int);
            ret[0] = t;
            ret += 1;
            state = PRINTF_STATE_NORMAL;
            break;
          }
          case '%': {
            *ret = '%';
            ret++;
            state = PRINTF_STATE_NORMAL;
            break;
          }
          case '0': {
            for (j = i + 1; fmt[j] != 'd' && fmt[j] != 's' && fmt[j] != 'c' && fmt[j] != 'x'; j++) {
              width = width * 10 + fmt[j] - '0';
              i++;
            }
            break;
          }
          default: {
            // putch(fmt[i]);
            // panic(" Format Not implemented"); // FIXME
            *ret = fmt[i];
            break;
          }
        
        }

      }
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

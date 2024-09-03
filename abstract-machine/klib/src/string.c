#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len_cnt = 0;
  while (s[len_cnt] != '\0') {
    len_cnt++;
  }
  return len_cnt;
}

size_t strnlen(const char *s, size_t n) {
  size_t len_cnt = 0;
  while (s[len_cnt] != '\0' && len_cnt < n) {
    len_cnt++;
  }
  return len_cnt;
}

char *strcpy(char *dst, const char *src) {
  char *p = NULL;
  p = memcpy(dst, src, strlen(src) + 1);
  p[strlen(src)] = '\0';
  return p;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t s_len = strnlen(src, n);
  size_t d_len = strlen(dst);
  char *p = memcpy(dst, src, s_len);
  if (s_len < d_len) {
    memset(p + s_len, '\0', d_len - s_len);
  }
  return p;
}

char *strcat(char *dst, const char *src) {
  memcpy(dst + strlen(dst), src, strlen(src) + 1);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  size_t s1_len = strlen(s1), s2_len = strlen(s2);
  size_t min_len = s1_len < s2_len ? s1_len : s2_len;
  return memcmp(s1, s2, min_len);
}

int strncmp(const char *s1, const char *s2, size_t n) {
  return memcmp(s1, s2, n);
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

// TODO: test
void *memmove(void *dst, const void *src, size_t n) {
  uint8_t tmp[n];
  memcpy(tmp, src, n);
  memcpy(dst, tmp, n);
  return dst;
}

// TODO: test
void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d  = (unsigned char *)out;
  const unsigned char *s = (const unsigned char *)in;
  // copy n bytes from s to d
  while (n--) {
    *d = *s;
    d++;
    s++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char* p1 = s1;
  const unsigned char* p2 = s2;
  // compare the n bytes
  while (n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

#endif

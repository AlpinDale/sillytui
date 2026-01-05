#include "core/time.h"
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <sys/time.h>
#include <windows.h>

static inline int gettimeofday_impl(struct timeval *tv, void *tz) {
  (void)tz;
  if (!tv)
    return -1;
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  unsigned long long time =
      ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  time /= 10;
  tv->tv_sec = (long)(time / 1000000UL);
  tv->tv_usec = (long)(time % 1000000UL);
  return 0;
}
#define gettimeofday gettimeofday_impl
#else
#include <sys/time.h>
#endif

void get_timestamp(char *buf, size_t buf_size) {
  if (!buf || buf_size == 0)
    return;

  struct timeval tv;
  gettimeofday(&tv, NULL);

  time_t tv_sec = (time_t)tv.tv_sec;
  struct tm *tm_info = localtime(&tv_sec);
  if (!tm_info) {
    buf[0] = '\0';
    return;
  }

  size_t written = strftime(buf, buf_size, "%H:%M:%S", tm_info);
  if (written > 0 && written < buf_size - 4) {
    snprintf(buf + written, buf_size - written, ".%03ld",
             (long)(tv.tv_usec / 1000));
  }
  if (buf_size > 0) {
    buf[buf_size - 1] = '\0';
  }
}

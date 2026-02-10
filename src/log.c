#include "log.h"

#include <stdio.h>
#include <stdarg.h>

void log_info(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf("[SERVER:INFO] ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
}

void log_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  printf("[SERVER:ERROR] ");
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
}

#include "../include/aglib_helpers.h"

void _err(const char* file, int line, const char* fmt, ...) {
  fprintf(stderr, "[ERROR] %s:%d: ", file, line);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

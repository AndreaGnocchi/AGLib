#ifndef AG_LIB_HELPERS
#define AG_LIB_HELPERS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define ERR(...)         _err(__FILE__, __LINE__, __VA_ARGS__)
#define MAX(v1, v2)     ((v1) > (v2) ? (v1) : (v2))
#define MIN(v1, v2)     ((v1) < (v2) ? (v1) : (v2))
#define SWAP(T, v1, v2) do { T temp = (v1); (v1) = (v2); (v2) = temp; } while (0)
#define TODO(message)   do { fprintf(stderr, "%s:%d: FUNC: %s TODO: %s \n", __FILE__, __LINE__, __func__, message); abort(); } while(0)
#define KB(n)           (n << 10)
#define MB(n)           (n << 20)
#define GB(n)           (n << 30)

void _err(const char* file, int line, const char* fmt, ...);
#endif // AG_LIB_HELPERS

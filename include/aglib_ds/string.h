#ifndef AG_LIB_DS_STRING
#define AG_LIB_DS_STRING

#include <string.h>
#include <stdarg.h>
#include "../../include/aglib_arena.h"
#include "array.h"

// ——— String —————————————————————————————————————————————————————————————————————————————————————

typedef struct {
    const char* ptr;
    size_t      len;
} sStringView;

#define String(name)                                                                              \
  DynamicArray(char, str##name)                                                                   \
  typedef str##name name;                                                                         \
                                                                                                  \
  static inline void name##_init(sArena* a, name* str, const char* init) {                        \
    if (!str || !a || !init) return;                                                              \
    size_t len = strlen(init);                                                                    \
    str##name##_init(a, str, len + 1);                                                            \
    str->size = len;                                                                              \
    memcpy(str->items, init, len);                                                                \
    str->items[len] = '\0';                                                                       \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_append(name* str, const char* toAppend) {                             \
    if (!str || !toAppend || !str->a) return false;                                               \
                                                                                                  \
    size_t appendLen = strlen(toAppend);                                                          \
    size_t needed    = str->size + appendLen + 1;                                                 \
                                                                                                  \
    if (needed > str->capacity) {                                                                 \
      size_t newCap = str->capacity ? str->capacity * 2 : needed;                                 \
      while (newCap < needed) newCap *= 2;                                                        \
                                                                                                  \
      char* newItems = (char*)arena_alloc(str->a, newCap, true);                                  \
      if (!newItems) return false;                                                                \
                                                                                                  \
      memcpy(newItems, str->items, str->size);                                                    \
      str->items    = newItems;                                                                   \
      str->capacity = newCap;                                                                     \
    }                                                                                             \
                                                                                                  \
    memcpy(str->items + str->size, toAppend, appendLen);                                          \
    str->size += appendLen;                                                                       \
    str->items[str->size] = '\0';                                                                 \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline sStringView name##_slice(name* str, size_t start, size_t len) {                   \
    if (!str || !str->items) return (sStringView){0};                                             \
    if (start >= str->size || len == 0 || start + len > str->size)                                 \
      return (sStringView){0};                                                                    \
                                                                                                  \
    return (sStringView){ .ptr = str->items + start, .len = len };                                \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_appendf(sArena* a, name* str, const char* fmt, ...) {                 \
    if (!a || !str || !fmt) return false;                                                         \
                                                                                                  \
    va_list args;                                                                                 \
    va_start(args, fmt);                                                                          \
    int len = vsnprintf(NULL, 0, fmt, args);                                                      \
    va_end(args);                                                                                 \
                                                                                                  \
    if (len < 0) return false;                                                                    \
                                                                                                  \
    size_t needed = str->size + (size_t)len + 1;                                                  \
    if (needed > str->capacity) {                                                                 \
      size_t newCap = str->capacity ? str->capacity * 2 : needed;                                 \
      while (newCap < needed) newCap *= 2;                                                        \
                                                                                                  \
      char* newItems = (char*)arena_alloc(a, newCap, true);                                       \
      if (!newItems) return false;                                                                \
                                                                                                  \
      if (str->items && str->size > 0) {                                                          \
        memcpy(newItems, str->items, str->size);                                                  \
      }                                                                                           \
                                                                                                  \
      str->items    = newItems;                                                                   \
      str->capacity = newCap;                                                                     \
    }                                                                                             \
                                                                                                  \
    va_start(args, fmt);                                                                          \
    vsnprintf(str->items + str->size, (size_t)len + 1, fmt, args);                                \
    va_end(args);                                                                                 \
    str->size += (size_t)len;                                                                     \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline const char* name##_cstr(name* str) {                                              \
    if (!str || !str->items) return NULL;                                                         \
    return (const char*)str->items;                                                               \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* str) {                                                    \
    str##name##_clear(str);                                                                       \
  }                                                                                               \


#endif // AG_LIB_DS_STRING

#ifndef AG_LIB_DS_STRING
#define AG_LIB_DS_STRING

#include <string.h>
#include <stdarg.h>
#include "../../include/aglib_allocator.h"
#include "array.h"

// ——— String —————————————————————————————————————————————————————————————————————————————————————

typedef struct {
    const char* ptr;
    size_t      len;
} sStringView;

#define String(name)                                                                        \
  DynamicArray(char, str##name)                                                             \
  typedef str##name name;                                                                   \
                                                                                            \
  static inline bool name##_init(sAllocator* a, name* str, const char* init) {              \
    if (!str || !a || !init) return false;                                                  \
    size_t len = strlen(init);                                                              \
    if (!str##name##_init(a, str, len + 1)) return false;                                   \
                                                                                            \
    str->size = len;                                                                        \
    memcpy(str->items, init, len);                                                          \
    str->items[len] = '\0';                                                                 \
                                                                                            \
    return true;                                                                            \
  }                                                                                         \
                                                                                            \
  static inline void name##_destroy(name* str) {                                            \
    str##name##_destroy(str);                                                               \
  }                                                                                         \
                                                                                            \
  static inline bool name##_append(name* str, const char* toAppend) {                       \
    if (!str || !toAppend || !str->a) return false;                                         \
                                                                                            \
    EnterCriticalSection(&str->lock);                                                       \
                                                                                            \
    size_t appendLen = strlen(toAppend);                                                    \
    size_t needed    = str->size + appendLen + 1;                                           \
                                                                                            \
    if (needed > str->capacity) {                                                           \
      size_t newCap = str->capacity ? str->capacity * 2 : needed;                           \
      while (newCap < needed) newCap *= 2;                                                  \
                                                                                            \
      char* newItems = (char*)ag_realloc(str->a, str->items, str->capacity, newCap, true);  \
      if (!newItems) {                                                                      \
        LeaveCriticalSection(&str->lock);                                                   \
        return false;                                                                       \
      }                                                                                     \
                                                                                            \
      str->items    = newItems;                                                             \
      str->capacity = newCap;                                                               \
    }                                                                                       \
                                                                                            \
    memcpy(str->items + str->size, toAppend, appendLen);                                    \
    str->size += appendLen;                                                                 \
    str->items[str->size] = '\0';                                                           \
                                                                                            \
    LeaveCriticalSection(&str->lock);                                                       \
    return true;                                                                            \
  }                                                                                         \
                                                                                            \
  static inline sStringView name##_slice(name* str, size_t start, size_t len) {             \
    if (!str || !str->items) return (sStringView){0};                                       \
                                                                                            \
    EnterCriticalSection(&str->lock);                                                       \
    if (start >= str->size || len == 0 || start + len > str->size) {                         \
      LeaveCriticalSection(&str->lock);                                                     \
      return (sStringView){0};                                                              \
    }                                                                                       \
                                                                                            \
    sStringView view = (sStringView){ .ptr = str->items + start, .len = len };              \
    LeaveCriticalSection(&str->lock);                                                       \
    return view;                                                                            \
  }                                                                                         \
                                                                                            \
  static inline bool name##_appendf(sAllocator* a, name* str, const char* fmt, ...) {       \
    if (!a || !str || !fmt) return false;                                                   \
                                                                                            \
    va_list args;                                                                           \
    va_start(args, fmt);                                                                    \
    int len = vsnprintf(NULL, 0, fmt, args);                                                \
    va_end(args);                                                                           \
                                                                                            \
    if (len < 0) return false;                                                              \
                                                                                            \
    EnterCriticalSection(&str->lock);                                                       \
                                                                                            \
    size_t needed = str->size + (size_t)len + 1;                                            \
    if (needed > str->capacity) {                                                           \
      size_t newCap = str->capacity ? str->capacity * 2 : needed;                           \
      while (newCap < needed) newCap *= 2;                                                  \
                                                                                            \
      char* newItems = (char*)ag_realloc(a, str->items, str->capacity, newCap, true);       \
      if (!newItems) {                                                                      \
        LeaveCriticalSection(&str->lock);                                                   \
        return false;                                                                       \
      }                                                                                     \
                                                                                            \
      str->items    = newItems;                                                             \
      str->capacity = newCap;                                                               \
    }                                                                                       \
                                                                                            \
    va_start(args, fmt);                                                                    \
    vsnprintf(str->items + str->size, (size_t)len + 1, fmt, args);                          \
    va_end(args);                                                                           \
    str->size += (size_t)len;                                                               \
                                                                                            \
    LeaveCriticalSection(&str->lock);                                                       \
    return true;                                                                            \
  }                                                                                         \
                                                                                            \
  static inline const char* name##_cstr(name* str) {                                        \
    if (!str || !str->items) return NULL;                                                   \
                                                                                            \
    EnterCriticalSection(&str->lock);                                                       \
    const char* p = (const char*)str->items;                                                \
    LeaveCriticalSection(&str->lock);                                                       \
    return p;                                                                               \
  }                                                                                         \
                                                                                            \
  static inline void name##_clear(name* str) {                                              \
    str##name##_clear(str);                                                                 \
  }                                                                                         \
                                                                                            \
  static inline void name##_free(name* str) {                                               \
    str##name##_free(str);                                                                  \
  }                                                                                         \

#endif // AG_LIB_DS_STRING

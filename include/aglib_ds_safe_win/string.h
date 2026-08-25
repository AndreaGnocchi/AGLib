#ifndef AG_LIB_DS_STRING
#define AG_LIB_DS_STRING

#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "../../include/aglib_allocator.h"
#include "array.h"

#ifndef AG_SHORTEST_STRING
#define AG_SHORTEST_STRING 32
#endif

// ——— String —————————————————————————————————————————————————————————————————————————————————————

#define String(name)                                                                              \
  typedef bool (*name##SearchCallback)(size_t idx, void* userdata);                               \
                                                                                                  \
  DynamicArray(char, str##name)                                                                   \
  typedef str##name name;                                                                         \
                                                                                                  \
  static inline bool name##_init(sAllocator* a, name* str, const char* init) {                    \
    if (!str || !a || !init) return false;                                                        \
                                                                                                  \
    size_t len = strlen(init);                                                                    \
    size_t initCap = (len + 1 > AG_SHORTEST_STRING) ? len + 1 : AG_SHORTEST_STRING;               \
    if (!str##name##_init(a, str, initCap)) return false;                                         \
                                                                                                  \
    str->size = len;                                                                              \
    memcpy(str->items, init, len);                                                                \
    str->items[len] = '\0';                                                                       \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_append(name* str, const char* toAppend) {                             \
    if (!str || !toAppend || !str->a) return false;                                               \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t appendLen = strlen(toAppend);                                                          \
    size_t needed    = str->size + appendLen + 1;                                                 \
                                                                                                  \
    if (needed > str->capacity) {                                                                 \
      size_t newCap = str->capacity ? str->capacity * 2 : needed;                                 \
      while (newCap < needed) newCap *= 2;                                                        \
                                                                                                  \
      char* newItems = (char*)ag_realloc(str->a, str->items, str->capacity, newCap, true);        \
      if (!newItems) {                                                                            \
        LeaveCriticalSection(&str->lock);                                                         \
        return false;                                                                             \
      }                                                                                           \
                                                                                                  \
      str->items    = newItems;                                                                   \
      str->capacity = newCap;                                                                     \
    }                                                                                             \
                                                                                                  \
    memcpy(str->items + str->size, toAppend, appendLen);                                          \
    str->size += appendLen;                                                                       \
    str->items[str->size] = '\0';                                                                 \
                                                                                                  \
    LeaveCriticalSection(&str->lock);                                                             \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_appendf(name* str, const char* fmt, ...) {                            \
    if (!str || !str->a || !fmt) return false;                                                    \
                                                                                                  \
    va_list args;                                                                                 \
    va_start(args, fmt);                                                                          \
    int len = vsnprintf(NULL, 0, fmt, args);                                                      \
    va_end(args);                                                                                 \
                                                                                                  \
    if (len < 0) return false;                                                                    \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t needed = str->size + (size_t)len + 1;                                                  \
    if (needed > str->capacity) {                                                                 \
      size_t newCap = str->capacity ? str->capacity * 2 : needed;                                 \
      while (newCap < needed) newCap *= 2;                                                        \
                                                                                                  \
      char* newItems = (char*)ag_realloc(str->a, str->items, str->capacity, newCap, true);        \
      if (!newItems) {                                                                            \
        LeaveCriticalSection(&str->lock);                                                         \
        return false;                                                                             \
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
    LeaveCriticalSection(&str->lock);                                                             \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline void name##_to_lower(name* str) {                                                 \
    if (!str || !str->items) return;                                                              \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
    for (size_t i = 0; i < str->size; ++i) {                                                      \
      char c = str->items[i];                                                                     \
                                                                                                  \
      if (c >= 'A' && c <= 'Z')                                                                     \
        str->items[i] = c + 32;                                                                   \
    }                                                                                             \
    LeaveCriticalSection(&str->lock);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_to_upper(name* str) {                                                 \
    if (!str || !str->items) return;                                                              \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
    for (size_t i = 0; i < str->size; ++i) {                                                      \
      char c = str->items[i];                                                                     \
                                                                                                  \
      if (c >= 'a' && c <= 'z')                                                                     \
        str->items[i] = c - 32;                                                                   \
    }                                                                                             \
    LeaveCriticalSection(&str->lock);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_normalize(name* str) {                                                \
    if (!str || !str->items || str->size == 0) return;                                            \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t written      = 0;                                                                      \
    size_t read         = 0;                                                                      \
    bool   capNext      = true;                                                                   \
    bool   pendingSpace = false;                                                                  \
                                                                                                  \
    while (read < str->size && (str->items[read] == ' '  || str->items[read] == '\t' ||           \
                                str->items[read] == '\n' || str->items[read] == '\r' ))           \
      read++;                                                                                     \
                                                                                                  \
    for (; read < str->size; ++read) {                                                            \
      char c       = str->items[read];                                                            \
      bool isSpace = (c == ' ' || c == '\t' || c == '\n' || c == '\r');                           \
                                                                                                  \
      if (isSpace) {                                                                              \
        pendingSpace = true;                                                                      \
        continue;                                                                                 \
      }                                                                                           \
                                                                                                  \
      if (pendingSpace && written > 0) {                                                          \
        str->items[written++] = ' ';                                                              \
      }                                                                                           \
      pendingSpace = false;                                                                       \
                                                                                                  \
      if (capNext && c >= 'a' && c <= 'z')                                                          \
        c -= 32;                                                                                  \
      if (!capNext && c >= 'A' && c <= 'Z')                                                         \
        c += 32;                                                                                  \
                                                                                                  \
      str->items[written++] = c;                                                                  \
      capNext = (c == '.' || c == '!' || c == '?');                                               \
    }                                                                                             \
                                                                                                  \
    str->size           = written;                                                                \
    str->items[written] = '\0';                                                                   \
                                                                                                  \
    LeaveCriticalSection(&str->lock);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_search_first_from(name* str, const char* needle,                      \
                                             size_t startFrom, size_t* outIdx) {                  \
    if (!str || !str->items || str->size == 0 || !needle || !outIdx) return false;                \
    if (startFrom > str->size)                                       return false;                \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t needleLen = strlen(needle);                                                            \
    if (needleLen == 0 || needleLen > str->size) {                                                \
      LeaveCriticalSection(&str->lock);                                                           \
      return false;                                                                               \
    }                                                                                             \
    if (startFrom > str->size - needleLen) {                                                      \
      LeaveCriticalSection(&str->lock);                                                           \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    size_t maxStart = str->size - needleLen;                                                      \
    for (size_t i = startFrom; i <= maxStart; ++i) {                                               \
      size_t idx = 0;                                                                             \
      while (idx < needleLen && str->items[i + idx] == needle[idx])                               \
        ++idx;                                                                                    \
                                                                                                  \
      if (idx == needleLen) {                                                                     \
        *outIdx = i;                                                                              \
        LeaveCriticalSection(&str->lock);                                                         \
        return true;                                                                              \
      }                                                                                           \
    }                                                                                             \
                                                                                                  \
    LeaveCriticalSection(&str->lock);                                                             \
    return false;                                                                                 \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_search_first(name* str, const char* needle, size_t* outIdx) {         \
    return name##_search_first_from(str, needle, 0, outIdx);                                      \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_search_last(name* str, const char* needle, size_t* outIdx) {          \
    if (!str || !str->items || str->size == 0 || !needle || !outIdx) return false;                \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t needleLen = strlen(needle);                                                            \
    if (needleLen == 0 || needleLen > str->size) {                                                \
      LeaveCriticalSection(&str->lock);                                                           \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    int64_t i = (int64_t)str->size - (int64_t)needleLen;                                          \
                                                                                                  \
    while (i >= 0) {                                                                               \
      size_t idx = 0;                                                                             \
      while (idx < needleLen && str->items[(size_t)i + idx] == needle[idx]) {                     \
        ++idx;                                                                                    \
      }                                                                                           \
                                                                                                  \
      if (idx == needleLen) {                                                                     \
        *outIdx = (size_t)i;                                                                      \
        LeaveCriticalSection(&str->lock);                                                         \
        return true;                                                                              \
      }                                                                                           \
                                                                                                  \
      --i;                                                                                        \
    }                                                                                             \
                                                                                                  \
    LeaveCriticalSection(&str->lock);                                                             \
    return false;                                                                                 \
  }                                                                                               \
                                                                                                  \
  static inline void name##_search_all(name* str, const char* needle,                             \
                                       name##SearchCallback cb, void* userdata) {                 \
    if (!str || !str->items || str->size == 0 || !needle || !cb) return;                          \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t needleLen = strlen(needle);                                                            \
    if (needleLen == 0 || needleLen > str->size) {                                                \
      LeaveCriticalSection(&str->lock);                                                           \
      return;                                                                                     \
    }                                                                                             \
                                                                                                  \
    size_t maxStart = str->size - needleLen;                                                      \
    size_t i = 0;                                                                                 \
                                                                                                  \
    while (i <= maxStart) {                                                                        \
      size_t idx = 0;                                                                             \
      while (idx < needleLen && str->items[i + idx] == needle[idx]) {                             \
        ++idx;                                                                                    \
      }                                                                                           \
                                                                                                  \
      if (idx == needleLen) {                                                                     \
        cb(i, userdata);                                                                          \
        i += needleLen;                                                                           \
      } else {                                                                                    \
        ++i;                                                                                      \
      }                                                                                           \
    }                                                                                             \
                                                                                                  \
    LeaveCriticalSection(&str->lock);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline bool _##name##_replace_impl(name* str, const char* needle, const char* rep,       \
                                         size_t outIdx) {                                         \
    size_t needleLen = strlen(needle);                                                            \
    size_t repLen    = strlen(rep);                                                               \
    size_t diff      = (repLen > needleLen) ? (repLen - needleLen) : (needleLen - repLen);        \
                                                                                                  \
    if (repLen > needleLen) {                                                                     \
      size_t needed = str->size + diff + 1;                                                       \
      if (needed > str->capacity) {                                                               \
        size_t newCap = str->capacity ? str->capacity * 2 : needed;                               \
        while (newCap < needed) newCap *= 2;                                                      \
                                                                                                  \
        char* newItems = (char*)ag_realloc(str->a, str->items, str->capacity, newCap, true);      \
        if (!newItems) return false;                                                              \
                                                                                                  \
        str->items    = newItems;                                                                 \
        str->capacity = newCap;                                                                   \
      }                                                                                           \
                                                                                                  \
      memmove(&str->items[outIdx + repLen], &str->items[outIdx + needleLen],                      \
              str->size - (outIdx + needleLen));                                                  \
      str->size += diff;                                                                          \
    } else if (repLen < needleLen) {                                                              \
      memmove(&str->items[outIdx + repLen], &str->items[outIdx + needleLen],                      \
              str->size - (outIdx + needleLen));                                                  \
      str->size -= diff;                                                                          \
    }                                                                                             \
                                                                                                  \
    memcpy(&str->items[outIdx], rep, repLen);                                                     \
    str->items[str->size] = '\0';                                                                 \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_replace_first(name* str, const char* needle, const char* rep) {       \
    if (!str || !str->a || !str->items || str->size == 0 || !needle || !rep) return false;        \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t outIdx = 0;                                                                            \
    if (!name##_search_first(str, needle, &outIdx)) {                                             \
      LeaveCriticalSection(&str->lock);                                                           \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    bool ok = _##name##_replace_impl(str, needle, rep, outIdx);                                   \
    LeaveCriticalSection(&str->lock);                                                             \
    return ok;                                                                                    \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_replace_last(name* str, const char* needle, const char* rep) {        \
    if (!str || !str->a || !str->items || str->size == 0 || !needle || !rep) return false;        \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t outIdx = 0;                                                                            \
    if (!name##_search_last(str, needle, &outIdx)) {                                              \
      LeaveCriticalSection(&str->lock);                                                           \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    bool ok = _##name##_replace_impl(str, needle, rep, outIdx);                                   \
    LeaveCriticalSection(&str->lock);                                                             \
    return ok;                                                                                    \
  }                                                                                               \
                                                                                                  \
  static inline size_t name##_replace_all(name* str, const char* needle, const char* rep) {       \
    if (!str || !str->a || !str->items || str->size == 0 || !needle || !rep) return 0;            \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
                                                                                                  \
    size_t repLen = strlen(rep);                                                                  \
    size_t outIdx = 0;                                                                            \
    size_t count  = 0;                                                                            \
                                                                                                  \
    while (name##_search_first_from(str, needle, outIdx, &outIdx)) {                              \
      if (!_##name##_replace_impl(str, needle, rep, outIdx)) break;                               \
                                                                                                  \
      count++;                                                                                    \
      outIdx += repLen;                                                                           \
    }                                                                                             \
                                                                                                  \
    LeaveCriticalSection(&str->lock);                                                             \
    return count;                                                                                 \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_cstr_copy(name* str, char* buf, size_t bufSize) {                     \
    if (!str || !str->items || !buf || bufSize == 0) return false;                                \
                                                                                                  \
    EnterCriticalSection(&str->lock);                                                             \
    size_t n = (str->size < bufSize - 1) ? str->size : bufSize - 1;                               \
    memcpy(buf, str->items, n);                                                                   \
    buf[n] = '\0';                                                                                \
    LeaveCriticalSection(&str->lock);                                                             \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* str) {                                                    \
    str##name##_clear(str);                                                                       \
  }                                                                                               \
                                                                                                  \
  static inline void name##_free(name* str) {                                                     \
    str##name##_free(str);                                                                        \
  }                                                                                               \

#endif // AG_LIB_DS_STRING

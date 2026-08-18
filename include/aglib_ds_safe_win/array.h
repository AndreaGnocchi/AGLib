#ifndef AG_LIB_DS_ARRAY
#define AG_LIB_DS_ARRAY

#include <string.h>
#include <stdint.h>
#include <windows.h>
#include "../../include/aglib_allocator.h"

// ——— Dynamic array ——————————————————————————————————————————————————————————————————————————————

#define DynamicArray(T, name)                                                      \
  typedef struct {                                                                 \
    T*              items;                                                         \
    size_t          capacity;                                                      \
    size_t          size;                                                          \
    sAllocator*     a;                                                             \
    CRITICAL_SECTION lock;                                                         \
  } name;                                                                          \
                                                                                   \
  static inline bool name##_init(sAllocator* a, name* arr, size_t initCap) {       \
    if (!a || !arr || initCap == 0 || a->type == SLAB) return false;               \
                                                                                   \
    arr->items = (T*)ag_alloc(a, sizeof(T) * initCap, true);                       \
    if (!arr->items) return false;                                                 \
                                                                                   \
    arr->a        = a;                                                             \
    arr->capacity = initCap;                                                       \
    arr->size     = 0;                                                             \
                                                                                   \
    InitializeCriticalSection(&arr->lock);                                         \
                                                                                   \
    return true;                                                                   \
  }                                                                                \
                                                                                   \
  static inline void name##_destroy(name* arr) {                                   \
    if (!arr) return;                                                              \
    DeleteCriticalSection(&arr->lock);                                             \
  }                                                                                \
                                                                                   \
  static inline bool name##_push(name* arr, T items) {                             \
    if (!arr || !arr->a) return false;                                             \
                                                                                   \
     EnterCriticalSection(&arr->lock);                                             \
                                                                                   \
    if (arr->size >= arr->capacity) {                                               \
      if (arr->capacity > SIZE_MAX / 2) {                                          \
        LeaveCriticalSection(&arr->lock);                                          \
        return false;                                                              \
      }                                                                            \
                                                                                   \
      size_t newCap   = arr->capacity * 2;                                         \
      T* newItems = (T*)ag_realloc(arr->a, arr->items, sizeof(T) * arr->capacity,  \
                                   sizeof(T) * newCap, true);                      \
      if (!newItems) {                                                             \
        LeaveCriticalSection(&arr->lock);                                          \
        return false;                                                              \
      }                                                                            \
                                                                                   \
      arr->items    = newItems;                                                    \
      arr->capacity = newCap;                                                      \
    }                                                                              \
                                                                                   \
    arr->items[arr->size++] = items;                                               \
    LeaveCriticalSection(&arr->lock);                                              \
    return true;                                                                   \
  }                                                                                \
                                                                                   \
  static inline bool name##_is_empty(name* arr) {                                  \
    if (!arr) return true;                                                         \
                                                                                   \
    EnterCriticalSection(&arr->lock);                                              \
    bool empty = arr->size == 0;                                                   \
    LeaveCriticalSection(&arr->lock);                                              \
    return empty;                                                                  \
  }                                                                                \
                                                                                   \
  static inline void name##_clear(name* arr) {                                     \
    if (!arr) return;                                                              \
                                                                                   \
    EnterCriticalSection(&arr->lock);                                              \
    memset(arr->items, 0, sizeof(T) * arr->capacity);                              \
    arr->size = 0;                                                                 \
    LeaveCriticalSection(&arr->lock);                                              \
  }                                                                                \
                                                                                   \
  static inline void name##_free(name* arr) {                                      \
    if (!arr) return;                                                              \
                                                                                   \
    EnterCriticalSection(&arr->lock);                                              \
    ag_free(arr->a, arr->items);                                                   \
    arr->items    = NULL;                                                          \
    arr->capacity = 0;                                                             \
    arr->size     = 0;                                                             \
    arr->a        = NULL;                                                          \
    LeaveCriticalSection(&arr->lock);                                              \
                                                                                   \
    DeleteCriticalSection(&arr->lock);                                             \
  }                                                                                \

#endif // AG_LIB_DS_ARRAY

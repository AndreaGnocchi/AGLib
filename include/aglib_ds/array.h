#ifndef AG_LIB_DS_ARRAY
#define AG_LIB_DS_ARRAY

#include <string.h>
#include <stdint.h>
#include "../../include/aglib_allocator.h"

// ——— Dynamic array ——————————————————————————————————————————————————————————————————————————————

#define DynamicArray(T, name)                                                                     \
  typedef struct {                                                                                \
    T*      items;                                                                                \
    size_t  capacity;                                                                             \
    size_t  size;                                                                                 \
    sAllocator* a;                                                                                \
  } name;                                                                                         \
                                                                                                  \
                                                                                                  \
  static inline bool name##_init(sAllocator* a, name* arr, size_t initCap) {                      \
    if (!a || !arr || initCap == 0 || a->type == SLAB) return false;                              \
                                                                                                  \
    arr->items = (T*)ag_alloc(a, sizeof(T) * initCap, true);                                      \
    if (!arr->items) return false;                                                                \
                                                                                                  \
    arr->a        = a;                                                                            \
    arr->capacity = initCap;                                                                      \
    arr->size     = 0;                                                                            \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push(name* arr, T items) {                                            \
    if (!arr || !arr->a) return false;                                                            \
                                                                                                  \
    if (arr->size >= arr->capacity) {                                                              \
      if (arr->capacity > SIZE_MAX / 2) return false;                                             \
                                                                                                  \
      size_t newCap   = arr->capacity * 2;                                                        \
      T* newItems = (T*)ag_realloc(arr->a, arr->items, sizeof(T) * arr->capacity,                 \
                                   sizeof(T) * newCap, true);                                     \
      if (!newItems) return false;                                                                \
                                                                                                  \
      arr->items    = newItems;                                                                   \
      arr->capacity = newCap;                                                                     \
    }                                                                                             \
                                                                                                  \
    arr->items[arr->size++] = items;                                                              \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_pop(name* arr, T* outVal) {                                           \
    if (!arr || !arr->items || arr->size == 0) return false;                                      \
                                                                                                  \
    arr->size--;                                                                                  \
    if (outVal) *outVal = arr->items[arr->size];                                                  \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* arr) {                                                 \
    if (!arr) return true;                                                                        \
    return arr->size == 0;                                                                        \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* arr) {                                                    \
    if (!arr || !arr->items) return;                                                              \
                                                                                                  \
    arr->size = 0;                                                                                \
  }                                                                                               \
                                                                                                  \
  static inline void name##_free(name* arr) {                                                     \
    if (!arr) return;                                                                             \
                                                                                                  \
    ag_free(arr->a, arr->items);                                                                  \
                                                                                                  \
    arr->items    = NULL;                                                                         \
    arr->capacity = 0;                                                                            \
    arr->size     = 0;                                                                            \
    arr->a        = NULL;                                                                         \
  }                                                                                               \

#endif // AG_LIB_DS_ARRAY
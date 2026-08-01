#ifndef AG_LIB_DS_ARRAY
#define AG_LIB_DS_ARRAY

#include <string.h>
#include "../../include/aglib_arena.h"

// ——— Dynamic array ——————————————————————————————————————————————————————————————————————————————

#define DynamicArray(T, name)                                                                     \
  typedef struct {                                                                                \
    T*      items;                                                                                \
    size_t  capacity;                                                                             \
    size_t  size;                                                                                 \
    sArena* a;                                                                                    \
  } name;                                                                                         \
                                                                                                  \
  static inline void name##_init(sArena* a, name* arr, size_t initCap) {                          \
    if (!a || !arr || initCap == 0) return;                                                       \
                                                                                                  \
    arr->a        = a;                                                                            \
    arr->capacity = initCap;                                                                      \
    arr->size     = 0;                                                                            \
    arr->items    = (T*)arena_alloc(a, sizeof(T) * initCap, true);                                \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push(name* arr, T items) {                                            \
    if (!arr->a) return false;                                                                    \
                                                                                                  \
    if (arr->size >= arr->capacity) {                                                              \
      size_t newCap   = arr->capacity * 2;                                                        \
      T*     newItems = (T*)arena_alloc(arr->a, sizeof(T) * newCap, true);                        \
      if (!newItems) return false;                                                                \
                                                                                                  \
      memcpy(newItems, arr->items, sizeof(T) * arr->size);                                        \
      arr->items    = newItems;                                                                   \
      arr->capacity = newCap;                                                                     \
    }                                                                                             \
                                                                                                  \
    arr->items[arr->size++] = items;                                                              \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* arr) {                                                 \
    if (!arr) return false;                                                                       \
    return arr->size == 0;                                                                        \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* arr) {                                                    \
    if (!arr) return;                                                                             \
                                                                                                  \
    memset(arr->items, 0, sizeof(T) * arr->capacity);                                             \
    arr->size = 0;                                                                                \
  }                                                                                               \



#endif // AG_LIB_DS_ARRAY

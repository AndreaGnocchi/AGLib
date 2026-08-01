#ifndef AG_LIB_DS_HEAP
#define AG_LIB_DS_HEAP

#include <string.h>
#include "../../include/aglib_arena.h"
#include "array.h"

// ——— Heap ———————————————————————————————————————————————————————————————————————————————————————
 
#define Heap(T, name, cmp)                                                                        \
  DynamicArray(T, Heap##name)                                                                     \
  typedef Heap##name name;                                                                        \
                                                                                                  \
  static inline void name##_init(sArena* a, name* h, size_t initCap) {                            \
    Heap##name##_init(a, h, initCap);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void _##name##_heap_swap(name* h, size_t i, size_t j) {                           \
    T tmp = h->items[i]; h->items[i] = h->items[j]; h->items[j] = tmp;                            \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push(name* h, T val) {                                                \
    if (!Heap##name##_push(h, val)) return false;                                                 \
                                                                                                  \
    size_t i = h->size - 1;                                                                       \
    while (i > 0) {                                                                               \
      size_t p = (i - 1) / 2;                                                                     \
      if (!cmp(h->items[i], h->items[p])) break;                                                  \
      _##name##_heap_swap(h, i, p);                                                               \
      i = p;                                                                                      \
    }                                                                                             \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_pop(name* h, T* outVal) {                                             \
    if (!h || h->size == 0) return false;                                                         \
                                                                                                  \
    if (outVal)                                                                                   \
      *outVal   = h->items[0];                                                                    \
    h->items[0] = h->items[--h->size];                                                            \
                                                                                                  \
    size_t i = 0;                                                                                 \
    while (1) {                                                                                   \
      size_t l = 2 * i + 1, r = 2 * i + 2, best = i;                                              \
      if (l < h->size && cmp(h->items[l], h->items[best]))                                        \
        best = l;                                                                                 \
      if (r < h->size && cmp(h->items[r], h->items[best]))                                        \
        best = r;                                                                                 \
      if (best == i) break;                                                                       \
      _##name##_heap_swap(h, i, best);                                                            \
      i = best;                                                                                   \
    }                                                                                             \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_peek(name* h, T* outVal) {                                            \
    if (!h || h->size == 0 || !outVal) return false;                                              \
                                                                                                  \
    *outVal = h->items[0];                                                                        \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* h) {                                                   \
    return Heap##name##_is_empty(h);                                                              \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* h) {                                                      \
    Heap##name##_clear(h);                                                                        \
  }  


#endif // AG_LIB_DS_HEAP

#ifndef AG_LIB_DS_SET
#define AG_LIB_DS_SET

#include <string.h>
#include "../../include/aglib_allocator.h"
#include "map.h"

// ——— Set ————————————————————————————————————————————————————————————————————————————————————————

#define Set(T, name, hash_fn, eq_fn)                                                              \
  Map(T, bool, name##_map_, hash_fn, eq_fn)                                                       \
  typedef name##_map_ name;                                                                       \
                                                                                                  \
  static inline bool name##_init(sAllocator* a, name* s, size_t initCap) {                        \
    return name##_map_##_init(a, s, initCap);                                                     \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_insert(name* s, T key) {                                              \
    return name##_map_##_insert(s, key, true);                                                    \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_contains(name* s, T key) {                                            \
    return name##_map_##_get(s, key, NULL);                                                       \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_resize(name* s, size_t newCap) {                                      \
    return name##_map_##_resize(s, newCap);                                                       \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_remove(name* s, T key) {                                              \
    return name##_map_##_delete(s, key);                                                          \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* s) {                                                   \
    return name##_map_##_is_empty(s);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* s) {                                                      \
    name##_map_##_clear(s);                                                                       \
  }                                                                                               \
                                                                                                  \
  static inline void name##_free(name* s) {                                                       \
    name##_map_##_free(s);                                                                        \
  }                                                                                               \

#endif // AG_LIB_DS_SET

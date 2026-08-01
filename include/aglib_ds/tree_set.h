#ifndef AG_LIB_DS_TREE_SET
#define AG_LIB_DS_TREE_SET

#include <string.h>
#include "../../include/aglib_arena.h"
#include "tree_map.h"

// ——— Red-Black Tree Set —————————————————————————————————————————————————————————————————————————

#define TreeSet(T, name, cmp_fn)                                                                  \
  TreeMap(T, bool, name##_tmap_, cmp_fn)                                                          \
  typedef name##_tmap_ name;                                                                      \
                                                                                                  \
  static inline void name##_init(sArena* a, name* s) {                                            \
    name##_tmap_##_init(a, s);                                                                    \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_insert(name* s, T key) {                                              \
    return name##_tmap_##_insert(s, key, true);                                                   \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_contains(name* s, T key) {                                            \
    bool _dummy;                                                                                  \
    return name##_tmap_##_find(s, key, &_dummy);                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_remove(name* s, T key) {                                              \
    return name##_tmap_##_remove(s, key);                                                         \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_min(name* s, T* outKey) {                                             \
    bool _dummy;                                                                                  \
    return name##_tmap_##_min(s, outKey, &_dummy);                                                \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_max(name* s, T* outKey) {                                             \
    bool _dummy;                                                                                  \
    return name##_tmap_##_max(s, outKey, &_dummy);                                                \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* s) {                                                   \
    return name##_tmap_##_is_empty(s);                                                            \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* s) {                                                      \
    name##_tmap_##_clear(s);                                                                      \
  }                                                                                               \

#endif // AG_LIB_DS_TREE_SET

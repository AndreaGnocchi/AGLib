#ifndef AG_LIB_DS_STACK
#define AG_LIB_DS_STACK

#include <string.h>
#include "../../include/aglib_allocator.h"
#include "linked_list.h"

// ——— Stack ——————————————————————————————————————————————————————————————————————————————————————

#define Stack(T, name)                                                                            \
  LinkedList(T, Stack##name)                                                                      \
  typedef Stack##name name;                                                                       \
                                                                                                  \
  static inline bool name##_init(sAllocator* a, name* s) {                                        \
    return Stack##name##_init(a, s);                                                              \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push(name* s, T val) {                                                \
    return Stack##name##_push_head(s, val);                                                       \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_pop(name* s, T* outVal) {                                             \
    return Stack##name##_pop_head(s, outVal);                                                     \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_peek(name* s, T* outVal) {                                            \
    return Stack##name##_peek_head(s, outVal);                                                    \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* s) {                                                   \
    return Stack##name##_is_empty(s);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* s) {                                                      \
    Stack##name##_clear(s);                                                                       \
  }                                                                                               \
                                                                                                  \
  static inline void name##_free(name* s) {                                                       \
    Stack##name##_free(s);                                                                        \
  }                                                                                               \


#endif // AG_LIB_DS_STACK

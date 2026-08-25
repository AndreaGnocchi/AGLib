#ifndef AG_LIB_DS_QUEUE
#define AG_LIB_DS_QUEUE

#include <string.h>
#include "../../include/aglib_allocator.h"
#include "linked_list.h"

// ——— Queue ——————————————————————————————————————————————————————————————————————————————————————

#define Queue(T, name)                                      \
  LinkedList(T, Queue##name)                                \
  typedef Queue##name name;                                 \
                                                            \
  static inline bool name##_init(sAllocator* a, name* q) {  \
    return Queue##name##_init(a, q);                        \
  }                                                         \
                                                            \
  static inline bool name##_enqueue(name* q, T val) {       \
    return Queue##name##_push_tail(q, val);                 \
  }                                                         \
                                                            \
  static inline bool name##_dequeue(name* q, T* outVal) {   \
    return Queue##name##_pop_head(q, outVal);               \
  }                                                         \
                                                            \
  static inline bool name##_peek(name* q, T* outVal) {      \
    return Queue##name##_peek_head(q, outVal);              \
  }                                                         \
                                                            \
  static inline bool name##_is_empty(name* q) {             \
    return Queue##name##_is_empty(q);                       \
  }                                                         \
                                                            \
  static inline void name##_clear(name* q) {                \
    Queue##name##_clear(q);                                 \
  }                                                         \
                                                            \
  static inline void name##_free(name* q) {                 \
    Queue##name##_free(q);                                  \
  }                                                         \

#endif // AG_LIB_DS_QUEUE

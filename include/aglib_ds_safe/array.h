#ifndef AG_LIB_DS_ARRAY
#define AG_LIB_DS_ARRAY

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include <pthread.h>
#include "../../include/aglib_arena.h"

// ——— Dynamic array ——————————————————————————————————————————————————————————————————————————————

#define DynamicArray(T, name)                                                                     \
  typedef struct {                                                                                \
    T*              items;                                                                        \
    size_t          capacity;                                                                     \
    size_t          size;                                                                         \
    sArena*         a;                                                                            \
    pthread_mutex_t lock;                                                                         \
  } name;                                                                                         \
                                                                                                  \
  static inline void name##_init(sArena* a, name* arr, size_t initCap) {                          \
    if (!a || !arr || initCap == 0) return;                                                       \
                                                                                                  \
    arr->a        = a;                                                                            \
    arr->capacity = initCap;                                                                      \
    arr->size     = 0;                                                                            \
    arr->items    = (T*)arena_alloc(a, sizeof(T) * initCap, true);                                \
                                                                                                  \
    pthread_mutexattr_t attr;                                                                     \
    pthread_mutexattr_init(&attr);                                                                \
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);                                    \
    pthread_mutex_init(&arr->lock, &attr);                                                        \
    pthread_mutexattr_destroy(&attr);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_destroy(name* arr) {                                                  \
    if (!arr) return;                                                                             \
    pthread_mutex_destroy(&arr->lock);                                                            \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push(name* arr, T items) {                                            \
    if (!arr || !arr->a) return false;                                                            \
                                                                                                  \
    pthread_mutex_lock(&arr->lock);                                                               \
                                                                                                  \
    if (arr->size >= arr->capacity) {                                                              \
      size_t newCap   = arr->capacity * 2;                                                        \
      T*     newItems = (T*)arena_alloc(arr->a, sizeof(T) * newCap, true);                        \
      if (!newItems) {                                                                            \
        pthread_mutex_unlock(&arr->lock);                                                         \
        return false;                                                                             \
      }                                                                                           \
                                                                                                  \
      memcpy(newItems, arr->items, sizeof(T) * arr->size);                                        \
      arr->items    = newItems;                                                                   \
      arr->capacity = newCap;                                                                     \
    }                                                                                             \
                                                                                                  \
    arr->items[arr->size++] = items;                                                              \
    pthread_mutex_unlock(&arr->lock);                                                             \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* arr) {                                                 \
    if (!arr) return false;                                                                       \
                                                                                                  \
    pthread_mutex_lock(&arr->lock);                                                               \
    bool empty = arr->size == 0;                                                                  \
    pthread_mutex_unlock(&arr->lock);                                                             \
    return empty;                                                                                 \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* arr) {                                                    \
    if (!arr) return;                                                                             \
                                                                                                  \
    pthread_mutex_lock(&arr->lock);                                                               \
    memset(arr->items, 0, sizeof(T) * arr->capacity);                                             \
    arr->size = 0;                                                                                \
    pthread_mutex_unlock(&arr->lock);                                                             \
  }                                                                                               \

#endif // AG_LIB_DS_ARRAY

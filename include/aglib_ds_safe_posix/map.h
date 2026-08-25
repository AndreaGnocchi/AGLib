#ifndef AG_LIB_DS_MAP
#define AG_LIB_DS_MAP

#include <string.h>
#include <pthread.h>
#include "../../include/aglib_allocator.h"

// ——— Map ————————————————————————————————————————————————————————————————————————————————————————

#define Map(Tk, Tv, name, hash_fn, eq_fn)                                                         \
  typedef struct {                                                                                \
    Tk     key;                                                                                   \
    Tv     val;                                                                                   \
    size_t psl;                                                                                   \
    bool   active;                                                                                \
  } name##Entry;                                                                                  \
                                                                                                  \
  typedef struct {                                                                                \
    name##Entry*    entries;                                                                      \
    size_t          capacity;                                                                     \
    size_t          size;                                                                         \
    sAllocator*     a;                                                                            \
    pthread_mutex_t lock;                                                                         \
  } name;                                                                                         \
                                                                                                  \
  static inline bool name##_resize(name* hm, size_t newCap);                                      \
  static inline bool _##name##_insert_nolock(name* hm, Tk key, Tv val);                           \
                                                                                                  \
  static inline bool name##_init(sAllocator* a, name* hm, size_t initCap) {                       \
    if (!a || !hm || initCap == 0 || a->type == SLAB) return false;                               \
                                                                                                  \
    hm->entries = (name##Entry*)ag_alloc(a, sizeof(name##Entry) * initCap, true);                 \
    if (!hm->entries) return false;                                                               \
                                                                                                  \
    hm->capacity = initCap;                                                                       \
    hm->size     = 0;                                                                             \
    hm->a        = a;                                                                             \
                                                                                                  \
    pthread_mutexattr_t attr;                                                                     \
    pthread_mutexattr_init(&attr);                                                                \
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);                                    \
    pthread_mutex_init(&hm->lock, &attr);                                                         \
    pthread_mutexattr_destroy(&attr);                                                             \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool _##name##_insert_nolock(name* hm, Tk key, Tv val) {                          \
    if (!hm) return false;                                                                        \
                                                                                                  \
    if (hm->size * 4 >= hm->capacity * 3) {                                                        \
      if (!name##_resize(hm, hm->capacity * 2)) return false;                                     \
    }                                                                                             \
                                                                                                  \
    name##Entry cur;                                                                              \
    cur.key    = key;                                                                             \
    cur.val    = val;                                                                             \
    cur.psl    = 0;                                                                               \
    cur.active = true;                                                                            \
                                                                                                  \
    size_t idx = hash_fn(key) % hm->capacity;                                                     \
    while (true) {                                                                                \
      name##Entry* slot = &hm->entries[idx];                                                      \
      if (!slot->active) {                                                                        \
        *slot = cur;                                                                              \
        hm->size++;                                                                               \
        return true;                                                                              \
      }                                                                                           \
      if (eq_fn(slot->key, cur.key)) {                                                            \
        slot->val = cur.val;                                                                      \
        return true;                                                                              \
      }                                                                                           \
      if (slot->psl < cur.psl) {                                                                  \
        name##Entry tmp = *slot;                                                                  \
        *slot = cur;                                                                              \
        cur   = tmp;                                                                              \
      }                                                                                           \
      cur.psl++;                                                                                  \
      idx = (idx + 1) % hm->capacity;                                                             \
    }                                                                                             \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_insert(name* hm, Tk key, Tv val) {                                    \
    if (!hm) return false;                                                                        \
                                                                                                  \
    pthread_mutex_lock(&hm->lock);                                                                \
    bool ok = _##name##_insert_nolock(hm, key, val);                                              \
    pthread_mutex_unlock(&hm->lock);                                                              \
    return ok;                                                                                    \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_get(name* hm, Tk key, Tv* outVal) {                                   \
    if (!hm) return false;                                                                        \
                                                                                                  \
    pthread_mutex_lock(&hm->lock);                                                                \
    if (hm->size == 0) {                                                                          \
      pthread_mutex_unlock(&hm->lock);                                                            \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    size_t idx = hash_fn(key) % hm->capacity;                                                     \
    size_t psl = 0;                                                                               \
    while (true) {                                                                                \
      name##Entry* slot = &hm->entries[idx];                                                      \
      if (!slot->active || slot->psl < psl) {                                                     \
        pthread_mutex_unlock(&hm->lock);                                                          \
        return false;                                                                             \
      }                                                                                           \
      if (eq_fn(slot->key, key)) {                                                                \
        if (outVal) *outVal = slot->val;                                                          \
        pthread_mutex_unlock(&hm->lock);                                                          \
        return true;                                                                              \
      }                                                                                           \
      psl++;                                                                                      \
      idx = (idx + 1) % hm->capacity;                                                             \
    }                                                                                             \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_resize(name* hm, size_t newCap) {                                     \
    if (!hm || !hm->a || newCap <= hm->size) return false;                                         \
                                                                                                  \
    pthread_mutex_lock(&hm->lock);                                                                \
                                                                                                  \
    name##Entry* newEntries = (name##Entry*)ag_alloc(                                             \
      hm->a, sizeof(name##Entry) * newCap, true);                                                 \
    if (!newEntries) {                                                                            \
      pthread_mutex_unlock(&hm->lock);                                                            \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    name##Entry* oldEntries = hm->entries;                                                        \
    size_t       oldCap     = hm->capacity;                                                       \
                                                                                                  \
    hm->entries  = newEntries;                                                                    \
    hm->capacity = newCap;                                                                        \
    hm->size     = 0;                                                                             \
                                                                                                  \
    for (size_t i = 0; i < oldCap; i++) {                                                         \
      if (oldEntries[i].active)                                                                   \
        _##name##_insert_nolock(hm, oldEntries[i].key, oldEntries[i].val);                        \
    }                                                                                             \
                                                                                                  \
    ag_free(hm->a, oldEntries);                                                                   \
                                                                                                  \
    pthread_mutex_unlock(&hm->lock);                                                              \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_delete(name* hm, Tk key) {                                            \
    if (!hm) return false;                                                                        \
                                                                                                  \
    pthread_mutex_lock(&hm->lock);                                                                \
    if (hm->size == 0) {                                                                          \
      pthread_mutex_unlock(&hm->lock);                                                            \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    size_t idx = hash_fn(key) % hm->capacity;                                                     \
    size_t psl = 0;                                                                               \
                                                                                                  \
    while (true) {                                                                                \
      name##Entry* slot = &hm->entries[idx];                                                      \
      if (!slot->active || slot->psl < psl) {                                                     \
        pthread_mutex_unlock(&hm->lock);                                                          \
        return false;                                                                             \
      }                                                                                           \
      if (eq_fn(slot->key, key)) break;                                                           \
      psl++;                                                                                      \
      idx = (idx + 1) % hm->capacity;                                                             \
    }                                                                                             \
                                                                                                  \
    while (true) {                                                                                \
      size_t       next     = (idx + 1) % hm->capacity;                                           \
      name##Entry* nextSlot = &hm->entries[next];                                                 \
                                                                                                  \
      if (!nextSlot->active || nextSlot->psl == 0) {                                              \
        hm->entries[idx].active = false;                                                          \
        break;                                                                                    \
      }                                                                                           \
                                                                                                  \
      hm->entries[idx]      = *nextSlot;                                                          \
      hm->entries[idx].psl -= 1;                                                                  \
      idx = next;                                                                                 \
    }                                                                                             \
                                                                                                  \
    hm->size--;                                                                                   \
    pthread_mutex_unlock(&hm->lock);                                                              \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* hm) {                                                  \
    if (!hm) return true;                                                                         \
                                                                                                  \
    pthread_mutex_lock(&hm->lock);                                                                \
    bool empty = hm->size == 0;                                                                   \
    pthread_mutex_unlock(&hm->lock);                                                              \
    return empty;                                                                                 \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* hm) {                                                     \
    if (!hm) return;                                                                              \
                                                                                                  \
    pthread_mutex_lock(&hm->lock);                                                                \
    memset(hm->entries, 0, sizeof(name##Entry) * hm->capacity);                                   \
    hm->size = 0;                                                                                 \
    pthread_mutex_unlock(&hm->lock);                                                              \
  }                                                                                               \
                                                                                                  \
  static inline void name##_free(name* hm) {                                                      \
    if (!hm) return;                                                                              \
                                                                                                  \
    pthread_mutex_lock(&hm->lock);                                                                \
    ag_free(hm->a, hm->entries);                                                                  \
    hm->entries  = NULL;                                                                          \
    hm->capacity = 0;                                                                             \
    hm->size     = 0;                                                                             \
    hm->a        = NULL;                                                                          \
    pthread_mutex_unlock(&hm->lock);                                                              \
                                                                                                  \
    pthread_mutex_destroy(&hm->lock);                                                             \
  }                                                                                               \

#endif // AG_LIB_DS_MAP

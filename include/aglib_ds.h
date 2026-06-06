#ifndef AG_LIB_DS
#define AG_LIB_DS

#include <string.h>
#include "../include/aglib_arena.h"

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
    if (arr->size >=  arr->capacity) {                                                            \
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

// ——— Doubly Linked List —————————————————————————————————————————————————————————————————————————

#define LinkedList(T, name)                                                                       \
  typedef struct name##Node {                                                                     \
    T*                 val;                                                                       \
    struct name##Node* next;                                                                      \
    struct name##Node* prev;                                                                      \
  } name##Node;                                                                                   \
                                                                                                  \
  typedef struct {                                                                                \
    name##Node* head;                                                                             \
    name##Node* tail;                                                                             \
    size_t      size;                                                                             \
    sArena*     a;                                                                                \
  } name;                                                                                         \
                                                                                                  \
  static inline void name##_init(sArena* a, name* list) {                                         \
    if (!a || !list) return;                                                                      \
                                                                                                  \
    list->a    = a;                                                                               \
    list->head = NULL;                                                                            \
    list->tail = NULL;                                                                            \
    list->size = 0;                                                                               \
  }                                                                                               \
                                                                                                  \
  static inline name##Node* _##name##_alloc_node(name* list, T val) {                             \
    if (!list || !list->a) return NULL;                                                           \
                                                                                                  \
    T* valCpy = arena_alloc(list->a, sizeof(T), true);                                            \
    if (!valCpy) return NULL;                                                                     \
    *valCpy = val;                                                                                \
                                                                                                  \
    name##Node* node = arena_alloc(list->a, sizeof(name##Node), true);                            \
    if (!node) return NULL;                                                                       \
                                                                                                  \
    node->val  = valCpy;                                                                          \
    node->next = NULL;                                                                            \
    node->prev = NULL;                                                                            \
                                                                                                  \
    return node;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push_head(name* list, T val) {                                        \
    name##Node* newNode = _##name##_alloc_node(list, val);                                        \
    if (!newNode) return false;                                                                   \
                                                                                                  \
    if (!list->head) {                                                                            \
      list->head = newNode;                                                                       \
      list->tail = newNode;                                                                       \
    } else {                                                                                      \
      newNode->next    = list->head;                                                              \
      list->head->prev = newNode;                                                                 \
      list->head       = newNode;                                                                 \
    }                                                                                             \
                                                                                                  \
    list->size++;                                                                                 \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push_tail(name* list, T val) {                                        \
    name##Node* newNode = _##name##_alloc_node(list, val);                                        \
    if (!newNode) return false;                                                                   \
                                                                                                  \
    if (!list->tail) {                                                                            \
      list->head = newNode;                                                                       \
      list->tail = newNode;                                                                       \
    } else {                                                                                      \
      newNode->prev    = list->tail;                                                              \
      list->tail->next = newNode;                                                                 \
      list->tail       = newNode;                                                                 \
    }                                                                                             \
                                                                                                  \
    list->size++;                                                                                 \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_pop_head(name* list, T* outVal) {                                     \
    if (!list || !list->head) return false;                                                       \
                                                                                                  \
    name##Node* oldHead = list->head;                                                             \
                                                                                                  \
    if (outVal)                                                                                   \
      *outVal = *oldHead->val;                                                                    \
                                                                                                  \
    list->head = list->head->next;                                                                \
    if (list->head) {                                                                             \
        list->head->prev = NULL;                                                                  \
    } else {                                                                                      \
        list->tail = NULL;                                                                        \
    }                                                                                             \
                                                                                                  \
    list->size--;                                                                                 \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_pop_tail(name* list, T* outVal) {                                     \
    if (!list || !list->tail) return false;                                                       \
                                                                                                  \
    name##Node* oldTail = list->tail;                                                             \
                                                                                                  \
    if (outVal)                                                                                   \
      *outVal = *oldTail->val;                                                                    \
                                                                                                  \
    list->tail = list->tail->prev;                                                                \
    if (list->tail) {                                                                             \
        list->tail->next = NULL;                                                                  \
    } else {                                                                                      \
        list->head = NULL;                                                                        \
    }                                                                                             \
                                                                                                  \
    list->size--;                                                                                 \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_peek_head(name* list, T* outVal) {                                    \
    if (!list || !list->head || !outVal) return false;                                            \
                                                                                                  \
    *outVal = *list->head->val;                                                                   \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_peek_tail(name* list, T* outVal) {                                    \
    if (!list || !list->tail || !outVal) return false;                                            \
                                                                                                  \
    *outVal = *list->tail->val;                                                                   \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_insert_after(name* list, name##Node* node, T val) {                   \
    if (!list || !node) return false;                                                             \
                                                                                                  \
    name##Node* newNode = _##name##_alloc_node(list, val);                                        \
    if (!newNode) return false;                                                                   \
                                                                                                  \
    newNode->next = node->next;                                                                   \
    newNode->prev = node;                                                                         \
                                                                                                  \
    if (node->next) {                                                                             \
      node->next->prev = newNode;                                                                 \
    } else {                                                                                      \
      list->tail = newNode;                                                                       \
    }                                                                                             \
                                                                                                  \
    node->next = newNode;                                                                         \
    list->size++;                                                                                 \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_insert_before(name* list, name##Node* node, T val) {                  \
    if (!list || !node) return false;                                                             \
                                                                                                  \
    if (node->prev == NULL)                                                                       \
        return name##_push_head(list, val);                                                       \
                                                                                                  \
    return name##_insert_after(list, node->prev, val);                                            \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_remove(name* list, name##Node* node) {                                \
    if (!list || !node) return false;                                                             \
                                                                                                  \
    if (node->prev) {                                                                             \
      node->prev->next = node->next;                                                              \
    } else {                                                                                      \
      list->head = node->next;                                                                    \
    }                                                                                             \
                                                                                                  \
    if (node->next) {                                                                             \
      node->next->prev = node->prev;                                                              \
    } else {                                                                                      \
      list->tail = node->prev;                                                                    \
    }                                                                                             \
                                                                                                  \
    list->size--;                                                                                 \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* list) {                                                \
    if (!list) return false;                                                                      \
                                                                                                  \
    return list->head == NULL;                                                                    \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* list) {                                                   \
    if (!list) return;                                                                            \
                                                                                                  \
    list->head = NULL;                                                                            \
    list->tail = NULL;                                                                            \
    list->size = 0;                                                                               \
  }                                                                                               \

// ——— Stack ——————————————————————————————————————————————————————————————————————————————————————

#define Stack(T, name)                                                                            \
  LinkedList(T, Stack##name)                                                                      \
  typedef Stack##name name;                                                                       \
                                                                                                  \
  static inline void name##_init(sArena* a, name* s) {                                            \
    Stack##name##_init(a, s);                                                                     \
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

// ——— Queue ——————————————————————————————————————————————————————————————————————————————————————

#define Queue(T, name)                                                                            \
  LinkedList(T, Queue##name)                                                                      \
  typedef Queue##name name;                                                                       \
                                                                                                  \
  static inline void name##_init(sArena* a, name* q) {                                            \
    Queue##name##_init(a, q);                                                                     \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_enqueue(name* q, T val) {                                             \
    return Queue##name##_push_tail(q, val);                                                       \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_dequeue(name* q, T* outVal) {                                         \
    return Queue##name##_pop_head(q, outVal);                                                     \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_peek(name* q, T* outVal) {                                            \
    return Queue##name##_peek_head(q, outVal);                                                    \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* q) {                                                   \
    return Queue##name##_is_empty(q);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* q) {                                                      \
    Queue##name##_clear(q);                                                                       \
  }                                                                                               \

// ——— Heap ———————————————————————————————————————————————————————————————————————————————————————

#define _HEAP_SWAP(a, i, j) do { T tmp = (a)->items[i]; (a)->items[i] = (a)->items[j]; (a)->items[j] = tmp; } while (0)
 
#define Heap(T, name, cmp)                                                                        \
  DynamicArray(T, Heap##name)                                                                     \
  typedef Heap##name name;                                                                        \
                                                                                                  \
  static inline void name##_init(sArena* a, name* h, size_t initCap) {                            \
    Heap##name##_init(a, h, initCap);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_push(name* h, T val) {                                                \
    if (!Heap##name##_push(h, val)) return false;                                                 \
                                                                                                  \
    size_t i = h->size - 1;                                                                       \
    while (i > 0) {                                                                               \
      size_t p = (i - 1) / 2;                                                                     \
      if (!cmp(h->items[i], h->items[p])) break;                                                  \
      _HEAP_SWAP(h, i, p);                                                                        \
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
      _HEAP_SWAP(h, i, best);                                                                     \
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
    name##Entry* entries;                                                                         \
    size_t       capacity;                                                                        \
    size_t       size;                                                                            \
    sArena*      a;                                                                               \
  } name;                                                                                         \
                                                                                                  \
  static inline void name##_init(sArena* a, name* hm, size_t initCap) {                           \
    if (!a || !hm || initCap == 0) return;                                                        \
                                                                                                  \
    hm->capacity = initCap;                                                                       \
    hm->size     = 0;                                                                             \
    hm->a        = a;                                                                             \
    hm->entries  = (name##Entry*)arena_alloc(a, sizeof(name##Entry) * initCap, true);             \
  }                                                                                               \
                                                                                                  \
    static inline bool name##_insert(name* hm, Tk key, Tv val) {                                  \
    if (!hm || hm->size >= hm->capacity) return false;                                            \
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
  static inline bool name##_get(name* hm, Tk key, Tv* outVal) {                                   \
    if (!hm || hm->size == 0) return false;                                                       \
                                                                                                  \
    size_t idx = hash_fn(key) % hm->capacity;                                                     \
    size_t psl = 0;                                                                               \
    while (true) {                                                                                \
      name##Entry* slot = &hm->entries[idx];                                                      \
      if (!slot->active || slot->psl < psl) return false;                                         \
      if (eq_fn(slot->key, key)) {                                                                \
        if (outVal) *outVal = slot->val;                                                          \
        return true;                                                                              \
      }                                                                                           \
      psl++;                                                                                      \
      idx = (idx + 1) % hm->capacity;                                                             \
    }                                                                                             \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_resize(name* hm, size_t newCap) {                                     \
  if (!hm || !hm->a || newCap <= hm->size) return false;                                          \
                                                                                                  \
    name##Entry* newEntries = (name##Entry*)arena_alloc(                                          \
      hm->a, sizeof(name##Entry) * newCap, true);                                                 \
    if (!newEntries) return false;                                                                \
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
        name##_insert(hm, oldEntries[i].key, oldEntries[i].val);                                  \
    }                                                                                             \
                                                                                                  \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* hm) {                                                  \
    if (!hm) return false;                                                                        \
    return hm->size == 0;                                                                         \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* hm) {                                                     \
    if (!hm) return;                                                                              \
                                                                                                  \
    memset(hm->entries, 0, sizeof(name##Entry) * hm->capacity);                                   \
    hm->size = 0;                                                                                 \
  }                                                                                               \

// ——— Set ————————————————————————————————————————————————————————————————————————————————————————

#define Set(T, name, hash_fn, eq_fn)                                                              \
  Map(T, bool, name##_map_, hash_fn, eq_fn)                                                       \
  typedef name##_map_ name;                                                                       \
                                                                                                  \
  static inline void name##_init(sArena* a, name* s, size_t initCap) {                            \
    name##_map_##_init(a, s, initCap);                                                            \
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
  static inline bool name##_is_empty(name* s) {                                                   \
    return name##_map_##_is_empty(s);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* s) {                                                      \
    name##_map_##_clear(s);                                                                       \
  }                                                                                               \

#endif // AG_LIB_DS

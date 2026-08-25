#ifndef AG_LIB_DS_LINKED_LIST
#define AG_LIB_DS_LINKED_LIST

#include <string.h>
#include <windows.h>
#include "../../include/aglib_allocator.h"

// ——— Doubly Linked List —————————————————————————————————————————————————————————————————————————

#define LinkedList(T, name)                                                       \
  typedef struct name##Node {                                                     \
    T*                 val;                                                       \
    struct name##Node* next;                                                      \
    struct name##Node* prev;                                                      \
  } name##Node;                                                                   \
                                                                                  \
  typedef struct {                                                                \
    name##Node*     head;                                                         \
    name##Node*     tail;                                                         \
    size_t          size;                                                         \
    sAllocator*     a;                                                            \
    CRITICAL_SECTION lock;                                                        \
  } name;                                                                         \
                                                                                  \
  static inline bool name##_init(sAllocator* a, name* list) {                     \
    if (!a || !list) return false;                                                \
                                                                                  \
    list->a    = a;                                                               \
    list->head = NULL;                                                            \
    list->tail = NULL;                                                            \
    list->size = 0;                                                               \
                                                                                  \
    InitializeCriticalSection(&list->lock);                                       \
                                                                                  \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline name##Node* _##name##_alloc_node(name* list, T val) {             \
    if (!list || !list->a) return NULL;                                           \
                                                                                  \
    T* valCpy = (T*)ag_alloc(list->a, sizeof(T), true);                           \
    if (!valCpy) return NULL;                                                     \
    *valCpy = val;                                                                \
                                                                                  \
    name##Node* node = (name##Node*)ag_alloc(list->a, sizeof(name##Node), true);  \
    if (!node) {                                                                  \
      ag_free(list->a, valCpy);                                                   \
      return NULL;                                                                \
    }                                                                             \
                                                                                  \
    node->val  = valCpy;                                                          \
    node->next = NULL;                                                            \
    node->prev = NULL;                                                            \
                                                                                  \
    return node;                                                                  \
  }                                                                               \
                                                                                  \
  static inline void _##name##_free_node(name* list, name##Node* node) {          \
    if (!list || !node) return;                                                   \
                                                                                  \
    ag_free(list->a, node->val);                                                  \
    ag_free(list->a, node);                                                       \
  }                                                                               \
                                                                                  \
  static inline bool name##_push_head(name* list, T val) {                        \
    if (!list) return false;                                                      \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    name##Node* newNode = _##name##_alloc_node(list, val);                        \
    if (!newNode) {                                                               \
      LeaveCriticalSection(&list->lock);                                          \
      return false;                                                               \
    }                                                                             \
                                                                                  \
    if (!list->head) {                                                            \
      list->head = newNode;                                                       \
      list->tail = newNode;                                                       \
    } else {                                                                      \
      newNode->next    = list->head;                                              \
      list->head->prev = newNode;                                                 \
      list->head       = newNode;                                                 \
    }                                                                             \
                                                                                  \
    list->size++;                                                                 \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_push_tail(name* list, T val) {                        \
    if (!list) return false;                                                      \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    name##Node* newNode = _##name##_alloc_node(list, val);                        \
    if (!newNode) {                                                               \
      LeaveCriticalSection(&list->lock);                                          \
      return false;                                                               \
    }                                                                             \
                                                                                  \
    if (!list->tail) {                                                            \
      list->head = newNode;                                                       \
      list->tail = newNode;                                                       \
    } else {                                                                      \
      newNode->prev    = list->tail;                                              \
      list->tail->next = newNode;                                                 \
      list->tail       = newNode;                                                 \
    }                                                                             \
                                                                                  \
    list->size++;                                                                 \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_pop_head(name* list, T* outVal) {                     \
    if (!list) return false;                                                      \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    if (!list->head) {                                                            \
      LeaveCriticalSection(&list->lock);                                          \
      return false;                                                               \
    }                                                                             \
                                                                                  \
    name##Node* oldHead = list->head;                                             \
                                                                                  \
    if (outVal)                                                                   \
      *outVal = *oldHead->val;                                                    \
                                                                                  \
    list->head = list->head->next;                                                \
    if (list->head) {                                                             \
        list->head->prev = NULL;                                                  \
    } else {                                                                      \
        list->tail = NULL;                                                        \
    }                                                                             \
                                                                                  \
    _##name##_free_node(list, oldHead);                                           \
    list->size--;                                                                 \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_pop_tail(name* list, T* outVal) {                     \
    if (!list) return false;                                                      \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    if (!list->tail) {                                                            \
      LeaveCriticalSection(&list->lock);                                          \
      return false;                                                               \
    }                                                                             \
                                                                                  \
    name##Node* oldTail = list->tail;                                             \
                                                                                  \
    if (outVal)                                                                   \
      *outVal = *oldTail->val;                                                    \
                                                                                  \
    list->tail = list->tail->prev;                                                \
    if (list->tail) {                                                             \
        list->tail->next = NULL;                                                  \
    } else {                                                                      \
        list->head = NULL;                                                        \
    }                                                                             \
                                                                                  \
    _##name##_free_node(list, oldTail);                                           \
    list->size--;                                                                 \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_peek_head(name* list, T* outVal) {                    \
    if (!list || !outVal) return false;                                           \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    if (!list->head) {                                                            \
      LeaveCriticalSection(&list->lock);                                          \
      return false;                                                               \
    }                                                                             \
                                                                                  \
    *outVal = *list->head->val;                                                   \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_peek_tail(name* list, T* outVal) {                    \
    if (!list || !outVal) return false;                                           \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    if (!list->tail) {                                                            \
      LeaveCriticalSection(&list->lock);                                          \
      return false;                                                               \
    }                                                                             \
                                                                                  \
    *outVal = *list->tail->val;                                                   \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_insert_after(name* list, name##Node* node, T val) {   \
    if (!list || !node) return false;                                             \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    name##Node* newNode = _##name##_alloc_node(list, val);                        \
    if (!newNode) {                                                               \
      LeaveCriticalSection(&list->lock);                                          \
      return false;                                                               \
    }                                                                             \
                                                                                  \
    newNode->next = node->next;                                                   \
    newNode->prev = node;                                                         \
                                                                                  \
    if (node->next) {                                                             \
      node->next->prev = newNode;                                                 \
    } else {                                                                      \
      list->tail = newNode;                                                       \
    }                                                                             \
                                                                                  \
    node->next = newNode;                                                         \
    list->size++;                                                                 \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_insert_before(name* list, name##Node* node, T val) {  \
    if (!list || !node) return false;                                             \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    bool ok;                                                                      \
    if (node->prev == NULL)                                                       \
        ok = name##_push_head(list, val);                                         \
    else                                                                          \
        ok = name##_insert_after(list, node->prev, val);                          \
    LeaveCriticalSection(&list->lock);                                            \
    return ok;                                                                    \
  }                                                                               \
                                                                                  \
  static inline bool name##_remove(name* list, name##Node* node) {                \
    if (!list || !node) return false;                                             \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    if (node->prev) {                                                             \
      node->prev->next = node->next;                                              \
    } else {                                                                      \
      list->head = node->next;                                                    \
    }                                                                             \
                                                                                  \
    if (node->next) {                                                             \
      node->next->prev = node->prev;                                              \
    } else {                                                                      \
      list->tail = node->prev;                                                    \
    }                                                                             \
                                                                                  \
    _##name##_free_node(list, node);                                              \
    list->size--;                                                                 \
    LeaveCriticalSection(&list->lock);                                            \
    return true;                                                                  \
  }                                                                               \
                                                                                  \
  static inline bool name##_is_empty(name* list) {                                \
    if (!list) return true;                                                       \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    bool empty = list->head == NULL;                                              \
    LeaveCriticalSection(&list->lock);                                            \
    return empty;                                                                 \
  }                                                                               \
                                                                                  \
  static inline void name##_clear(name* list) {                                   \
    if (!list) return;                                                            \
                                                                                  \
    EnterCriticalSection(&list->lock);                                            \
    name##Node* cur = list->head;                                                 \
    while (cur) {                                                                 \
      name##Node* next = cur->next;                                               \
      _##name##_free_node(list, cur);                                             \
      cur = next;                                                                 \
    }                                                                             \
                                                                                  \
    list->head = NULL;                                                            \
    list->tail = NULL;                                                            \
    list->size = 0;                                                               \
    LeaveCriticalSection(&list->lock);                                            \
  }                                                                               \
                                                                                  \
  static inline void name##_free(name* list) {                                    \
    if (!list) return;                                                            \
                                                                                  \
    name##_clear(list);                                                           \
    list->a = NULL;                                                               \
    DeleteCriticalSection(&list->lock);                                           \
  }                                                                               \

#endif // AG_LIB_DS_LINKED_LIST

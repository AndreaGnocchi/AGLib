#ifndef AG_LIB_DS_TREE_MAP
#define AG_LIB_DS_TREE_MAP

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include <pthread.h>
#include "../../include/aglib_arena.h"

// ——— Red-Black Tree —————————————————————————————————————————————————————————————————————————————
typedef enum {black, red} eColor;

#define TreeMap(Tk, Tv, name, cmp_fn)                                                             \
                                                                                                  \
  typedef struct name##Node {                                                                     \
    Tk                 key;                                                                       \
    Tv                 val;                                                                       \
    eColor             color;                                                                     \
    struct name##Node* left;                                                                      \
    struct name##Node* right;                                                                     \
    struct name##Node* parent;                                                                    \
  } name##Node;                                                                                   \
                                                                                                  \
  typedef struct {                                                                                \
    name##Node*     root;                                                                         \
    name##Node*     nil;                                                                          \
    size_t          size;                                                                         \
    sArena*         a;                                                                            \
    pthread_mutex_t lock;                                                                         \
  } name;                                                                                         \
                                                                                                  \
  static inline void name##_init(sArena* a, name* t) {                                            \
    if (!t || !a) return;                                                                         \
                                                                                                  \
    t->nil        = (name##Node*)arena_alloc(a, sizeof(name##Node), true);                        \
    t->nil->color = black;                                                                        \
    t->nil->left  = t->nil->right = t->nil->parent = t->nil;                                      \
    t->root       = t->nil;                                                                       \
    t->size       = 0;                                                                            \
    t->a          = a;                                                                            \
                                                                                                  \
    pthread_mutexattr_t attr;                                                                     \
    pthread_mutexattr_init(&attr);                                                                \
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);                                    \
    pthread_mutex_init(&t->lock, &attr);                                                          \
    pthread_mutexattr_destroy(&attr);                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void name##_destroy(name* t) {                                                    \
    if (!t) return;                                                                               \
    pthread_mutex_destroy(&t->lock);                                                              \
  }                                                                                               \
                                                                                                  \
  static inline void _##name##_left_rotate(name* t, name##Node* x) {                              \
    if (!t || !x) return;                                                                         \
                                                                                                  \
    name##Node* y = x->right;                                                                     \
    x->right      = y->left;                                                                      \
                                                                                                  \
    if (y->left != t->nil) {                                                                       \
      y->left->parent = x;                                                                        \
    }                                                                                             \
                                                                                                  \
    y->parent = x->parent;                                                                        \
                                                                                                  \
    if (x->parent == t->nil) {                                                                    \
      t->root = y;                                                                                \
    } else if (x == x->parent->left) {                                                            \
      x->parent->left = y;                                                                        \
    } else {                                                                                      \
      x->parent->right = y;                                                                       \
    }                                                                                             \
                                                                                                  \
    y->left = x;                                                                                  \
    x->parent = y;                                                                                \
  }                                                                                               \
                                                                                                  \
  static inline void _##name##_right_rotate(name* t, name##Node* x) {                             \
    if (!t || !x) return;                                                                         \
                                                                                                  \
    name##Node* y = x->left;                                                                      \
    x->left      = y->right;                                                                      \
                                                                                                  \
    if (y->right != t->nil) {                                                                      \
      y->right->parent = x;                                                                       \
    }                                                                                             \
                                                                                                  \
    y->parent = x->parent;                                                                        \
                                                                                                  \
    if (x->parent == t->nil) {                                                                    \
      t->root = y;                                                                                \
    } else if (x == x->parent->right) {                                                           \
      x->parent->right = y;                                                                       \
    } else {                                                                                      \
      x->parent->left = y;                                                                        \
    }                                                                                             \
                                                                                                  \
    y->right  = x;                                                                                \
    x->parent = y;                                                                                \
  }                                                                                               \
                                                                                                  \
  static inline void _##name##_insert_fixup(name* t, name##Node* z) {                             \
    if (!t || !z) return;                                                                         \
                                                                                                  \
    while (z->parent->color == red) {                                                             \
      if (z->parent == z->parent->parent->left) {                                                 \
        name##Node* y = z->parent->parent->right;                                                 \
                                                                                                  \
        if (y->color == red) {                                                                    \
          z->parent->color         = black;                                                       \
          y->color                 = black;                                                       \
          z->parent->parent->color = red;                                                         \
          z                        = z->parent->parent;                                           \
        } else {                                                                                  \
          if (z == z->parent->right) {                                                            \
            z = z->parent;                                                                        \
            _##name##_left_rotate(t, z);                                                          \
          }                                                                                       \
                                                                                                  \
          z->parent->color         = black;                                                       \
          z->parent->parent->color = red;                                                         \
          _##name##_right_rotate(t, z->parent->parent);                                           \
        }                                                                                         \
      } else {                                                                                    \
        name##Node* y = z->parent->parent->left;                                                  \
                                                                                                  \
        if (y->color == red) {                                                                    \
          z->parent->color         = black;                                                       \
          y->color                 = black;                                                       \
          z->parent->parent->color = red;                                                         \
          z                        = z->parent->parent;                                           \
        } else {                                                                                  \
          if (z == z->parent->left) {                                                             \
            z = z->parent;                                                                        \
            _##name##_right_rotate(t, z);                                                         \
          }                                                                                       \
                                                                                                  \
          z->parent->color         = black;                                                       \
          z->parent->parent->color = red;                                                         \
          _##name##_left_rotate(t, z->parent->parent);                                            \
        }                                                                                         \
      }                                                                                           \
    }                                                                                             \
                                                                                                  \
    t->root->color = black;                                                                       \
  }                                                                                               \
                                                                                                  \
  static inline void _##name##_delete_fixup(name* t, name##Node* x) {                             \
    if (!t || !x) return;                                                                         \
                                                                                                  \
    while (x != t->root && x->color == black) {                                                    \
      if (x == x->parent->left) {                                                                 \
        name##Node* w = x->parent->right;                                                         \
                                                                                                  \
        if (w->color == red) {                                                                    \
          w->color         = black;                                                               \
          x->parent->color = red;                                                                 \
          _##name##_left_rotate(t, x->parent);                                                    \
          w = x->parent->right;                                                                   \
        }                                                                                         \
                                                                                                  \
        if (w->left->color == black && w->right->color == black) {                                \
          w->color = red;                                                                         \
          x        = x->parent;                                                                   \
        } else {                                                                                  \
          if (w->right->color == black) {                                                         \
            w->left->color = black;                                                               \
            w->color       = red;                                                                 \
            _##name##_right_rotate(t, w);                                                         \
            w = x->parent->right;                                                                 \
          }                                                                                       \
                                                                                                  \
          w->color         = x->parent->color;                                                    \
          x->parent->color = black;                                                               \
          w->right->color  = black;                                                               \
          _##name##_left_rotate(t, x->parent);                                                    \
          x = t->root;                                                                            \
        }                                                                                         \
      } else {                                                                                    \
        name##Node* w = x->parent->left;                                                          \
                                                                                                  \
        if (w->color == red) {                                                                    \
          w->color         = black;                                                               \
          x->parent->color = red;                                                                 \
          _##name##_right_rotate(t, x->parent);                                                   \
          w = x->parent->left;                                                                    \
        }                                                                                         \
                                                                                                  \
        if (w->right->color == black && w->left->color == black) {                                \
          w->color = red;                                                                         \
          x        = x->parent;                                                                   \
        } else {                                                                                  \
          if (w->left->color == black) {                                                          \
            w->right->color = black;                                                              \
            w->color        = red;                                                                \
            _##name##_left_rotate(t, w);                                                          \
            w = x->parent->left;                                                                  \
          }                                                                                       \
                                                                                                  \
          w->color         = x->parent->color;                                                    \
          x->parent->color = black;                                                               \
          w->left->color   = black;                                                               \
          _##name##_right_rotate(t, x->parent);                                                   \
          x = t->root;                                                                            \
        }                                                                                         \
      }                                                                                           \
    }                                                                                             \
                                                                                                  \
    x->color = black;                                                                             \
  }                                                                                               \
                                                                                                  \
  static inline void _##name##_transplant(name* t, name##Node* u, name##Node* v) {                \
    if (!t || !u || !v) return;                                                                   \
                                                                                                  \
    if (u->parent == t->nil) {                                                                    \
      t->root = v;                                                                                \
    } else if (u == u->parent->left) {                                                            \
      u->parent->left = v;                                                                        \
    } else {                                                                                      \
      u->parent->right = v;                                                                       \
    }                                                                                             \
                                                                                                  \
    v->parent = u->parent;                                                                        \
  }                                                                                               \
                                                                                                  \
  static inline name##Node* _##name##_minimum(name* t, name##Node* node) {                        \
    if (!t || !node) return NULL;                                                                 \
                                                                                                  \
    while (node->left != t->nil) node = node->left;                                                \
    return node;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_insert(name* t, Tk key, Tv val) {                                     \
    if (!t || !t->a) return false;                                                                \
                                                                                                  \
    pthread_mutex_lock(&t->lock);                                                                 \
                                                                                                  \
    name##Node* y = t->nil;                                                                       \
    name##Node* x = t->root;                                                                      \
                                                                                                  \
    while (x != t->nil) {                                                                          \
      y = x;                                                                                      \
      int c = cmp_fn(key, x->key);                                                                \
      if (c == 0) {                                                                               \
        pthread_mutex_unlock(&t->lock);                                                           \
        return false;                                                                             \
      }                                                                                           \
      x = (c < 0) ? x->left : x->right;                                                           \
    }                                                                                             \
                                                                                                  \
    name##Node* z = (name##Node*)arena_alloc(t->a, sizeof(name##Node), true);                     \
    if (!z) {                                                                                     \
      pthread_mutex_unlock(&t->lock);                                                             \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    z->key    = key;                                                                              \
    z->val    = val;                                                                              \
    z->color  = red;                                                                              \
    z->left   = t->nil;                                                                           \
    z->right  = t->nil;                                                                           \
    z->parent = y;                                                                                \
                                                                                                  \
    if (y == t->nil) {                                                                            \
      t->root = z;                                                                                \
    } else if (cmp_fn(key, y->key) < 0) {                                                         \
      y->left = z;                                                                                \
    } else {                                                                                      \
      y->right = z;                                                                               \
    }                                                                                             \
                                                                                                  \
    _##name##_insert_fixup(t, z);                                                                 \
    t->size++;                                                                                    \
    pthread_mutex_unlock(&t->lock);                                                               \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_find(name* t, Tk key, Tv* out) {                                      \
    if (!t || !out) return false;                                                                 \
                                                                                                  \
    pthread_mutex_lock(&t->lock);                                                                 \
    name##Node* curr = t->root;                                                                   \
                                                                                                  \
    while (curr != t->nil) {                                                                       \
      int c = cmp_fn(key, curr->key);                                                             \
      if (c == 0) {                                                                               \
        if (out) *out = curr->val;                                                                \
        pthread_mutex_unlock(&t->lock);                                                           \
        return true;                                                                              \
      }                                                                                           \
      curr = (c < 0) ? curr->left : curr->right;                                                  \
    }                                                                                             \
                                                                                                  \
    pthread_mutex_unlock(&t->lock);                                                               \
    return false;                                                                                 \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_remove(name* t, Tk key) {                                             \
    if (!t) return false;                                                                         \
                                                                                                  \
    pthread_mutex_lock(&t->lock);                                                                 \
                                                                                                  \
    name##Node* z = t->root;                                                                      \
    while (z != t->nil && cmp_fn(key, z->key) != 0) {                                               \
      z = (cmp_fn(key, z->key) < 0) ? z->left : z->right;                                         \
    }                                                                                             \
    if (z == t->nil) {                                                                            \
      pthread_mutex_unlock(&t->lock);                                                             \
      return false;                                                                               \
    }                                                                                             \
                                                                                                  \
    name##Node* y = z;                                                                            \
    name##Node* x;                                                                                \
    eColor y_original_color = y->color;                                                           \
                                                                                                  \
    if (z->left == t->nil) {                                                                      \
      x = z->right;                                                                               \
      _##name##_transplant(t, z, z->right);                                                       \
    } else if (z->right == t->nil) {                                                              \
      x = z->left;                                                                                \
      _##name##_transplant(t, z, z->left);                                                        \
    } else {                                                                                      \
      y = _##name##_minimum(t, z->right);                                                         \
      y_original_color = y->color;                                                                \
      x = y->right;                                                                               \
                                                                                                  \
      if (y->parent == z) {                                                                       \
        x->parent = y;                                                                            \
      } else {                                                                                    \
        _##name##_transplant(t, y, y->right);                                                     \
        y->right = z->right;                                                                      \
        y->right->parent = y;                                                                     \
      }                                                                                           \
                                                                                                  \
      _##name##_transplant(t, z, y);                                                              \
      y->left = z->left;                                                                          \
      y->left->parent = y;                                                                        \
      y->color = z->color;                                                                        \
    }                                                                                             \
                                                                                                  \
    if (y_original_color == black) {                                                              \
      _##name##_delete_fixup(t, x);                                                               \
    }                                                                                             \
                                                                                                  \
    t->size--;                                                                                    \
    pthread_mutex_unlock(&t->lock);                                                               \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_min(name* t, Tk* outKey, Tv* outVal) {                                \
    if (!t || !outKey || !outVal) return false;                                                   \
                                                                                                  \
    pthread_mutex_lock(&t->lock);                                                                 \
    if (t->root == t->nil) {                                                                      \
      pthread_mutex_unlock(&t->lock);                                                             \
      return false;                                                                               \
    }                                                                                             \
    name##Node* n = t->root;                                                                      \
    while (n->left != t->nil) n = n->left;                                                         \
    *outKey = n->key; *outVal = n->val;                                                           \
    pthread_mutex_unlock(&t->lock);                                                               \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_max(name* t, Tk* outKey, Tv* outVal) {                                \
    if (!t || !outKey || !outVal) return false;                                                   \
                                                                                                  \
    pthread_mutex_lock(&t->lock);                                                                 \
    if (t->root == t->nil) {                                                                      \
      pthread_mutex_unlock(&t->lock);                                                             \
      return false;                                                                               \
    }                                                                                             \
    name##Node* n = t->root;                                                                      \
    while (n->right != t->nil) n = n->right;                                                       \
    *outKey = n->key; *outVal = n->val;                                                           \
    pthread_mutex_unlock(&t->lock);                                                               \
    return true;                                                                                  \
  }                                                                                               \
                                                                                                  \
  static inline bool name##_is_empty(name* t) {                                                   \
    if (!t) return false;                                                                         \
                                                                                                  \
    pthread_mutex_lock(&t->lock);                                                                 \
    bool empty = t->root == t->nil;                                                               \
    pthread_mutex_unlock(&t->lock);                                                               \
    return empty;                                                                                 \
  }                                                                                               \
                                                                                                  \
  static inline void _##name##_clear_node(name* t, name##Node* node) {                            \
    if (!t || !node) return;                                                                      \
                                                                                                  \
    if (node == t->nil) return;                                                                   \
    _##name##_clear_node(t, node->left);                                                          \
    _##name##_clear_node(t, node->right);                                                         \
  }                                                                                               \
                                                                                                  \
  static inline void name##_clear(name* t) {                                                      \
    if (!t) return;                                                                               \
                                                                                                  \
    pthread_mutex_lock(&t->lock);                                                                 \
    _##name##_clear_node(t, t->root);                                                             \
    t->root = t->nil;                                                                             \
    t->size = 0;                                                                                  \
    pthread_mutex_unlock(&t->lock);                                                               \
  }                                                                                               \

#endif // AG_LIB_DS_TREE_MAP

#ifndef AG_LIB_ARENA
#define AG_LIB_ARENA

#include <stddef.h>
#include <stdbool.h>

typedef struct {
  unsigned char* base;
  size_t         size;
  size_t         offset;
} sArena;

typedef struct {
  sArena* a;
  size_t originalOffset;
} sTempArena;

bool       arena_init         (sArena*   a, size_t initSize);
void*      arena_alloc_aligned(sArena*   a, size_t size, bool zero, size_t alignment);
void*      arena_alloc        (sArena*   a, size_t size, bool zero);
void       arena_reset        (sArena*   a);
void       arena_free         (sArena*   a);
sTempArena arena_temp_start   (sArena*   a);
void       arena_temp_end     (sTempArena temp);

#endif // AG_LIB_ARENA

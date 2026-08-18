#ifndef AG_LIB_ALLOCATOR
#define AG_LIB_ALLOCATOR

#include "aglib_arena.h"
#include "aglib_slab.h"

typedef enum {
  INVALID_ALLOCATOR = 0,
  ARENA,
  SLAB,
  STD
} eAllocatorType;

typedef struct {
  eAllocatorType type;
  union {
    sArena* arena;
    sSlab*  slab;
  };
} sAllocator;

// ——— Constructors ———————————————————————————————————————————————————————————————————————————————

sAllocator use_arena(sArena* a);
sAllocator use_slab (sSlab*  b);
sAllocator use_std  (void     );

// ——— API ————————————————————————————————————————————————————————————————————————————————————————

void* ag_alloc  (sAllocator* allocator , size_t size  , bool   zero  );
void  ag_free   (sAllocator* allocator , void*  ptr  );
void* ag_realloc(sAllocator* allocator , void*  oldPtr, size_t oldSize, size_t newSize, bool zero);
void  ag_reset  (sAllocator* allocator);
void  ag_destroy(sAllocator* allocator);
 
#endif // AG_LIB_ALLOCATOR

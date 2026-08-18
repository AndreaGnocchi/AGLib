#include "../include/aglib_allocator.h"
#include <stdlib.h>
#include <string.h>

// ——— Constructors ———————————————————————————————————————————————————————————————————————————————

sAllocator use_arena(sArena* a) {
  if (!a) return (sAllocator){.type = INVALID_ALLOCATOR};

  return (sAllocator){.type = ARENA, .arena = a};
}

sAllocator use_slab(sSlab* s) {
  if (!s) return (sAllocator){.type = INVALID_ALLOCATOR};

  return (sAllocator){.type = SLAB, .slab = s};
}

sAllocator use_std(void) {
  return (sAllocator){.type = STD};
}

// ——— API ————————————————————————————————————————————————————————————————————————————————————————

void* ag_alloc(sAllocator* allocator, size_t size, bool zero) {
  if (!allocator || size == 0) return NULL;

  switch (allocator->type) {
    case ARENA:
      return arena_alloc(allocator->arena, size, zero);
      
    case SLAB:
      if (size > allocator->slab->blockSize) return NULL;
      return slab_alloc(allocator->slab, zero);
      
    case STD: {
      void* ptr = malloc(size);
      
      if (ptr && zero)
        memset(ptr, 0, size);

      return ptr;
    }
      
    default:
      return NULL;
  }
}

void ag_free(sAllocator* allocator, void*  ptr) {
  if (!allocator) return;

  switch (allocator->type) {
    case ARENA:                                 return;
    case SLAB: slab_free(allocator->slab, ptr); return;
    case STD:  free(ptr);                       return;
    default:                                    return;
  }
}

void* ag_realloc(sAllocator* allocator, void* oldPtr, size_t oldSize, size_t newSize, bool zero) {
  if (!allocator) return NULL;

  switch (allocator->type) {
    case ARENA: case SLAB: {
      void* newPtr = ag_alloc(allocator, newSize, zero);
      if (!newPtr) return NULL;

      if (oldPtr && oldSize)
        memcpy(newPtr, oldPtr, oldSize < newSize ? oldSize : newSize);
      
      ag_free(allocator, oldPtr);
      return newPtr;
    }
      
    case STD: {
      void* newPtr = realloc(oldPtr, newSize);
      if (newPtr && zero && newSize > oldSize)
        memset((char*)newPtr + oldSize, 0, newSize - oldSize);
      return newPtr;
    }

    default: return NULL;
  }
}

void ag_reset(sAllocator* allocator) {
  if (!allocator) return;
  
  switch (allocator->type) {
    case ARENA: arena_reset(allocator->arena); return;
    case SLAB:  slab_reset (allocator->slab ); return;
    case STD:                                  return;
    default:                                   return;
  }
}

void ag_destroy(sAllocator* allocator) {
  if (!allocator) return;
    
  switch (allocator->type) {
    case ARENA: arena_free   (allocator->arena); return;
    case SLAB:  slab_free_all(allocator->slab ); return;
    case STD:                                    return;
    default:                                     return;
  }
}

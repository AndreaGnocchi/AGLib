#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>
#include "../include/aglib_arena.h"

#define DEFAULT_ALIGNMENT (sizeof(void*))
#define ALIGN_POW2(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

#ifndef MAP_ANONYMOUS
  #define MAP_ANONYMOUS MAP_ANON
#endif

bool arena_init(sArena* a, size_t initSize) {
  if (!a || initSize == 0) return false;

  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize < 0)
    pageSize = 4096;

  size_t actualSize = ALIGN_POW2(initSize, (size_t)pageSize);
  void*  ptr        = mmap(NULL, actualSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (ptr == MAP_FAILED) return false;

  a->base   = (unsigned char*)ptr;
  a->size   = actualSize;
  a->offset = 0;

  return true;
}

void* arena_alloc_aligned(sArena* a, size_t size, bool zero, size_t alignment) {
  if (!a || size == 0 || alignment == 0) return NULL;
  if ((alignment & (alignment - 1)) != 0) return NULL;

  size_t curPtr    = (size_t)(a->base + a->offset);
  size_t alignPtr  = ALIGN_POW2(curPtr, alignment);
  size_t padding   = alignPtr - curPtr;

  if (a->offset + padding + size > a->size) return NULL;
  
  void* result = (void*)alignPtr;
  a->offset += padding + size; 
  
  if (zero)
    memset(result, 0, size); 
  
  return result;
}

void* arena_alloc(sArena* a, size_t size, bool zero) {
  return arena_alloc_aligned(a, size, zero, DEFAULT_ALIGNMENT);
}

void arena_reset(sArena* a) {
  if (!a || !a->base) return;

  a->offset = 0;
#ifdef MADV_DONTNEED
  madvise(a->base, a->size, MADV_DONTNEED);
#endif
}

void arena_free(sArena* a) {
  if (!a || !a->base) return;

  munmap(a->base, a->size);
  a->base = NULL;
  a->size = 0;
  a->offset = 0;
}

sTempArena arena_temp_start(sArena* a) {
  sTempArena temp = {0};
  if (a) {
    temp.a              = a;
    temp.originalOffset = a->offset;
  }

  return temp;
}

void arena_temp_end(sTempArena temp) {
  if (temp.a) 
    temp.a->offset = temp.originalOffset;
}

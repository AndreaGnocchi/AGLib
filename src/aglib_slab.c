#include "../include/aglib_slab.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
void _build_free_list(sSlab* s);

#define DEFAULT_ALIGNMENT (sizeof(void*))
#define ALIGN_POW2(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

#ifndef _WIN32

#include <unistd.h>
#include <sys/mman.h>
#include "../include/aglib_helpers.h"

#ifndef MAP_ANONYMOUS
  #define MAP_ANONYMOUS MAP_ANON
#endif

static inline size_t _round_up_to_page(size_t size) {
  if (size == 0) return 0;

  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize < 0)
    pageSize = 4096;

  if (size > SIZE_MAX - pageSize) return 0;
  
  size_t actualSize = (size % pageSize == 0) ? size : size + (pageSize - (size % pageSize));

  return actualSize;
}

bool slab_init(sSlab* s , size_t blockSize, size_t nBlocks) {
  if (!s || blockSize == 0 || nBlocks == 0) return false;

  size_t max = MAX(blockSize, DEFAULT_ALIGNMENT);
  if (max > SIZE_MAX - (DEFAULT_ALIGNMENT - 1)) return false;
  
  blockSize         = ALIGN_POW2(max, DEFAULT_ALIGNMENT);
  if (nBlocks > SIZE_MAX / blockSize) return false;
  
  size_t actualSize = _round_up_to_page(blockSize * nBlocks);
  void*  ptr        = mmap(NULL, actualSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (ptr == MAP_FAILED) return false;
  
  s->base      = (unsigned char*)ptr;
  s->blockSize = blockSize;
  s->capacity  = actualSize / blockSize;
  s->totSize   = actualSize;
  s->freeList  = NULL;

  _build_free_list(s);
  
  return true;
}

void slab_free_all(sSlab* s) {
  if (!s || !s->base) return;

  munmap(s->base, s->totSize);
  memset(s, 0, sizeof(sSlab));
}

#endif // _WIN32

void _build_free_list(sSlab* s) {
  if (!s) return;

  for (size_t i = s->capacity; i-- > 0; ) {
    unsigned char* block = s->base + i * s->blockSize;
    *(void**)block = s->freeList;
    s->freeList = (sSlabBlock*)block;
  }
}

void* slab_alloc(sSlab* s, bool zero) {
  if (!s || !s->freeList) return NULL;

  void* result = s->freeList;
  s->freeList    = *(void**)result;
  
  if (zero) memset(result, 0, s->blockSize);

  return result;
}

void slab_free(sSlab* s, void* ptr) {
  if (!s || !ptr) return;

  unsigned char* p = (unsigned char*)ptr;
  if (p < s->base || p >= s->base + s->totSize) return;

  size_t offset = p - s->base;
  if (offset % s->blockSize != 0) return;

  *(void**)ptr = s->freeList;
  s->freeList  = ptr;
}

void slab_reset(sSlab* s) {
  if (!s || !s->base) return;

  s->freeList = NULL;
  
  _build_free_list(s);
}

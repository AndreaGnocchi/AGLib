#ifdef _WIN32

#include <windows.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../include/aglib_slab.h"
#include "../include/aglib_helpers.h"

void _build_free_list(sSlab* s); // defined in aglib_slab.c, shared across platforms

#define DEFAULT_ALIGNMENT (sizeof(void*))
#define ALIGN_POW2(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

static inline size_t _round_up_to_page(size_t size) {
  if (size == 0) return 0;

  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  size_t pageSize = sysInfo.dwPageSize;

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
  
  void* ptr = VirtualAlloc(NULL, actualSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

  if (ptr == NULL) return false;
  
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

  VirtualFree(s->base, 0, MEM_RELEASE);
  memset(s, 0, sizeof(sSlab));
}

#endif // _WIN32

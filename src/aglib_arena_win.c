#ifdef _WIN32

#include <windows.h>
#include <string.h>
#include <stdbool.h>
#include "../include/aglib_arena.h"

#define DEFAULT_ALIGNMENT (sizeof(void*))
#define ALIGN_POW2(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

bool arena_init(sArena* a, size_t initSize) {
  if (!a || initSize == 0) return false;

  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  size_t pageSize   = sysInfo.dwPageSize;
  size_t actualSize = ALIGN_POW2(initSize, pageSize);

  void* ptr = VirtualAlloc(NULL, actualSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (ptr == NULL) return false;

  a->base   = (unsigned char*)ptr;
  a->size   = actualSize;
  a->offset = 0;

  return true;
}

void arena_reset(sArena* a) {
  if (!a || !a->base) return;

  a->offset = 0;
  VirtualAlloc(a->base, a->size, MEM_RESET, PAGE_READWRITE);
}

void arena_free(sArena* a) {
  if (!a || !a->base) return;

  VirtualFree(a->base, 0, MEM_RELEASE);
  a->base   = NULL;
  a->size   = 0;
  a->offset = 0;
}

#endif // _WIN32

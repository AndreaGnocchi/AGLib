#ifndef AG_LIB_SLAB
#define AG_LIB_SLAB

#include <stddef.h>
#include <stdbool.h>

typedef struct sSlabBlock sSlabBlock; 

typedef struct {
  unsigned char* base;
  size_t         blockSize;
  size_t         capacity;
  size_t         totSize;
  sSlabBlock*    freeList;
} sSlab;

bool  slab_init    (sSlab* s , size_t blockSize, size_t nBlocks);
void* slab_alloc   (sSlab* s , bool   zero    );
void  slab_free    (sSlab* s , void*  ptr     );
void  slab_reset   (sSlab* s);
void  slab_free_all(sSlab* s);

#endif // AG_LIB_SLAB

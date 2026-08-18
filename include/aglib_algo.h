#ifndef AG_LIB_ALGO
#define AG_LIB_ALGO
#include "../include/aglib_allocator.h"

void ag_pdqsort  (void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *) , sAllocator* a);
void ag_mergesort(void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *) , sAllocator* a);
void ag_heapsort (void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *));

#endif // AG_LIB_ALGO

#ifndef AG_LIB_ALGO
#define AG_LIB_ALGO
#include "../include/aglib_allocator.h"

void pdqsort  (void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *) , sAllocator* a);
void mergesort(void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *) , sAllocator* a);
void heapsort (void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *));

#endif // AG_LIB_ALGO

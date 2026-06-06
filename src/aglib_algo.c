#include "../include/aglib_algo.h"
#include <string.h>
#include <stdbool.h>

static inline void _memswap(void *a, void *b, size_t size, void *tmp) {
  memcpy(tmp, a,   size);
  memcpy(a,   b,   size);
  memcpy(b,   tmp, size);
}

static void _insertion_sort(char *base, size_t left, size_t right, size_t size,
                            void *tmp, int (*cmp)(const void *, const void *)) {
  for (size_t i = left + 1; i <= right; i++) {
    memcpy(tmp, base + i * size, size);
    size_t j = i;
    while (j > left && cmp(base + (j - 1) * size, tmp) > 0) {
      memmove(base + j * size, base + (j - 1) * size, size);
      j--;
    }
    memcpy(base + j * size, tmp, size);
  }
}

// ——— Heapsort ———————————————————————————————————————————————————————————————————————————————————

static void _sift_down(char *base, size_t start, size_t end, size_t size,
                       void *tmp, int (*cmp)(const void *, const void *)) {
  size_t root = start;

  while (root * 2 + 1 <= end) {
    size_t child   = root * 2 + 1;
    size_t swapIdx = root;

    if (cmp(base + swapIdx * size, base + child * size) < 0)
      swapIdx = child;
    if (child + 1 <= end && cmp(base + swapIdx * size, base + (child + 1) * size) < 0)
      swapIdx = child + 1;

    if (swapIdx == root) return;

    _memswap(base + root * size, base + swapIdx * size, size, tmp);
    root = swapIdx;
  }
}

static void _heapsort_impl(char *b, size_t nmemb, size_t size,
                           void *tmp, int (*cmp)(const void *, const void *)) {
  size_t start = (nmemb - 2) / 2;
  do {
    _sift_down(b, start, nmemb - 1, size, tmp, cmp);
  } while (start-- > 0);

  for (size_t end = nmemb - 1; end > 0; end--) {
    _memswap  (b, b + end * size, size, tmp);
    _sift_down(b, 0, end - 1, size, tmp, cmp);
  }
}

void heapsort(void *base, size_t nmemb, size_t size,
              int (*cmp)(const void *, const void *)) {
  if (nmemb < 2) return;

  sArena a;
  if (!arena_init(&a, size)) return;
  void *tmp = arena_alloc(&a, size, false);

  _heapsort_impl((char*)base, nmemb, size, tmp, cmp);

  arena_free(&a);
}

// ——— Pdqsort ————————————————————————————————————————————————————————————————————————————————————

static inline size_t _log2_floor(size_t n) {
  size_t log = 0;
  while (n >>= 1) ++log;
  return log;
}

static size_t _partition(char *base, size_t left, size_t right, size_t size,
                         void *tmp, int (*cmp)(const void *, const void *)) {
  size_t mid = left + (right - left) / 2;
  
  if (cmp(base + left * size, base + mid   * size) > 0) _memswap(base + left * size, base + mid   * size, size, tmp);
  if (cmp(base + left * size, base + right * size) > 0) _memswap(base + left * size, base + right * size, size, tmp);
  if (cmp(base + mid  * size, base + right * size) > 0) _memswap(base + mid  * size, base + right * size, size, tmp);
  
  memcpy(tmp, base + mid * size, size);
  size_t i = left;
  size_t j = right;
  
  while (true) {
    do { i++; } while (cmp(base + i * size, tmp) < 0);
    do { j--; } while (cmp(base + j * size, tmp) > 0);
    if (i >= j) return j;
    _memswap(base + i * size, base + j * size, size, tmp);
    i++;
    j--;
  }
}

static void _pdqsort_recursive(char *base, size_t left, size_t right, size_t limit,
                               size_t size, void *tmp,
                               int (*cmp)(const void *, const void *)) {
  while (right - left + 1 > 16) {
    if (limit == 0) {
      _heapsort_impl(base + left * size, right - left + 1, size, tmp, cmp);
      return;
    }
    limit--;

    size_t pivotIdx = _partition(base, left, right, size, tmp, cmp);

    _pdqsort_recursive(base, left, pivotIdx, limit, size, tmp, cmp);
    left = pivotIdx + 1;
  }

  _insertion_sort(base, left, right, size, tmp, cmp);
}

void pdqsort(void *base, size_t nmemb, size_t size,
             int (*cmp)(const void *, const void *), sArena* a) {
  if (nmemb < 2) return;

  void *tmp = arena_alloc(a, size, false);
  if (!tmp) return;

  size_t limit = _log2_floor(nmemb) * 2;
  _pdqsort_recursive((char*)base, 0, nmemb - 1, limit, size, tmp, cmp);
}

// ——— Mergesort ——————————————————————————————————————————————————————————————————————————————————

static inline void _merge(void *base, void *temp, size_t left, size_t mid,
                          size_t right, size_t size,
                          int (*cmp)(const void *, const void *)) {
  size_t i = left, j = mid + 1, k = left;
  char  *b = (char*)base;
  char  *t = (char*)temp;

  while (i <= mid && j <= right) {
    if (cmp(b + i * size, b + j * size) <= 0)
      memcpy(t + k++ * size, b + i++ * size, size);
    else
      memcpy(t + k++ * size, b + j++ * size, size);
  }
  while (i <= mid)   memcpy(t + k++ * size, b + i++ * size, size);
  while (j <= right) memcpy(t + k++ * size, b + j++ * size, size);

  memcpy(b + left * size, t + left * size, (right - left + 1) * size);
}

static inline void _msort_recursive(void *base, void *temp, size_t left,
                                    size_t right, size_t size,
                                    int (*cmp)(const void *, const void *)) {
  if (right - left < 16) {
    _insertion_sort((char*)base, left, right, size, temp, cmp);
    return;
  }

  if (left < right) {
    size_t mid = left + (right - left) / 2;
    _msort_recursive(base, temp, left,    mid,   size, cmp);
    _msort_recursive(base, temp, mid + 1, right, size, cmp);
    _merge          (base, temp, left,    mid,   right, size, cmp);
  }
}

void mergesort(void *base, size_t nmemb, size_t size,
               int (*cmp)(const void *, const void *), sArena* a) {
  if (nmemb < 2) return;

  void *temp = arena_alloc(a, nmemb * size, false);
  if (!temp) return;

  _msort_recursive(base, temp, 0, nmemb - 1, size, cmp);
}

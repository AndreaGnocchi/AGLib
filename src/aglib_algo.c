#include "../include/aglib_algo.h"
#include <string.h>
#include <stdbool.h>

static inline void _memswap(void* a, void* b, size_t size, void* tmp) {
  memcpy(tmp, a,   size);
  memcpy(a,   b,   size);
  memcpy(b,   tmp, size);
}

static inline char* _med3(char* a, char* b, char* c, int (*cmp)(const void* , const void* )) {
  return cmp(a, b) < 0 ?
         (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a)) :
         (cmp(b, c) > 0 ? b : (cmp(a, c) > 0 ? c : a));
}

static void _insertion_sort(char* base, size_t left, size_t right, size_t size,
                            void* tmp, int (*cmp)(const void* , const void* )) {
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

static bool _partial_insertion_sort(char* base, size_t n, size_t size, void* tmp, 
                                    int (*cmp)(const void* , const void* )) {
  int limit = 8;
  for (size_t i = 1; i < n; i++) {
    char *curr = base + i * size;
    if (cmp(curr, base + (i - 1) * size) < 0) {
      memcpy(tmp, curr, size);

      size_t j = i;
      do {
        memmove(base + j * size, base + (j - 1) * size, size);
        j--;
      } while (j > 0 && cmp(tmp, base + (j - 1) * size) < 0);
      
      memcpy(base + j * size, tmp, size);
      
      if (--limit == 0) return false;
    }
  }
  return true;
}

// ——— Heapsort ———————————————————————————————————————————————————————————————————————————————————

static void _sift_down(char* base, size_t start, size_t end, size_t size,
                       void* tmp, int (*cmp)(const void* , const void* )) {
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

static void _heapsort_impl(char* b, size_t nmemb, size_t size,
                           void* tmp, int (*cmp)(const void* , const void * )) {
  size_t start = (nmemb - 2) / 2;
  do {
    _sift_down(b, start, nmemb - 1, size, tmp, cmp);
  } while (start-- > 0);

  for (size_t end = nmemb - 1; end > 0; end--) {
    _memswap  (b, b + end * size, size, tmp);
    _sift_down(b, 0, end - 1, size, tmp, cmp);
  }
}

void ag_heapsort(void* base, size_t nmemb, size_t size,
                 int (*cmp)(const void* , const void* ), sAllocator* a) {
  if (nmemb < 2 || !a) return;
  
  void *tmp = ag_alloc(a, size, false);
  if (!tmp) return;

  _heapsort_impl((char*)base, nmemb, size, tmp, cmp);

  ag_free(a, tmp);
}

// ——— Pdqsort ————————————————————————————————————————————————————————————————————————————————————

static inline size_t _log2_floor(size_t n) {
  size_t log = 0;
  while (n >>= 1) ++log;
  return log;
}

static inline size_t _partition_pdq(char* base, size_t n, size_t size, void* tmp, void* pivotBuf,
                             int (*cmp)(const void* , const void* ), bool* swapsMade) {
  char* p0 = base;
  char* pm = base + (n / 2) * size;
  char* pn = base + (n - 1) * size;
  char* pivotPtr;

  if (n > 128) {
    size_t step = (n / 8) * size;
    char*  p1   = _med3(p0           , p0 + step, p0 + 2 * step, cmp);
    char*  p2   = _med3(pm - step    , pm       , pm + step    , cmp);
    char*  p3   = _med3(pn - 2 * step, pn - step, pn           , cmp);
    pivotPtr    = _med3(p1           , p2       , p3           , cmp);
  } else {
    pivotPtr = _med3(p0, pm, pn, cmp);
  }

  _memswap(base, pivotPtr, size, tmp);
  memcpy(pivotBuf, base, size);

  size_t i = 1;
  size_t j = n - 1;
  *swapsMade = false;

  while (true) {
    while (i <= j && cmp(base + i * size, pivotBuf) < 0) i++;
    while (i <= j && cmp(base + j * size, pivotBuf) > 0) j--;
    if (i > j) break;
    
    _memswap(base + i * size, base + j * size, size, tmp);
    *swapsMade = true;
    i++; j--;
  }

  if (j > 0) {
    _memswap(base, base + j * size, size, tmp);
  }
  return j;
}

static inline void _pdqsort_recursive(char* base, size_t n, size_t badAllowed, size_t size, 
                               void* tmp, void* pivotBuf, int (*cmp)(const void* , const void* ), 
                               bool leftmost) {
  while (n > 24) {
    bool   swapsMade;
    size_t pivotIdx  = _partition_pdq(base, n, size, tmp, pivotBuf, cmp, &swapsMade);
    size_t leftSize  = pivotIdx;
    size_t rightSize = n - pivotIdx - 1;

    if (leftSize < n / 8 || rightSize < n / 8) {
      if (--badAllowed == 0) {
        _heapsort_impl(base, n, size, tmp, cmp);
        return;
      }
      if (n >= 24) {
        _memswap(base + (n / 2) * size, base + (n / 4) * size, size, tmp);
        _memswap(base + (n / 2 - 1) * size, base + (3 * n / 4) * size, size, tmp);
      }
    }

    if (!swapsMade && leftmost) {
      if (_partial_insertion_sort(base, n, size, tmp, cmp)) return;
    }

    if (leftSize < rightSize) {
      _pdqsort_recursive(base, leftSize, badAllowed, size, tmp, pivotBuf, cmp, leftmost);
      base     = base + (pivotIdx + 1) * size;
      n        = rightSize;
      leftmost = false;
    } else {
      _pdqsort_recursive(base + (pivotIdx + 1) * size, rightSize, badAllowed, size, tmp, pivotBuf, cmp, false);
      n = leftSize;
    }
  }

  if (n > 1) {
    _insertion_sort(base, 0, n - 1, size, tmp, cmp);
  }
}

void ag_pdqsort(void* base, size_t nmemb, size_t size,
                int (*cmp)(const void* , const void* ), sAllocator* a) {
  if (nmemb < 2 || !a) return;

  void* memBlock = ag_alloc(a, size * 2, false);
  if (!memBlock) return;

  void* tmp      = memBlock;
  void* pivotBuf = (char*)memBlock + size;

  size_t bad_allowed = _log2_floor(nmemb); 
  _pdqsort_recursive((char*)base, nmemb, bad_allowed, size, tmp, pivotBuf, cmp, true);

  ag_free(a, memBlock);
}

// ——— Mergesort ——————————————————————————————————————————————————————————————————————————————————

static inline void _merge(void* base, void* temp, size_t left, size_t mid,
                          size_t right, size_t size,
                          int (*cmp)(const void* , const void* )) {
  size_t i = left, j = mid + 1, k = left;
  char*  b = (char*)base;
  char*  t = (char*)temp;

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

static inline void _msort_recursive(void* base, void* temp, size_t left,
                                    size_t right, size_t size,
                                    int (*cmp)(const void* , const void* )) {
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

void ag_mergesort(void* base, size_t nmemb, size_t size,
                  int (*cmp)(const void* , const void* ), sAllocator* a) {
  if (nmemb < 2 || !a) return;

  void* temp = ag_alloc(a, nmemb * size, false);
  if (!temp) return;

  _msort_recursive(base, temp, 0, nmemb - 1, size, cmp);

  ag_free(a, temp);
}

// ——— Timsort ————————————————————————————————————————————————————————————————————————————————————

typedef struct {
  size_t start;
  size_t len;
} sTimSortRun;

static inline size_t _calculate_minrun(size_t n) {
  size_t r = 0;
  while (n >= 64) {
    r |= (n & 1);
    n >>= 1;
  }
  return n + r;
}

static inline void _reverse_run(char* base, size_t left, size_t right, size_t size, void* tmp) {
  while (left < right) {
    _memswap(base + left * size, base + right * size, size, tmp);
    left++;
    right--;
  }
}

static inline void _timsort_collapse(char* base, void* temp, size_t size,
                                     sTimSortRun* stack, size_t* stackLen,
                                     int (*cmp)(const void* , const void* )) {
  while (*stackLen > 1) {
    size_t n              = *stackLen - 1;
    bool   mergeNandNm1   = false;
    bool   mergeNm1andNm2 = false;

    if (n >= 2 && stack[n - 2].len <= stack[n - 1].len + stack[n].len) {
      if (stack[n - 2].len < stack[n].len) {
        mergeNm1andNm2 = true;
      } else {
        mergeNandNm1 = true;
      }
    } else if (stack[n - 1].len <= stack[n].len) {
      mergeNandNm1 = true;
    } else {
      break;
    }

    if (mergeNm1andNm2) {
      _merge(base, temp, stack[n - 2].start, 
             stack[n - 1].start - 1, 
             stack[n - 1].start + stack[n - 1].len - 1, size, cmp);
      stack[n - 2].len += stack[n - 1].len;
      stack[n - 1] = stack[n];
      (*stackLen)--;
    } else if (mergeNandNm1) {
      _merge(base, temp, stack[n - 1].start, 
             stack[n].start - 1, 
             stack[n].start + stack[n].len - 1, size, cmp);
      stack[n - 1].len += stack[n].len;
      (*stackLen)--;
    }
  }
}

void ag_timsort(void* base, size_t nmemb, size_t size,
                int (*cmp)(const void* , const void* ), sAllocator* a) {
  if (nmemb < 2 || !a) return;

  void* memBlock = ag_alloc(a, (nmemb * size) + size, false);
  if (!memBlock) return;

  char*       b      = (char *)base;
  void*       temp   = memBlock;
  void*       tmp    = (char *)memBlock + (nmemb * size);
  size_t      minRun = _calculate_minrun(nmemb);
  sTimSortRun runStack[85];
  size_t      stackLen = 0;
  size_t      start    = 0;

  while (start < nmemb) {
    size_t runLen = 1;

    if (start + 1 < nmemb) {
      if (cmp(b + (start + 1) * size, b + start * size) < 0) {
        runLen++;
        while (start + runLen < nmemb && 
               cmp(b + (start + runLen) * size, b + (start + runLen - 1) * size) < 0) {
          runLen++;
        }
        _reverse_run(b, start, start + runLen - 1, size, tmp);
      } else {
        runLen++;
        while (start + runLen < nmemb && 
               cmp(b + (start + runLen) * size, b + (start + runLen - 1) * size) >= 0) {
          runLen++;
        }
      }
    }

    if (runLen < minRun) {
      size_t forceLen = (nmemb - start) < minRun ? (nmemb - start) : minRun;
      _insertion_sort(b, start, start + forceLen - 1, size, tmp, cmp);
      runLen = forceLen;
    }

    runStack[stackLen].start = start;
    runStack[stackLen].len = runLen;
    stackLen++;

    _timsort_collapse(b, temp, size, runStack, &stackLen, cmp);

    start += runLen;
  }

  while (stackLen > 1) {
    size_t n = stackLen - 1;
    if (n >= 2 && runStack[n - 2].len < runStack[n].len) {
      _merge(b, temp, runStack[n - 2].start, 
             runStack[n - 1].start - 1, 
             runStack[n - 1].start + runStack[n - 1].len - 1, size, cmp);
      runStack[n - 2].len += runStack[n - 1].len;
      runStack[n - 1] = runStack[n];
    } else {
      _merge(b, temp, runStack[n - 1].start, 
             runStack[n].start - 1, 
             runStack[n].start + runStack[n].len - 1, size, cmp);
      runStack[n - 1].len += runStack[n].len;
    }
    stackLen--;
  }

  ag_free(a, memBlock);
}

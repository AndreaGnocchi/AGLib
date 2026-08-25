#include "../include/ag.h"
#include "test_framework.h"
#include <string.h>
#include <stdlib.h>

static int qcmp_int(const void* a, const void* b) {
  return (*(const int*)a) - (*(const int*)b);
}

static bool is_sorted(const int* arr, size_t n) {
  for (size_t i = 1; i < n; i++)
    if (arr[i - 1] > arr[i]) return false;
  return true;
}

// simple xorshift PRNG so results are reproducible across platforms
static uint32_t xorshift_next(uint32_t* state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

typedef void (*sort_with_alloc_fn)(void*, size_t, size_t, int (*)(const void*, const void*), sAllocator*);

static void run_all_four(int* src, size_t n, sAllocator* alloc) {
  int* a1 = (int*)malloc(n * sizeof(int));
  int* a2 = (int*)malloc(n * sizeof(int));
  int* a3 = (int*)malloc(n * sizeof(int));
  int* a4 = (int*)malloc(n * sizeof(int));

  if (n > 0) {
    memcpy(a1, src, n * sizeof(int));
    memcpy(a2, src, n * sizeof(int));
    memcpy(a3, src, n * sizeof(int));
    memcpy(a4, src, n * sizeof(int));
  }

  ag_pdqsort(a1, n, sizeof(int), qcmp_int, alloc);
  AG_CHECK(is_sorted(a1, n));

  ag_mergesort(a2, n, sizeof(int), qcmp_int, alloc);
  AG_CHECK(is_sorted(a2, n));

  ag_heapsort(a3, n, sizeof(int), qcmp_int, alloc);
  AG_CHECK(is_sorted(a3, n));

  ag_timsort(a4, n, sizeof(int), qcmp_int, alloc);
  AG_CHECK(is_sorted(a4, n));

  // Cross-check: all four sorts should agree on the multiset of results.
  AG_CHECK(memcmp(a1, a2, n * sizeof(int)) == 0);
  AG_CHECK(memcmp(a1, a3, n * sizeof(int)) == 0);
  AG_CHECK(memcmp(a1, a4, n * sizeof(int)) == 0);

  free(a1); free(a2); free(a3); free(a4);
}

static void test_algo_basic_distributions(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  int base[] = { 9, 2, 7, 1, 5, 5, 3, 8, 0, 6, 4 };
  size_t n = sizeof(base) / sizeof(base[0]);
  run_all_four(base, n, &alloc);

  int already_sorted[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  run_all_four(already_sorted, sizeof(already_sorted) / sizeof(already_sorted[0]), &alloc);

  int reverse_sorted[] = { 7, 6, 5, 4, 3, 2, 1, 0 };
  run_all_four(reverse_sorted, sizeof(reverse_sorted) / sizeof(reverse_sorted[0]), &alloc);

  int all_same[] = { 4, 4, 4, 4, 4, 4 };
  run_all_four(all_same, sizeof(all_same) / sizeof(all_same[0]), &alloc);

  int negatives[] = { -5, 3, -100, 0, 42, -1, 7 };
  run_all_four(negatives, sizeof(negatives) / sizeof(negatives[0]), &alloc);

  // A handful of ascending/descending runs, to exercise timsort's natural
  // run detection specifically.
  int sawtooth[] = { 1, 2, 3, 9, 8, 7, 6, 4, 5, 6, 7, 0, -1, -2 };
  run_all_four(sawtooth, sizeof(sawtooth) / sizeof(sawtooth[0]), &alloc);

  arena_free(&a);
}

static void test_algo_edge_sizes(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  int empty[1];
  ag_pdqsort(empty, 0, sizeof(int), qcmp_int, &alloc);
  ag_mergesort(empty, 0, sizeof(int), qcmp_int, &alloc);
  ag_heapsort(empty, 0, sizeof(int), qcmp_int, &alloc);
  ag_timsort(empty, 0, sizeof(int), qcmp_int, &alloc);

  int single[1] = { 42 };
  ag_pdqsort(single, 1, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(single[0] == 42);

  int single2[1] = { 42 };
  ag_mergesort(single2, 1, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(single2[0] == 42);

  int single3[1] = { 42 };
  ag_heapsort(single3, 1, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(single3[0] == 42);

  int single4[1] = { 42 };
  ag_timsort(single4, 1, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(single4[0] == 42);

  int pair[2] = { 2, 1 };
  ag_pdqsort(pair, 2, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(pair[0] == 1 && pair[1] == 2);

  int pair2[2] = { 2, 1 };
  ag_timsort(pair2, 2, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(pair2[0] == 1 && pair2[1] == 2);

  arena_free(&a);
}

static void test_algo_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  const size_t n = 5000;
  int* data = (int*)malloc(n * sizeof(int));
  uint32_t state = 0xC0FFEEu;
  for (size_t i = 0; i < n; i++)
    data[i] = (int)(xorshift_next(&state) % 10000) - 5000;

  run_all_four(data, n, &alloc);

  free(data);
  arena_free(&a);
}

static void test_algo_timsort_large_runs(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  // Nearly-sorted data with a few injected out-of-order runs is timsort's
  // best case; make sure it still produces a correct order (and, on the
  // cross-check inside run_all_four, that it agrees with everything else).
  const size_t n = 2000;
  int* data = (int*)malloc(n * sizeof(int));
  for (size_t i = 0; i < n; i++) data[i] = (int)i;

  uint32_t state = 0xBADF00Du;
  for (int shuffle = 0; shuffle < 40; shuffle++) {
    size_t i = xorshift_next(&state) % n;
    size_t j = xorshift_next(&state) % n;
    int tmp = data[i]; data[i] = data[j]; data[j] = tmp;
  }

  run_all_four(data, n, &alloc);

  free(data);
  arena_free(&a);
}

void run_algo_tests(void) {
  test_algo_basic_distributions();
  test_algo_edge_sizes();
  test_algo_stress();
  test_algo_timsort_large_runs();
}

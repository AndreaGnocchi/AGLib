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

static void run_all_three(int* src, size_t n, sAllocator* alloc) {
  int* a1 = (int*)malloc(n * sizeof(int));
  int* a2 = (int*)malloc(n * sizeof(int));
  int* a3 = (int*)malloc(n * sizeof(int));

  if (n > 0) {
    memcpy(a1, src, n * sizeof(int));
    memcpy(a2, src, n * sizeof(int));
    memcpy(a3, src, n * sizeof(int));
  }

  pdqsort(a1, n, sizeof(int), qcmp_int, alloc);
  AG_CHECK(is_sorted(a1, n));

  mergesort(a2, n, sizeof(int), qcmp_int, alloc);
  AG_CHECK(is_sorted(a2, n));

  heapsort(a3, n, sizeof(int), qcmp_int);
  AG_CHECK(is_sorted(a3, n));

  // Cross-check: all three sorts should agree on the multiset of results.
  AG_CHECK(memcmp(a1, a2, n * sizeof(int)) == 0);
  AG_CHECK(memcmp(a1, a3, n * sizeof(int)) == 0);

  free(a1); free(a2); free(a3);
}

static void test_algo_basic_distributions(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  int base[] = { 9, 2, 7, 1, 5, 5, 3, 8, 0, 6, 4 };
  size_t n = sizeof(base) / sizeof(base[0]);
  run_all_three(base, n, &alloc);

  int already_sorted[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  run_all_three(already_sorted, sizeof(already_sorted) / sizeof(already_sorted[0]), &alloc);

  int reverse_sorted[] = { 7, 6, 5, 4, 3, 2, 1, 0 };
  run_all_three(reverse_sorted, sizeof(reverse_sorted) / sizeof(reverse_sorted[0]), &alloc);

  int all_same[] = { 4, 4, 4, 4, 4, 4 };
  run_all_three(all_same, sizeof(all_same) / sizeof(all_same[0]), &alloc);

  int negatives[] = { -5, 3, -100, 0, 42, -1, 7 };
  run_all_three(negatives, sizeof(negatives) / sizeof(negatives[0]), &alloc);

  arena_free(&a);
}

static void test_algo_edge_sizes(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  int empty[1];
  pdqsort(empty, 0, sizeof(int), qcmp_int, &alloc);
  mergesort(empty, 0, sizeof(int), qcmp_int, &alloc);
  heapsort(empty, 0, sizeof(int), qcmp_int);

  int single[1] = { 42 };
  pdqsort(single, 1, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(single[0] == 42);

  int single2[1] = { 42 };
  mergesort(single2, 1, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(single2[0] == 42);

  int single3[1] = { 42 };
  heapsort(single3, 1, sizeof(int), qcmp_int);
  AG_CHECK(single3[0] == 42);

  int pair[2] = { 2, 1 };
  pdqsort(pair, 2, sizeof(int), qcmp_int, &alloc);
  AG_CHECK(pair[0] == 1 && pair[1] == 2);

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

  run_all_three(data, n, &alloc);

  free(data);
  arena_free(&a);
}

void run_algo_tests(void) {
  test_algo_basic_distributions();
  test_algo_edge_sizes();
  test_algo_stress();
}

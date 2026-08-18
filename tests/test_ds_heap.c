#include "../include/ag.h"
#include "test_framework.h"
#include <stdlib.h>

static bool less_int(int a, int b) { return a < b; }

Heap(int, TestHeap, less_int)

static void test_heap_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestHeap h;
  AG_CHECK(TestHeap_init(&alloc, &h, 4));
  AG_CHECK(TestHeap_is_empty(&h));

  int values[] = { 5, 1, 8, 2, 9, 3 };
  for (size_t i = 0; i < 6; i++)
    AG_CHECK(TestHeap_push(&h, values[i]));

  int v;
  AG_CHECK(TestHeap_peek(&h, &v)); AG_CHECK(v == 1);

  int prev = -1;
  while (TestHeap_pop(&h, &v)) {
    AG_CHECK(v >= prev);
    prev = v;
  }
  AG_CHECK(TestHeap_is_empty(&h));

  TestHeap_free(&h);
  arena_free(&a);
}

static void test_heap_duplicates(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestHeap h;
  AG_CHECK(TestHeap_init(&alloc, &h, 4));

  int values[] = { 3, 3, 3, 1, 1, 5, 5, 5, 5 };
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    AG_CHECK(TestHeap_push(&h, values[i]));

  int counts[6] = {0};
  int v;
  while (TestHeap_pop(&h, &v)) counts[v]++;

  AG_CHECK(counts[1] == 2);
  AG_CHECK(counts[3] == 3);
  AG_CHECK(counts[5] == 4);

  TestHeap_free(&h);
  arena_free(&a);
}

static int qcmp_int(const void* a, const void* b) {
  return (*(const int*)a) - (*(const int*)b);
}

static void test_heap_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestHeap h;
  AG_CHECK(TestHeap_init(&alloc, &h, 4));

  const int N = 5000;
  int* reference = (int*)malloc(N * sizeof(int));

  uint32_t state = 0xACE1u;
  for (int i = 0; i < N; i++) {
    uint32_t x = state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    state = x;
    int val = (int)(x % 100000);
    reference[i] = val;
    AG_CHECK(TestHeap_push(&h, val));
  }

  qsort(reference, N, sizeof(int), qcmp_int);

  bool orderOk = true;
  for (int i = 0; i < N; i++) {
    int v;
    if (!TestHeap_pop(&h, &v) || v != reference[i]) { orderOk = false; break; }
  }
  AG_CHECK(orderOk);
  AG_CHECK(TestHeap_is_empty(&h));

  free(reference);
  TestHeap_free(&h);
  arena_free(&a);
}

static void test_heap_invalid_args(void) {
  AG_CHECK(TestHeap_is_empty(NULL));

  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestHeap h;
  AG_CHECK(TestHeap_init(&alloc, &h, 4));

  int v;
  AG_CHECK(!TestHeap_peek(&h, &v));
  AG_CHECK(!TestHeap_pop(&h, &v));
  AG_CHECK(!TestHeap_pop(NULL, &v));

  TestHeap_free(&h);
  arena_free(&a);
}

void run_heap_tests(void) {
  test_heap_basic();
  test_heap_duplicates();
  test_heap_stress();
  test_heap_invalid_args();
}

#include "../include/ag.h"
#include "test_framework.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// TreeMap has always accepted a slab-backed sAllocator: unlike DynamicArray
// or Map, it never grows a single backing block - every insert allocates
// exactly one fixed-size node, and _init allocates exactly one more for
// the sentinel `nil` node. That makes it a natural fit for a slab.

static int cmp_int(int a, int b) { return a - b; }

TreeMap(int, int, TestTreeMapSlab, cmp_int)

#define SLAB_BLOCK_SIZE sizeof(TestTreeMapSlabNode)

static void test_tree_map_slab_init_accepts_slab(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestTreeMapSlab t;
  AG_CHECK(TestTreeMapSlab_init(&alloc, &t));
  AG_CHECK(TestTreeMapSlab_is_empty(&t));

  TestTreeMapSlab_free(&t);
  slab_free_all(&s);
}

static void test_tree_map_slab_basic(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestTreeMapSlab t;
  AG_CHECK(TestTreeMapSlab_init(&alloc, &t));

  int keys[] = { 50, 30, 70, 20, 40, 60, 80, 10 };
  for (size_t i = 0; i < 8; i++)
    AG_CHECK(TestTreeMapSlab_insert(&t, keys[i], keys[i] * 10));

  int out;
  AG_CHECK(TestTreeMapSlab_find(&t, 40, &out)); AG_CHECK(out == 400);
  AG_CHECK(!TestTreeMapSlab_find(&t, 999, &out));

  AG_CHECK(!TestTreeMapSlab_insert(&t, 40, 111));
  AG_CHECK(TestTreeMapSlab_find(&t, 40, &out)); AG_CHECK(out == 400);

  int minK, minV, maxK, maxV;
  AG_CHECK(TestTreeMapSlab_min(&t, &minK, &minV)); AG_CHECK(minK == 10);
  AG_CHECK(TestTreeMapSlab_max(&t, &maxK, &maxV)); AG_CHECK(maxK == 80);

  AG_CHECK(TestTreeMapSlab_remove(&t, 40));
  AG_CHECK(!TestTreeMapSlab_find(&t, 40, &out));
  AG_CHECK(!TestTreeMapSlab_remove(&t, 40));

  TestTreeMapSlab_free(&t);
  slab_free_all(&s);
}

static void test_tree_map_slab_ascending_insert(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 3100));
  sAllocator alloc = use_slab(&s);

  TestTreeMapSlab t;
  AG_CHECK(TestTreeMapSlab_init(&alloc, &t));

  const int N = 3000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestTreeMapSlab_insert(&t, i, i));

  int minK, minV, maxK, maxV;
  AG_CHECK(TestTreeMapSlab_min(&t, &minK, &minV)); AG_CHECK(minK == 0);
  AG_CHECK(TestTreeMapSlab_max(&t, &maxK, &maxV)); AG_CHECK(maxK == N - 1);

  for (int i = 0; i < N; i++) {
    int out;
    AG_CHECK(TestTreeMapSlab_find(&t, i, &out));
    AG_CHECK(out == i);
  }

  TestTreeMapSlab_free(&t);
  slab_free_all(&s);
}

static uint32_t xorshift_next(uint32_t* state) {
  uint32_t x = *state;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  *state = x;
  return x;
}

static int qcmp_int(const void* a, const void* b) {
  return (*(const int*)a) - (*(const int*)b);
}

static void test_tree_map_slab_random_stress(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4100));
  sAllocator alloc = use_slab(&s);

  TestTreeMapSlab t;
  AG_CHECK(TestTreeMapSlab_init(&alloc, &t));

  const int N = 4000;
  int* keys = (int*)malloc(N * sizeof(int));
  uint32_t state = 0xBEEFu;
  for (int i = 0; i < N; i++) keys[i] = i;

  for (int i = N - 1; i > 0; i--) {
    int j = (int)(xorshift_next(&state) % (uint32_t)(i + 1));
    int tmp = keys[i]; keys[i] = keys[j]; keys[j] = tmp;
  }

  for (int i = 0; i < N; i++)
    AG_CHECK(TestTreeMapSlab_insert(&t, keys[i], keys[i] * 2));

  AG_CHECK_EQ_INT(t.size, N);

  int* sortedKeys = (int*)malloc(N * sizeof(int));
  memcpy(sortedKeys, keys, N * sizeof(int));
  qsort(sortedKeys, N, sizeof(int), qcmp_int);

  int minK, minV, maxK, maxV;
  AG_CHECK(TestTreeMapSlab_min(&t, &minK, &minV)); AG_CHECK(minK == sortedKeys[0]);
  AG_CHECK(TestTreeMapSlab_max(&t, &maxK, &maxV)); AG_CHECK(maxK == sortedKeys[N - 1]);

  // Remove half of the keys, verifying they're really returned to the
  // slab (the removed count of blocks becomes available again below).
  for (int i = 0; i < N; i += 2)
    AG_CHECK(TestTreeMapSlab_remove(&t, keys[i]));

  AG_CHECK_EQ_INT(t.size, N / 2);

  for (int i = 0; i < N; i++) {
    int out;
    bool found = TestTreeMapSlab_find(&t, keys[i], &out);
    if (i % 2 == 0) AG_CHECK(!found);
    else            { AG_CHECK(found); AG_CHECK(out == keys[i] * 2); }
  }

  // Re-insert the removed half - only possible if slab_free actually
  // returned those N/2 node-blocks to the free list.
  for (int i = 0; i < N; i += 2)
    AG_CHECK(TestTreeMapSlab_insert(&t, keys[i], keys[i] * 3));
  AG_CHECK_EQ_INT(t.size, N);

  free(keys);
  free(sortedKeys);
  TestTreeMapSlab_free(&t);
  slab_free_all(&s);
}

static void test_tree_map_slab_exhaustion(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);
  // One block is consumed by the sentinel `nil` node at init time.
  size_t maxInserts = s.capacity - 1;

  TestTreeMapSlab t;
  AG_CHECK(TestTreeMapSlab_init(&alloc, &t));

  for (size_t i = 0; i < maxInserts; i++)
    AG_CHECK(TestTreeMapSlab_insert(&t, (int)i, (int)i));

  AG_CHECK(!TestTreeMapSlab_insert(&t, 999999, 0));
  AG_CHECK_EQ_INT(t.size, maxInserts);

  AG_CHECK(TestTreeMapSlab_remove(&t, 0));
  AG_CHECK(TestTreeMapSlab_insert(&t, 999999, 0));
  AG_CHECK_EQ_INT(t.size, maxInserts);

  TestTreeMapSlab_free(&t);
  slab_free_all(&s);
}

static void test_tree_map_slab_invalid_args(void) {
  AG_CHECK(TestTreeMapSlab_is_empty(NULL));

  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestTreeMapSlab t;
  AG_CHECK(!TestTreeMapSlab_init(NULL, &t));
  AG_CHECK(TestTreeMapSlab_init(&alloc, &t));

  int outK, outV;
  AG_CHECK(!TestTreeMapSlab_min(&t, &outK, &outV));
  AG_CHECK(!TestTreeMapSlab_max(&t, &outK, &outV));
  AG_CHECK(!TestTreeMapSlab_remove(&t, 1));

  TestTreeMapSlab_free(&t);
  slab_free_all(&s);
}

void run_tree_map_slab_tests(void) {
  test_tree_map_slab_init_accepts_slab();
  test_tree_map_slab_basic();
  test_tree_map_slab_ascending_insert();
  test_tree_map_slab_random_stress();
  test_tree_map_slab_exhaustion();
  test_tree_map_slab_invalid_args();
}

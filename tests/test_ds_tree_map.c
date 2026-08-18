#include "../include/ag.h"
#include "test_framework.h"
#include <stdlib.h>

static int cmp_int(int a, int b) { return a - b; }

TreeMap(int, int, TestTreeMap, cmp_int)

static void test_tree_map_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestTreeMap t;
  AG_CHECK(TestTreeMap_init(&alloc, &t));
  AG_CHECK(TestTreeMap_is_empty(&t));

  int keys[] = { 50, 30, 70, 20, 40, 60, 80, 10 };
  for (size_t i = 0; i < 8; i++)
    AG_CHECK(TestTreeMap_insert(&t, keys[i], keys[i] * 10));

  int out;
  AG_CHECK(TestTreeMap_find(&t, 40, &out)); AG_CHECK(out == 400);
  AG_CHECK(!TestTreeMap_find(&t, 999, &out));

  // Inserting an existing key must fail (not silently overwrite), per the
  // documented contract of this TreeMap.
  AG_CHECK(!TestTreeMap_insert(&t, 40, 111));
  AG_CHECK(TestTreeMap_find(&t, 40, &out)); AG_CHECK(out == 400);

  int minK, minV, maxK, maxV;
  AG_CHECK(TestTreeMap_min(&t, &minK, &minV)); AG_CHECK(minK == 10);
  AG_CHECK(TestTreeMap_max(&t, &maxK, &maxV)); AG_CHECK(maxK == 80);

  AG_CHECK(TestTreeMap_remove(&t, 40));
  AG_CHECK(!TestTreeMap_find(&t, 40, &out));
  AG_CHECK(!TestTreeMap_remove(&t, 40));

  TestTreeMap_free(&t);
  arena_free(&a);
}

static void test_tree_map_ascending_insert(void) {
  // Inserting strictly ascending keys is the classic pathological case
  // that turns an unbalanced BST into a linked list - this is exactly
  // what red-black rebalancing exists to prevent.
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestTreeMap t;
  AG_CHECK(TestTreeMap_init(&alloc, &t));

  const int N = 3000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestTreeMap_insert(&t, i, i));

  int minK, minV, maxK, maxV;
  AG_CHECK(TestTreeMap_min(&t, &minK, &minV)); AG_CHECK(minK == 0);
  AG_CHECK(TestTreeMap_max(&t, &maxK, &maxV)); AG_CHECK(maxK == N - 1);

  for (int i = 0; i < N; i++) {
    int out;
    AG_CHECK(TestTreeMap_find(&t, i, &out));
    AG_CHECK(out == i);
  }

  TestTreeMap_free(&t);
  arena_free(&a);
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

static void test_tree_map_random_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestTreeMap t;
  AG_CHECK(TestTreeMap_init(&alloc, &t));

  const int N = 4000;
  int* keys = (int*)malloc(N * sizeof(int));
  uint32_t state = 0xBEEFu;
  for (int i = 0; i < N; i++) keys[i] = i; // unique keys, shuffled below

  // Fisher-Yates shuffle for a randomized insertion order.
  for (int i = N - 1; i > 0; i--) {
    int j = (int)(xorshift_next(&state) % (uint32_t)(i + 1));
    int tmp = keys[i]; keys[i] = keys[j]; keys[j] = tmp;
  }

  for (int i = 0; i < N; i++)
    AG_CHECK(TestTreeMap_insert(&t, keys[i], keys[i] * 2));

  AG_CHECK_EQ_INT(t.size, N);

  int* sortedKeys = (int*)malloc(N * sizeof(int));
  memcpy(sortedKeys, keys, N * sizeof(int));
  qsort(sortedKeys, N, sizeof(int), qcmp_int);

  int minK, minV, maxK, maxV;
  AG_CHECK(TestTreeMap_min(&t, &minK, &minV)); AG_CHECK(minK == sortedKeys[0]);
  AG_CHECK(TestTreeMap_max(&t, &maxK, &maxV)); AG_CHECK(maxK == sortedKeys[N - 1]);

  // Remove a random half of the keys, checking min/max stay consistent
  // and that removed keys really vanish while survivors remain findable.
  for (int i = 0; i < N; i += 2)
    AG_CHECK(TestTreeMap_remove(&t, keys[i]));

  AG_CHECK_EQ_INT(t.size, N / 2);

  for (int i = 0; i < N; i++) {
    int out;
    bool found = TestTreeMap_find(&t, keys[i], &out);
    if (i % 2 == 0) AG_CHECK(!found);
    else            { AG_CHECK(found); AG_CHECK(out == keys[i] * 2); }
  }

  free(keys);
  free(sortedKeys);
  TestTreeMap_free(&t);
  arena_free(&a);
}

static void test_tree_map_invalid_args(void) {
  AG_CHECK(TestTreeMap_is_empty(NULL));

  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestTreeMap t;
  AG_CHECK(!TestTreeMap_init(NULL, &t));
  AG_CHECK(TestTreeMap_init(&alloc, &t));

  int outK, outV;
  AG_CHECK(!TestTreeMap_min(&t, &outK, &outV)); // empty tree
  AG_CHECK(!TestTreeMap_max(&t, &outK, &outV)); // empty tree
  AG_CHECK(!TestTreeMap_remove(&t, 1));         // empty tree

  TestTreeMap_free(&t);
  arena_free(&a);
}

void run_tree_map_tests(void) {
  test_tree_map_basic();
  test_tree_map_ascending_insert();
  test_tree_map_random_stress();
  test_tree_map_invalid_args();
}

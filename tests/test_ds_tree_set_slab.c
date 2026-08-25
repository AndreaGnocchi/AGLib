#include "../include/ag.h"
#include "test_framework.h"

// TreeSet is built directly on TreeMap, so it shares the same slab
// friendliness - see test_ds_tree_map_slab.c.

static int cmp_int(int a, int b) { return a - b; }

TreeSet(int, TestTreeSetSlab, cmp_int)

// TreeSet(T, name, cmp_fn) expands to TreeMap(T, bool, name##_tmap_, cmp_fn),
// so the generated node type is named name##_tmap_Node, not name##Node.
#define SLAB_BLOCK_SIZE sizeof(TestTreeSetSlab_tmap_Node)

static void test_tree_set_slab_init_accepts_slab(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestTreeSetSlab set;
  AG_CHECK(TestTreeSetSlab_init(&alloc, &set));
  AG_CHECK(TestTreeSetSlab_is_empty(&set));

  TestTreeSetSlab_free(&set);
  slab_free_all(&s);
}

static void test_tree_set_slab_basic(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestTreeSetSlab set;
  AG_CHECK(TestTreeSetSlab_init(&alloc, &set));

  int keys[] = { 5, 3, 8, 1, 4 };
  for (size_t i = 0; i < 5; i++)
    AG_CHECK(TestTreeSetSlab_insert(&set, keys[i]));

  AG_CHECK(TestTreeSetSlab_contains(&set, 4));
  AG_CHECK(!TestTreeSetSlab_contains(&set, 999));

  int minK, maxK;
  AG_CHECK(TestTreeSetSlab_min(&set, &minK)); AG_CHECK(minK == 1);
  AG_CHECK(TestTreeSetSlab_max(&set, &maxK)); AG_CHECK(maxK == 8);

  AG_CHECK(TestTreeSetSlab_remove(&set, 3));
  AG_CHECK(!TestTreeSetSlab_contains(&set, 3));

  TestTreeSetSlab_free(&set);
  slab_free_all(&s);
}

static void test_tree_set_slab_dedup(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestTreeSetSlab set;
  AG_CHECK(TestTreeSetSlab_init(&alloc, &set));

  AG_CHECK(TestTreeSetSlab_insert(&set, 7));
  AG_CHECK(!TestTreeSetSlab_insert(&set, 7)); // duplicate insert must fail
  AG_CHECK_EQ_INT(set.size, 1);

  TestTreeSetSlab_free(&set);
  slab_free_all(&s);
}

static void test_tree_set_slab_stress(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 3100));
  sAllocator alloc = use_slab(&s);

  TestTreeSetSlab set;
  AG_CHECK(TestTreeSetSlab_init(&alloc, &set));

  const int N = 3000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestTreeSetSlab_insert(&set, i));

  int minK, maxK;
  AG_CHECK(TestTreeSetSlab_min(&set, &minK)); AG_CHECK(minK == 0);
  AG_CHECK(TestTreeSetSlab_max(&set, &maxK)); AG_CHECK(maxK == N - 1);

  for (int i = 0; i < N; i += 2)
    AG_CHECK(TestTreeSetSlab_remove(&set, i));

  for (int i = 0; i < N; i++) {
    if (i % 2 == 0) AG_CHECK(!TestTreeSetSlab_contains(&set, i));
    else            AG_CHECK(TestTreeSetSlab_contains(&set, i));
  }

  // Removed half should be re-insertable, proving those node blocks were
  // actually returned to the slab rather than merely logically dropped.
  for (int i = 0; i < N; i += 2)
    AG_CHECK(TestTreeSetSlab_insert(&set, i));
  AG_CHECK_EQ_INT(set.size, N);

  TestTreeSetSlab_free(&set);
  slab_free_all(&s);
}

static void test_tree_set_slab_exhaustion(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);
  size_t maxInserts = s.capacity - 1; // one block reserved for `nil`

  TestTreeSetSlab set;
  AG_CHECK(TestTreeSetSlab_init(&alloc, &set));

  for (size_t i = 0; i < maxInserts; i++)
    AG_CHECK(TestTreeSetSlab_insert(&set, (int)i));

  AG_CHECK(!TestTreeSetSlab_insert(&set, 999999));
  AG_CHECK_EQ_INT(set.size, maxInserts);

  AG_CHECK(TestTreeSetSlab_remove(&set, 0));
  AG_CHECK(TestTreeSetSlab_insert(&set, 999999));
  AG_CHECK_EQ_INT(set.size, maxInserts);

  TestTreeSetSlab_free(&set);
  slab_free_all(&s);
}

static void test_tree_set_slab_invalid_args(void) {
  AG_CHECK(TestTreeSetSlab_is_empty(NULL));

  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestTreeSetSlab set;
  AG_CHECK(!TestTreeSetSlab_init(NULL, &set));
  AG_CHECK(TestTreeSetSlab_init(&alloc, &set));

  int outK;
  AG_CHECK(!TestTreeSetSlab_min(&set, &outK));
  AG_CHECK(!TestTreeSetSlab_max(&set, &outK));
  AG_CHECK(!TestTreeSetSlab_remove(&set, 1));

  TestTreeSetSlab_free(&set);
  slab_free_all(&s);
}

void run_tree_set_slab_tests(void) {
  test_tree_set_slab_init_accepts_slab();
  test_tree_set_slab_basic();
  test_tree_set_slab_dedup();
  test_tree_set_slab_stress();
  test_tree_set_slab_exhaustion();
  test_tree_set_slab_invalid_args();
}

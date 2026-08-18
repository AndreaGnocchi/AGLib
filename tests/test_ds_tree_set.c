#include "../include/ag.h"
#include "test_framework.h"
#include <stdlib.h>

static int cmp_int(int a, int b) { return a - b; }

TreeSet(int, TestTreeSet, cmp_int)

static void test_tree_set_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestTreeSet s;
  AG_CHECK(TestTreeSet_init(&alloc, &s));
  AG_CHECK(TestTreeSet_is_empty(&s));

  int keys[] = { 5, 3, 8, 1, 4 };
  for (size_t i = 0; i < 5; i++)
    AG_CHECK(TestTreeSet_insert(&s, keys[i]));

  AG_CHECK(TestTreeSet_contains(&s, 4));
  AG_CHECK(!TestTreeSet_contains(&s, 999));

  int minK, maxK;
  AG_CHECK(TestTreeSet_min(&s, &minK)); AG_CHECK(minK == 1);
  AG_CHECK(TestTreeSet_max(&s, &maxK)); AG_CHECK(maxK == 8);

  AG_CHECK(TestTreeSet_remove(&s, 3));
  AG_CHECK(!TestTreeSet_contains(&s, 3));

  TestTreeSet_free(&s);
  arena_free(&a);
}

static void test_tree_set_dedup(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestTreeSet s;
  AG_CHECK(TestTreeSet_init(&alloc, &s));

  AG_CHECK(TestTreeSet_insert(&s, 7));
  AG_CHECK(!TestTreeSet_insert(&s, 7)); // duplicate insert must fail
  AG_CHECK_EQ_INT(s.size, 1);

  TestTreeSet_free(&s);
  arena_free(&a);
}

static void test_tree_set_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestTreeSet s;
  AG_CHECK(TestTreeSet_init(&alloc, &s));

  const int N = 3000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestTreeSet_insert(&s, i));

  int minK, maxK;
  AG_CHECK(TestTreeSet_min(&s, &minK)); AG_CHECK(minK == 0);
  AG_CHECK(TestTreeSet_max(&s, &maxK)); AG_CHECK(maxK == N - 1);

  for (int i = 0; i < N; i += 2)
    AG_CHECK(TestTreeSet_remove(&s, i));

  for (int i = 0; i < N; i++) {
    if (i % 2 == 0) AG_CHECK(!TestTreeSet_contains(&s, i));
    else            AG_CHECK(TestTreeSet_contains(&s, i));
  }

  TestTreeSet_free(&s);
  arena_free(&a);
}

static void test_tree_set_invalid_args(void) {
  AG_CHECK(TestTreeSet_is_empty(NULL));

  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestTreeSet s;
  AG_CHECK(!TestTreeSet_init(NULL, &s));
  AG_CHECK(TestTreeSet_init(&alloc, &s));

  int outK;
  AG_CHECK(!TestTreeSet_min(&s, &outK)); // empty
  AG_CHECK(!TestTreeSet_max(&s, &outK)); // empty
  AG_CHECK(!TestTreeSet_remove(&s, 1));  // empty

  TestTreeSet_free(&s);
  arena_free(&a);
}

void run_tree_set_tests(void) {
  test_tree_set_basic();
  test_tree_set_dedup();
  test_tree_set_stress();
  test_tree_set_invalid_args();
}

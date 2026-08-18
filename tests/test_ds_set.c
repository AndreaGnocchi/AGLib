#include "../include/ag.h"
#include "test_framework.h"

static uint64_t hash_int(int x) {
  uint64_t h = (uint64_t)x;
  h += 0x9E3779B97F4A7C15ULL;
  h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
  h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
  return h ^ (h >> 31);
}
static bool eq_int(int a, int b) { return a == b; }

Set(int, TestSet, hash_int, eq_int)

static void test_set_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestSet s;
  AG_CHECK(TestSet_init(&alloc, &s, 8));
  AG_CHECK(TestSet_is_empty(&s));

  AG_CHECK(TestSet_insert(&s, 1));
  AG_CHECK(TestSet_insert(&s, 2));
  AG_CHECK(TestSet_contains(&s, 1));
  AG_CHECK(!TestSet_contains(&s, 999));

  AG_CHECK(TestSet_remove(&s, 1));
  AG_CHECK(!TestSet_contains(&s, 1));
  AG_CHECK(TestSet_contains(&s, 2));

  TestSet_free(&s);
  arena_free(&a);
}

static void test_set_dedup(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestSet s;
  AG_CHECK(TestSet_init(&alloc, &s, 8));

  // Inserting the same value repeatedly must not grow the set.
  for (int i = 0; i < 50; i++)
    AG_CHECK(TestSet_insert(&s, 42));

  AG_CHECK_EQ_INT(s.size, 1);
  AG_CHECK(TestSet_contains(&s, 42));

  TestSet_free(&s);
  arena_free(&a);
}

static void test_set_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestSet s;
  AG_CHECK(TestSet_init(&alloc, &s, 8));

  const int N = 3000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestSet_insert(&s, i));

  AG_CHECK_EQ_INT(s.size, N);
  for (int i = 0; i < N; i++)
    AG_CHECK(TestSet_contains(&s, i));

  for (int i = 0; i < N; i += 3)
    AG_CHECK(TestSet_remove(&s, i));

  for (int i = 0; i < N; i++) {
    if (i % 3 == 0) AG_CHECK(!TestSet_contains(&s, i));
    else            AG_CHECK(TestSet_contains(&s, i));
  }

  TestSet_free(&s);
  arena_free(&a);
}

static void test_set_invalid_args(void) {
  AG_CHECK(TestSet_is_empty(NULL));

  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestSet s;
  AG_CHECK(!TestSet_init(NULL, &s, 8));
  AG_CHECK(TestSet_init(&alloc, &s, 8));

  AG_CHECK(!TestSet_contains(&s, 1));  // empty set
  AG_CHECK(!TestSet_remove(&s, 1));    // empty set

  TestSet_free(&s);
  arena_free(&a);
}

void run_set_tests(void) {
  test_set_basic();
  test_set_dedup();
  test_set_stress();
  test_set_invalid_args();
}

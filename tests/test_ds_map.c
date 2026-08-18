#include "../include/ag.h"
#include "test_framework.h"
#include <stdlib.h>

static uint64_t hash_int(int x) {
  uint64_t h = (uint64_t)x;
  h += 0x9E3779B97F4A7C15ULL;
  h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
  h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
  return h ^ (h >> 31);
}
static bool eq_int(int a, int b) { return a == b; }

// Deliberately terrible hash to force heavy Robin-Hood probing/collisions.
static uint64_t hash_int_bad(int x) { (void)x; return 0; }

Map(int, int, TestMap, hash_int, eq_int)
Map(int, int, TestMapBadHash, hash_int_bad, eq_int)

static void test_map_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestMap m;
  AG_CHECK(TestMap_init(&alloc, &m, 8));
  AG_CHECK(TestMap_is_empty(&m));

  AG_CHECK(TestMap_insert(&m, 1, 100));
  AG_CHECK(TestMap_insert(&m, 2, 200));
  AG_CHECK(TestMap_insert(&m, 3, 300));

  int out;
  AG_CHECK(TestMap_get(&m, 2, &out)); AG_CHECK(out == 200);
  AG_CHECK(!TestMap_get(&m, 999, &out));

  // Insert on an existing key updates in place rather than duplicating.
  AG_CHECK(TestMap_insert(&m, 2, 999));
  AG_CHECK(TestMap_get(&m, 2, &out)); AG_CHECK(out == 999);
  AG_CHECK_EQ_INT(m.size, 3);

  AG_CHECK(TestMap_delete(&m, 2));
  AG_CHECK(!TestMap_get(&m, 2, &out));
  AG_CHECK(!TestMap_delete(&m, 2));

  TestMap_free(&m);
  arena_free(&a);
}

static void test_map_resize_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestMap m;
  AG_CHECK(TestMap_init(&alloc, &m, 8));

  for (int i = 100; i < 3000; i++)
    AG_CHECK(TestMap_insert(&m, i, i * 2));

  AG_CHECK(m.capacity > 8); // must have grown

  for (int i = 100; i < 3000; i++) {
    int out;
    AG_CHECK(TestMap_get(&m, i, &out));
    AG_CHECK(out == i * 2);
  }

  // Delete every other key, verify survivors are intact.
  for (int i = 100; i < 3000; i += 2)
    AG_CHECK(TestMap_delete(&m, i));

  for (int i = 100; i < 3000; i++) {
    int out;
    bool found = TestMap_get(&m, i, &out);
    if (i % 2 == 0) {
      AG_CHECK(!found);
    } else {
      AG_CHECK(found);
      AG_CHECK(out == i * 2);
    }
  }

  TestMap_free(&m);
  arena_free(&a);
}

static void test_map_heavy_collisions(void) {
  // Every key hashes to the same bucket - this stresses Robin Hood
  // displacement and the delete backward-shift loop specifically.
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestMapBadHash m;
  AG_CHECK(TestMapBadHash_init(&alloc, &m, 8));

  const int N = 300;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestMapBadHash_insert(&m, i, i + 1));

  for (int i = 0; i < N; i++) {
    int out;
    AG_CHECK(TestMapBadHash_get(&m, i, &out));
    AG_CHECK(out == i + 1);
  }

  // Delete from the middle of the collision chain and re-verify neighbours.
  for (int i = 100; i < 200; i++)
    AG_CHECK(TestMapBadHash_delete(&m, i));

  for (int i = 0; i < N; i++) {
    int out;
    bool found = TestMapBadHash_get(&m, i, &out);
    if (i >= 100 && i < 200) AG_CHECK(!found);
    else                     { AG_CHECK(found); AG_CHECK(out == i + 1); }
  }

  TestMapBadHash_free(&m);
  arena_free(&a);
}

static void test_map_invalid_args(void) {
  AG_CHECK(TestMap_is_empty(NULL));

  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestMap m;
  AG_CHECK(!TestMap_init(NULL, &m, 8));
  AG_CHECK(!TestMap_init(&alloc, &m, 0));

  AG_CHECK(TestMap_init(&alloc, &m, 8));
  int out;
  AG_CHECK(!TestMap_get(&m, 1, &out));   // empty map
  AG_CHECK(!TestMap_delete(&m, 1));      // empty map
  AG_CHECK(!TestMap_insert(NULL, 1, 1));

  TestMap_free(&m);
  arena_free(&a);
}

void run_map_tests(void) {
  test_map_basic();
  test_map_resize_stress();
  test_map_heavy_collisions();
  test_map_invalid_args();
}

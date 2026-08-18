#include "../include/ag.h"
#include "test_framework.h"
#include <string.h>
#include <stdint.h>

static void test_arena_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, KB(4)));

  int* p1 = (int*)arena_alloc(&a, sizeof(int), true);
  AG_CHECK(p1 != NULL);
  AG_CHECK(*p1 == 0);

  *p1 = 42;
  int* p2 = (int*)arena_alloc(&a, sizeof(int), false);
  AG_CHECK(p2 != NULL);
  AG_CHECK(p1 != p2);

  arena_free(&a);
}

static void test_arena_alignment(void) {
  sArena a;
  AG_CHECK(arena_init(&a, KB(4)));

  // Force an odd offset, then check every requested alignment is honoured.
  (void)arena_alloc(&a, 1, false);

  size_t alignments[] = { 1, 2, 4, 8, 16, 32, 64 };
  for (size_t i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
    void* p = arena_alloc_aligned(&a, 16, false, alignments[i]);
    AG_CHECK(p != NULL);
    AG_CHECK(((uintptr_t)p % alignments[i]) == 0);
  }

  // Non-power-of-two alignment must be rejected.
  AG_CHECK(arena_alloc_aligned(&a, 16, false, 3) == NULL);
  AG_CHECK(arena_alloc_aligned(&a, 16, false, 0) == NULL);

  arena_free(&a);
}

static void test_arena_reset(void) {
  sArena a;
  AG_CHECK(arena_init(&a, KB(4)));

  (void)arena_alloc(&a, 256, false);
  AG_CHECK(a.offset != 0);

  arena_reset(&a);
  AG_CHECK(a.offset == 0);

  // Memory should be reusable after reset.
  void* p = arena_alloc(&a, 256, false);
  AG_CHECK(p != NULL);

  arena_free(&a);
}

static void test_arena_temp(void) {
  sArena a;
  AG_CHECK(arena_init(&a, KB(4)));

  void* before = arena_alloc(&a, 8, false);
  (void)before;
  size_t offsetBeforeTemp = a.offset;

  sTempArena temp = arena_temp_start(&a);
  (void)arena_alloc(&a, 256, false);
  AG_CHECK(a.offset != offsetBeforeTemp);
  arena_temp_end(temp);
  AG_CHECK(a.offset == offsetBeforeTemp);

  arena_free(&a);
}

static void test_arena_temp_nested(void) {
  sArena a;
  AG_CHECK(arena_init(&a, KB(4)));

  size_t base = a.offset;

  sTempArena outer = arena_temp_start(&a);
  (void)arena_alloc(&a, 64, false);
  size_t afterOuterAlloc = a.offset;

  sTempArena inner = arena_temp_start(&a);
  (void)arena_alloc(&a, 128, false);
  AG_CHECK(a.offset != afterOuterAlloc);

  arena_temp_end(inner);
  AG_CHECK(a.offset == afterOuterAlloc);

  arena_temp_end(outer);
  AG_CHECK(a.offset == base);

  arena_free(&a);
}

static void test_arena_exhaustion(void) {
  sArena a;
  AG_CHECK(arena_init(&a, KB(4)));

  // Ask for more than the arena could possibly hold - must fail cleanly,
  // not overflow or corrupt state.
  void* huge = arena_alloc(&a, MB(64), false);
  AG_CHECK(huge == NULL);

  // Arena should still be usable for a reasonable allocation afterwards.
  void* ok = arena_alloc(&a, 16, false);
  AG_CHECK(ok != NULL);

  arena_free(&a);
}

static void test_arena_invalid_args(void) {
  AG_CHECK(arena_init(NULL, KB(4)) == false);

  sArena a;
  AG_CHECK(arena_init(&a, 0) == false);

  AG_CHECK(arena_init(&a, KB(4)));
  AG_CHECK(arena_alloc(&a, 0, false) == NULL);
  AG_CHECK(arena_alloc(NULL, 16, false) == NULL);

  // Double free / free-after-free should not crash.
  arena_free(&a);
  arena_free(&a);

  // Operating on a freed arena should be a safe no-op, not a crash.
  arena_reset(&a);

  // NULL-safety on the temp helpers.
  sTempArena badTemp = arena_temp_start(NULL);
  arena_temp_end(badTemp);
}

void run_arena_tests(void) {
  test_arena_basic();
  test_arena_alignment();
  test_arena_reset();
  test_arena_temp();
  test_arena_temp_nested();
  test_arena_exhaustion();
  test_arena_invalid_args();
}

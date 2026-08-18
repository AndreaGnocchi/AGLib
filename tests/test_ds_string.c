#include "../include/ag.h"
#include "test_framework.h"
#include <string.h>

String(TestStr)

static void test_string_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "hello"));
  AG_CHECK_STR_EQ(TestStr_cstr(&s), "hello");

  AG_CHECK(TestStr_append(&s, " world"));
  AG_CHECK_STR_EQ(TestStr_cstr(&s), "hello world");

  AG_CHECK(TestStr_appendf(&alloc, &s, " %d/%d", 1, 2));
  AG_CHECK_STR_EQ(TestStr_cstr(&s), "hello world 1/2");

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_slicing(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "hello world"));

  sStringView v = TestStr_slice(&s, 0, 5);
  AG_CHECK_EQ_INT(v.len, 5);
  AG_CHECK(memcmp(v.ptr, "hello", 5) == 0);

  sStringView v2 = TestStr_slice(&s, 6, 5);
  AG_CHECK_EQ_INT(v2.len, 5);
  AG_CHECK(memcmp(v2.ptr, "world", 5) == 0);

  // Out-of-range slices must fail cleanly.
  sStringView bad = TestStr_slice(&s, 1000, 5);
  AG_CHECK(bad.ptr == NULL);

  sStringView badLen = TestStr_slice(&s, 0, 1000);
  AG_CHECK(badLen.ptr == NULL);

  sStringView zeroLen = TestStr_slice(&s, 0, 0);
  AG_CHECK(zeroLen.ptr == NULL);

  // A slice that exactly spans the whole string is valid.
  sStringView full = TestStr_slice(&s, 0, s.size);
  AG_CHECK_EQ_INT(full.len, s.size);

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_growth_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "x"));

  // Repeated small appends force many reallocations.
  for (int i = 0; i < 2000; i++)
    AG_CHECK(TestStr_append(&s, "ab"));

  AG_CHECK_EQ_INT(s.size, 1 + 2000 * 2);
  AG_CHECK(TestStr_cstr(&s)[0] == 'x');
  AG_CHECK(TestStr_cstr(&s)[s.size - 1] == 'b');

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_appendf_large(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, ""));

  // appendf must correctly grow the buffer for output larger than the
  // current capacity, not just small strings.
  AG_CHECK(TestStr_appendf(&alloc, &s, "%0300d", 7));
  AG_CHECK_EQ_INT(s.size, 300);

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_invalid_args(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(!TestStr_init(NULL, &s, "x"));
  AG_CHECK(!TestStr_init(&alloc, &s, NULL));
  AG_CHECK(!TestStr_init(&alloc, NULL, "x"));

  AG_CHECK(TestStr_init(&alloc, &s, "ok"));
  AG_CHECK(!TestStr_append(&s, NULL));
  AG_CHECK(!TestStr_append(NULL, "x"));
  AG_CHECK(TestStr_cstr(NULL) == NULL);

  TestStr_free(&s);
  arena_free(&a);
}

void run_string_tests(void) {
  test_string_basic();
  test_string_slicing();
  test_string_growth_stress();
  test_string_appendf_large();
  test_string_invalid_args();
}

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
  AG_CHECK_STR_EQ(s.items, "hello");

  AG_CHECK(TestStr_append(&s, " world"));
  AG_CHECK_STR_EQ(s.items, "hello world");

  AG_CHECK(TestStr_appendf(&s, " %d/%d", 1, 2));
  AG_CHECK_STR_EQ(s.items, "hello world 1/2");

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_shortest_capacity(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  // Short strings should still reserve AG_SHORTEST_STRING bytes up front,
  // so a small init doesn't force an immediate reallocation on append.
  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "hi"));
  AG_CHECK(s.capacity >= 32);
  AG_CHECK_EQ_INT(s.size, 2);

  // A string longer than AG_SHORTEST_STRING should size to fit instead.
  const char* longInit = "this initial string is longer than thirty two bytes";
  TestStr s2;
  AG_CHECK(TestStr_init(&alloc, &s2, longInit));
  AG_CHECK(s2.capacity >= strlen(longInit) + 1);
  AG_CHECK_STR_EQ(s2.items, longInit);

  TestStr_free(&s);
  TestStr_free(&s2);
  arena_free(&a);
}

static void test_string_case_conversion(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "Hello, World! 123"));

  TestStr_to_lower(&s);
  AG_CHECK_STR_EQ(s.items, "hello, world! 123");

  TestStr_to_upper(&s);
  AG_CHECK_STR_EQ(s.items, "HELLO, WORLD! 123");

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_normalize(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "  hello  world. this is a TEST!"));

  TestStr_normalize(&s);
  AG_CHECK_STR_EQ(s.items, "Hello world. This is a test!");
  AG_CHECK_EQ_INT(s.size, (long long)strlen("Hello world. This is a test!"));

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_search(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "the quick brown fox jumps over the lazy dog"));

  size_t idx = 0;
  AG_CHECK(TestStr_search_first(&s, "the", &idx));
  AG_CHECK_EQ_INT(idx, 0);

  AG_CHECK(TestStr_search_last(&s, "the", &idx));
  AG_CHECK_EQ_INT(idx, 31);

  AG_CHECK(TestStr_search_first_from(&s, "the", 1, &idx));
  AG_CHECK_EQ_INT(idx, 31);

  AG_CHECK(!TestStr_search_first(&s, "cat", &idx));
  AG_CHECK(!TestStr_search_last(&s, "cat", &idx));

  TestStr_free(&s);
  arena_free(&a);
}

typedef struct {
  size_t hits[8];
  size_t count;
} sSearchAllCtx;

static bool on_match(size_t idx, void* userdata) {
  sSearchAllCtx* ctx = (sSearchAllCtx*)userdata;
  if (ctx->count < 8) ctx->hits[ctx->count++] = idx;
  return true;
}

static void test_string_search_all(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "abcabcabc"));

  sSearchAllCtx ctx = {0};
  TestStr_search_all(&s, "abc", on_match, &ctx);

  AG_CHECK_EQ_INT(ctx.count, 3);
  if (ctx.count == 3) {
    AG_CHECK_EQ_INT(ctx.hits[0], 0);
    AG_CHECK_EQ_INT(ctx.hits[1], 3);
    AG_CHECK_EQ_INT(ctx.hits[2], 6);
  }

  // Overlapping matches must not be double-counted: "aa" in "aaaa" only
  // matches at 0 and 2, since the matcher advances past each hit.
  TestStr s2;
  AG_CHECK(TestStr_init(&alloc, &s2, "aaaa"));
  sSearchAllCtx ctx2 = {0};
  TestStr_search_all(&s2, "aa", on_match, &ctx2);
  AG_CHECK_EQ_INT(ctx2.count, 2);

  TestStr_free(&s);
  TestStr_free(&s2);
  arena_free(&a);
}

static void test_string_replace(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "cat sat on the cat mat"));

  AG_CHECK(TestStr_replace_first(&s, "cat", "dog"));
  AG_CHECK_STR_EQ(s.items, "dog sat on the cat mat");

  AG_CHECK(TestStr_replace_last(&s, "cat", "hat"));
  AG_CHECK_STR_EQ(s.items, "dog sat on the hat mat");

  // Replacement longer than the needle forces growth.
  AG_CHECK(TestStr_replace_first(&s, "dog", "elephant"));
  AG_CHECK_STR_EQ(s.items, "elephant sat on the hat mat");

  // Replacement shorter than the needle shrinks the string.
  AG_CHECK(TestStr_replace_first(&s, "elephant", "cat"));
  AG_CHECK_STR_EQ(s.items, "cat sat on the hat mat");

  AG_CHECK(!TestStr_replace_first(&s, "zzz", "x"));

  TestStr_free(&s);
  arena_free(&a);
}

static void test_string_replace_all(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "one two one two one"));

  size_t n = TestStr_replace_all(&s, "one", "1");
  AG_CHECK_EQ_INT(n, 3);
  AG_CHECK_STR_EQ(s.items, "1 two 1 two 1");

  TestStr s2;
  AG_CHECK(TestStr_init(&alloc, &s2, "x-x-x-x"));
  size_t n2 = TestStr_replace_all(&s2, "x", "yyy");
  AG_CHECK_EQ_INT(n2, 4);
  AG_CHECK_STR_EQ(s2.items, "yyy-yyy-yyy-yyy");

  TestStr_free(&s);
  TestStr_free(&s2);
  arena_free(&a);
}

static void test_string_cstr_copy(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStr s;
  AG_CHECK(TestStr_init(&alloc, &s, "hello world"));

  char buf[64];
  AG_CHECK(TestStr_cstr_copy(&s, buf, sizeof(buf)));
  AG_CHECK_STR_EQ(buf, "hello world");

  // A buffer smaller than the string should truncate, not overflow.
  char small[4];
  AG_CHECK(TestStr_cstr_copy(&s, small, sizeof(small)));
  AG_CHECK_EQ_INT(strlen(small), 3);
  AG_CHECK(small[3] == '\0');

  AG_CHECK(!TestStr_cstr_copy(&s, NULL, 10));
  AG_CHECK(!TestStr_cstr_copy(&s, buf, 0));

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
  AG_CHECK(s.items[0] == 'x');
  AG_CHECK(s.items[s.size - 1] == 'b');

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
  AG_CHECK(TestStr_appendf(&s, "%0300d", 7));
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

  size_t idx = 0;
  AG_CHECK(!TestStr_search_first(NULL, "o", &idx));
  AG_CHECK(!TestStr_search_first(&s, NULL, &idx));
  AG_CHECK(!TestStr_replace_first(&s, NULL, "x"));
  AG_CHECK(!TestStr_replace_first(&s, "o", NULL));

  TestStr_free(&s);
  arena_free(&a);
}

void run_string_tests(void) {
  test_string_basic();
  test_string_shortest_capacity();
  test_string_case_conversion();
  test_string_normalize();
  test_string_search();
  test_string_search_all();
  test_string_replace();
  test_string_replace_all();
  test_string_cstr_copy();
  test_string_growth_stress();
  test_string_appendf_large();
  test_string_invalid_args();
}

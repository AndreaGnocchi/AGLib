#include "../include/ag.h"
#include "test_framework.h"

Stack(int, TestStack)

static void test_stack_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStack s;
  AG_CHECK(TestStack_init(&alloc, &s));
  AG_CHECK(TestStack_is_empty(&s));

  AG_CHECK(TestStack_push(&s, 1));
  AG_CHECK(TestStack_push(&s, 2));
  AG_CHECK(TestStack_push(&s, 3));

  int v;
  AG_CHECK(TestStack_peek(&s, &v)); AG_CHECK(v == 3);

  AG_CHECK(TestStack_pop(&s, &v)); AG_CHECK(v == 3);
  AG_CHECK(TestStack_pop(&s, &v)); AG_CHECK(v == 2);
  AG_CHECK(TestStack_pop(&s, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestStack_is_empty(&s));
  AG_CHECK(!TestStack_pop(&s, &v));

  TestStack_free(&s);
  arena_free(&a);
}

static void test_stack_lifo_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestStack s;
  AG_CHECK(TestStack_init(&alloc, &s));

  const int N = 5000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestStack_push(&s, i));

  AG_CHECK_EQ_INT(s.size, N);

  for (int i = N - 1; i >= 0; i--) {
    int v;
    AG_CHECK(TestStack_pop(&s, &v));
    AG_CHECK(v == i);
  }
  AG_CHECK(TestStack_is_empty(&s));

  TestStack_free(&s);
  arena_free(&a);
}

static void test_stack_invalid_args(void) {
  AG_CHECK(TestStack_is_empty(NULL));

  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestStack s;
  AG_CHECK(!TestStack_init(NULL, &s));
  AG_CHECK(TestStack_init(&alloc, &s));

  int v;
  AG_CHECK(!TestStack_peek(&s, &v));
  AG_CHECK(!TestStack_pop(&s, &v));
  AG_CHECK(!TestStack_pop(NULL, &v));

  TestStack_free(&s);
  arena_free(&a);
}

void run_stack_tests(void) {
  test_stack_basic();
  test_stack_lifo_stress();
  test_stack_invalid_args();
}

#include "../include/ag.h"
#include "test_framework.h"

// Stack is built directly on LinkedList, so it inherits LinkedList's new
// support for a slab-backed sAllocator. See test_ds_queue_slab.c for why
// 3*sizeof(void*) is the right block size.

Stack(int, TestStackSlab)

#define SLAB_BLOCK_SIZE (3 * sizeof(void*))

static void test_stack_slab_init_accepts_slab(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestStackSlab st;
  AG_CHECK(TestStackSlab_init(&alloc, &st));
  AG_CHECK(TestStackSlab_is_empty(&st));

  TestStackSlab_free(&st);
  slab_free_all(&s);
}

static void test_stack_slab_basic(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestStackSlab st;
  AG_CHECK(TestStackSlab_init(&alloc, &st));

  AG_CHECK(TestStackSlab_push(&st, 1));
  AG_CHECK(TestStackSlab_push(&st, 2));
  AG_CHECK(TestStackSlab_push(&st, 3));

  int v;
  AG_CHECK(TestStackSlab_peek(&st, &v)); AG_CHECK(v == 3);

  AG_CHECK(TestStackSlab_pop(&st, &v)); AG_CHECK(v == 3);
  AG_CHECK(TestStackSlab_pop(&st, &v)); AG_CHECK(v == 2);
  AG_CHECK(TestStackSlab_pop(&st, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestStackSlab_is_empty(&st));
  AG_CHECK(!TestStackSlab_pop(&st, &v));

  TestStackSlab_free(&st);
  slab_free_all(&s);
}

static void test_stack_slab_lifo_bounded_reuse(void) {
  // A tiny slab relative to N below. Never holding more than one item
  // alive at a time, we push+pop N times - far more operations than
  // there are blocks. This only works if every pop genuinely returns
  // its node's blocks to the slab for the next push to reuse.
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);

  TestStackSlab st;
  AG_CHECK(TestStackSlab_init(&alloc, &st));

  const int N = 5000;
  for (int i = 0; i < N; i++) {
    AG_CHECK(TestStackSlab_push(&st, i));
    int v;
    AG_CHECK(TestStackSlab_pop(&st, &v));
    AG_CHECK(v == i);
  }
  AG_CHECK(TestStackSlab_is_empty(&st));

  TestStackSlab_free(&st);
  slab_free_all(&s);
}

static void test_stack_slab_exhaustion(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);
  size_t maxItems = s.capacity / 2; // 2 blocks (value + node) per item

  TestStackSlab st;
  AG_CHECK(TestStackSlab_init(&alloc, &st));

  for (size_t i = 0; i < maxItems; i++)
    AG_CHECK(TestStackSlab_push(&st, (int)i));

  AG_CHECK(!TestStackSlab_push(&st, 999));
  AG_CHECK_EQ_INT(st.size, maxItems);

  int v;
  AG_CHECK(TestStackSlab_pop(&st, &v));
  AG_CHECK(TestStackSlab_push(&st, 999));
  AG_CHECK_EQ_INT(st.size, maxItems);

  TestStackSlab_free(&st);
  slab_free_all(&s);
}

static void test_stack_slab_invalid_args(void) {
  AG_CHECK(TestStackSlab_is_empty(NULL));

  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestStackSlab st;
  AG_CHECK(!TestStackSlab_init(NULL, &st));
  AG_CHECK(TestStackSlab_init(&alloc, &st));

  int v;
  AG_CHECK(!TestStackSlab_peek(&st, &v));
  AG_CHECK(!TestStackSlab_pop(&st, &v));
  AG_CHECK(!TestStackSlab_pop(NULL, &v));

  TestStackSlab_free(&st);
  slab_free_all(&s);
}

void run_stack_slab_tests(void) {
  test_stack_slab_init_accepts_slab();
  test_stack_slab_basic();
  test_stack_slab_lifo_bounded_reuse();
  test_stack_slab_exhaustion();
  test_stack_slab_invalid_args();
}

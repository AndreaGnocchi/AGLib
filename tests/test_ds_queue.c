#include "../include/ag.h"
#include "test_framework.h"

Queue(int, TestQueue)

static void test_queue_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestQueue q;
  AG_CHECK(TestQueue_init(&alloc, &q));
  AG_CHECK(TestQueue_is_empty(&q));

  AG_CHECK(TestQueue_enqueue(&q, 1));
  AG_CHECK(TestQueue_enqueue(&q, 2));
  AG_CHECK(TestQueue_enqueue(&q, 3));

  int v;
  AG_CHECK(TestQueue_peek(&q, &v)); AG_CHECK(v == 1);

  AG_CHECK(TestQueue_dequeue(&q, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestQueue_dequeue(&q, &v)); AG_CHECK(v == 2);
  AG_CHECK(TestQueue_dequeue(&q, &v)); AG_CHECK(v == 3);
  AG_CHECK(TestQueue_is_empty(&q));
  AG_CHECK(!TestQueue_dequeue(&q, &v));

  TestQueue_free(&q);
  arena_free(&a);
}

static void test_queue_fifo_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestQueue q;
  AG_CHECK(TestQueue_init(&alloc, &q));

  const int N = 5000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestQueue_enqueue(&q, i));

  AG_CHECK_EQ_INT(q.size, N);

  for (int i = 0; i < N; i++) {
    int v;
    AG_CHECK(TestQueue_dequeue(&q, &v));
    AG_CHECK(v == i);
  }
  AG_CHECK(TestQueue_is_empty(&q));

  TestQueue_free(&q);
  arena_free(&a);
}

static void test_queue_interleaved(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestQueue q;
  AG_CHECK(TestQueue_init(&alloc, &q));

  // Interleave enqueue/dequeue to exercise both ends repeatedly, tracking
  // the actual expected FIFO order ourselves (the queue accumulates a
  // backlog since each round enqueues 2 and dequeues only 1).
  int expected[200];
  int expectedCount = 0;
  int nextExpectedIdx = 0;

  for (int round = 0; round < 100; round++) {
    AG_CHECK(TestQueue_enqueue(&q, round));
    expected[expectedCount++] = round;

    AG_CHECK(TestQueue_enqueue(&q, round + 1000));
    expected[expectedCount++] = round + 1000;

    int v;
    AG_CHECK(TestQueue_dequeue(&q, &v));
    AG_CHECK(v == expected[nextExpectedIdx++]);
  }
  AG_CHECK_EQ_INT(q.size, expectedCount - nextExpectedIdx);

  // Drain the rest and confirm the remaining FIFO order holds.
  int v;
  while (TestQueue_dequeue(&q, &v))
    AG_CHECK(v == expected[nextExpectedIdx++]);

  AG_CHECK_EQ_INT(nextExpectedIdx, expectedCount);

  TestQueue_free(&q);
  arena_free(&a);
}

static void test_queue_invalid_args(void) {
  AG_CHECK(TestQueue_is_empty(NULL));

  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestQueue q;
  AG_CHECK(!TestQueue_init(NULL, &q));
  AG_CHECK(TestQueue_init(&alloc, &q));

  int v;
  AG_CHECK(!TestQueue_peek(&q, &v));
  AG_CHECK(!TestQueue_dequeue(&q, &v));

  TestQueue_free(&q);
  arena_free(&a);
}

void run_queue_tests(void) {
  test_queue_basic();
  test_queue_fifo_stress();
  test_queue_interleaved();
  test_queue_invalid_args();
}

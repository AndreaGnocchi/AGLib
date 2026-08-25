#include "../include/ag.h"
#include "test_framework.h"

// Queue is built directly on LinkedList, so it inherits LinkedList's new
// support for a slab-backed sAllocator. Each internal node is
// { T* val; nameNode* next; nameNode* prev; } - three pointers - so a
// blockSize of 3*sizeof(void*) comfortably fits both that node and a
// standalone T (int) allocation.

Queue(int, TestQueueSlab)

#define SLAB_BLOCK_SIZE (3 * sizeof(void*))

static void test_queue_slab_init_accepts_slab(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestQueueSlab q;
  AG_CHECK(TestQueueSlab_init(&alloc, &q));
  AG_CHECK(TestQueueSlab_is_empty(&q));

  TestQueueSlab_free(&q);
  slab_free_all(&s);
}

static void test_queue_slab_basic(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestQueueSlab q;
  AG_CHECK(TestQueueSlab_init(&alloc, &q));

  AG_CHECK(TestQueueSlab_enqueue(&q, 1));
  AG_CHECK(TestQueueSlab_enqueue(&q, 2));
  AG_CHECK(TestQueueSlab_enqueue(&q, 3));

  int v;
  AG_CHECK(TestQueueSlab_peek(&q, &v)); AG_CHECK(v == 1);

  AG_CHECK(TestQueueSlab_dequeue(&q, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestQueueSlab_dequeue(&q, &v)); AG_CHECK(v == 2);
  AG_CHECK(TestQueueSlab_dequeue(&q, &v)); AG_CHECK(v == 3);
  AG_CHECK(TestQueueSlab_is_empty(&q));
  AG_CHECK(!TestQueueSlab_dequeue(&q, &v));

  TestQueueSlab_free(&q);
  slab_free_all(&s);
}

static void test_queue_slab_fifo_bounded_reuse(void) {
  // A tiny slab relative to N below. Never holding more than one item
  // alive at a time, we enqueue+dequeue N times - far more operations
  // than there are blocks. This only survives if every dequeue genuinely
  // returns its node's blocks to the slab for the next enqueue to reuse.
  // (test_queue_slab_interleaved below covers multi-item FIFO ordering.)
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);

  TestQueueSlab q;
  AG_CHECK(TestQueueSlab_init(&alloc, &q));

  const int N = 5000;
  for (int i = 0; i < N; i++) {
    AG_CHECK(TestQueueSlab_enqueue(&q, i));
    int v;
    AG_CHECK(TestQueueSlab_dequeue(&q, &v));
    AG_CHECK(v == i);
  }
  AG_CHECK(TestQueueSlab_is_empty(&q));

  TestQueueSlab_free(&q);
  slab_free_all(&s);
}

static void test_queue_slab_interleaved(void) {
  // Each round enqueues 2 and dequeues 1, so live items grow by 1 every
  // round - after 100 rounds the queue holds 100 live items (200
  // blocks). Request enough blocks up front to comfortably cover that
  // peak; slab_init only ever rounds nBlocks up (to a full page), never
  // down, so this is a safe floor regardless of the host's page size.
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 220));
  sAllocator alloc = use_slab(&s);

  TestQueueSlab q;
  AG_CHECK(TestQueueSlab_init(&alloc, &q));

  int expected[200];
  int expectedCount = 0;
  int nextExpectedIdx = 0;

  for (int round = 0; round < 100; round++) {
    AG_CHECK(TestQueueSlab_enqueue(&q, round));
    expected[expectedCount++] = round;

    AG_CHECK(TestQueueSlab_enqueue(&q, round + 1000));
    expected[expectedCount++] = round + 1000;

    int v;
    AG_CHECK(TestQueueSlab_dequeue(&q, &v));
    AG_CHECK(v == expected[nextExpectedIdx++]);
  }
  AG_CHECK_EQ_INT(q.size, expectedCount - nextExpectedIdx);

  int v;
  while (TestQueueSlab_dequeue(&q, &v))
    AG_CHECK(v == expected[nextExpectedIdx++]);

  AG_CHECK_EQ_INT(nextExpectedIdx, expectedCount);

  TestQueueSlab_free(&q);
  slab_free_all(&s);
}

static void test_queue_slab_exhaustion(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);
  size_t maxItems = s.capacity / 2; // 2 blocks (value + node) per item

  TestQueueSlab q;
  AG_CHECK(TestQueueSlab_init(&alloc, &q));

  for (size_t i = 0; i < maxItems; i++)
    AG_CHECK(TestQueueSlab_enqueue(&q, (int)i));

  AG_CHECK(!TestQueueSlab_enqueue(&q, 999));
  AG_CHECK_EQ_INT(q.size, maxItems);

  int v;
  AG_CHECK(TestQueueSlab_dequeue(&q, &v));
  AG_CHECK(TestQueueSlab_enqueue(&q, 999));
  AG_CHECK_EQ_INT(q.size, maxItems);

  TestQueueSlab_free(&q);
  slab_free_all(&s);
}

static void test_queue_slab_invalid_args(void) {
  AG_CHECK(TestQueueSlab_is_empty(NULL));

  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestQueueSlab q;
  AG_CHECK(!TestQueueSlab_init(NULL, &q));
  AG_CHECK(TestQueueSlab_init(&alloc, &q));

  int v;
  AG_CHECK(!TestQueueSlab_peek(&q, &v));
  AG_CHECK(!TestQueueSlab_dequeue(&q, &v));

  TestQueueSlab_free(&q);
  slab_free_all(&s);
}

void run_queue_slab_tests(void) {
  test_queue_slab_init_accepts_slab();
  test_queue_slab_basic();
  test_queue_slab_fifo_bounded_reuse();
  test_queue_slab_interleaved();
  test_queue_slab_exhaustion();
  test_queue_slab_invalid_args();
}

#include "../include/ag.h"
#include "test_framework.h"

// LinkedList now accepts a slab-backed sAllocator (unlike DynamicArray and
// Map, which still reject one - they need to grow a single backing block
// past a fixed size, which a slab structurally cannot do). Every node the
// list allocates is a fixed size, so a slab is actually a *better* fit than
// an arena here: individual nodes really get returned to the free list and
// reused on push_head/push_tail after a pop/remove, instead of only being
// reclaimable in bulk.

LinkedList(int, TestLinkedListSlab)

// One push allocates two fixed-size blocks from the slab: a copy of the
// value (sizeof(T)) and the node itself (sizeof(nameNode)). Since T (int)
// is always smaller than the node struct here, sizing the slab's blocks
// off the node type covers both allocations.
#define SLAB_BLOCK_SIZE sizeof(TestLinkedListSlabNode)

static void test_linked_list_slab_init_accepts_slab(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestLinkedListSlab list;
  // This is the headline behavior change: a slab-backed allocator used to
  // be rejected outright by LinkedList_init; it must now succeed.
  AG_CHECK(TestLinkedListSlab_init(&alloc, &list));
  AG_CHECK(TestLinkedListSlab_is_empty(&list));

  TestLinkedListSlab_free(&list);
  slab_free_all(&s);
}

static void test_linked_list_slab_basic(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestLinkedListSlab list;
  AG_CHECK(TestLinkedListSlab_init(&alloc, &list));

  AG_CHECK(TestLinkedListSlab_push_head(&list, 2));
  AG_CHECK(TestLinkedListSlab_push_head(&list, 1));
  AG_CHECK(TestLinkedListSlab_push_tail(&list, 3));

  int v;
  AG_CHECK(TestLinkedListSlab_peek_head(&list, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestLinkedListSlab_peek_tail(&list, &v)); AG_CHECK(v == 3);
  AG_CHECK_EQ_INT(list.size, 3);

  AG_CHECK(TestLinkedListSlab_pop_head(&list, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestLinkedListSlab_pop_tail(&list, &v)); AG_CHECK(v == 3);
  AG_CHECK_EQ_INT(list.size, 1);

  AG_CHECK(TestLinkedListSlab_pop_head(&list, &v)); AG_CHECK(v == 2);
  AG_CHECK(TestLinkedListSlab_is_empty(&list));
  AG_CHECK(!TestLinkedListSlab_pop_head(&list, &v));
  AG_CHECK(!TestLinkedListSlab_pop_tail(&list, &v));

  TestLinkedListSlab_free(&list);
  slab_free_all(&s);
}

static void test_linked_list_slab_insert(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestLinkedListSlab list;
  AG_CHECK(TestLinkedListSlab_init(&alloc, &list));

  AG_CHECK(TestLinkedListSlab_push_tail(&list, 1));
  AG_CHECK(TestLinkedListSlab_push_tail(&list, 3));
  // list: 1 -> 3

  TestLinkedListSlabNode* first = list.head;
  AG_CHECK(TestLinkedListSlab_insert_after(&list, first, 2));
  // list: 1 -> 2 -> 3
  AG_CHECK_EQ_INT(list.size, 3);
  AG_CHECK(*list.head->val == 1);
  AG_CHECK(*list.head->next->val == 2);
  AG_CHECK(*list.tail->val == 3);

  TestLinkedListSlabNode* last = list.tail;
  AG_CHECK(TestLinkedListSlab_insert_before(&list, last, 25));
  // list: 1 -> 2 -> 25 -> 3
  AG_CHECK_EQ_INT(list.size, 4);
  AG_CHECK(*list.tail->prev->val == 25);

  // insert_before the head should behave like push_head.
  AG_CHECK(TestLinkedListSlab_insert_before(&list, list.head, 0));
  AG_CHECK_EQ_INT(list.size, 5);
  AG_CHECK(*list.head->val == 0);

  // insert_after the tail should extend the tail.
  AG_CHECK(TestLinkedListSlab_insert_after(&list, list.tail, 99));
  AG_CHECK(*list.tail->val == 99);

  TestLinkedListSlab_free(&list);
  slab_free_all(&s);
}

static void test_linked_list_slab_remove(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestLinkedListSlab list;
  AG_CHECK(TestLinkedListSlab_init(&alloc, &list));

  for (int i = 0; i < 5; i++)
    AG_CHECK(TestLinkedListSlab_push_tail(&list, i));
  // list: 0 1 2 3 4

  TestLinkedListSlabNode* two = list.head->next->next;
  AG_CHECK(*two->val == 2);
  AG_CHECK(TestLinkedListSlab_remove(&list, two));
  AG_CHECK_EQ_INT(list.size, 4);

  int values[4];
  TestLinkedListSlabNode* cur = list.head;
  for (int i = 0; i < 4; i++) { values[i] = *cur->val; cur = cur->next; }
  AG_CHECK(values[0] == 0 && values[1] == 1 && values[2] == 3 && values[3] == 4);

  AG_CHECK(TestLinkedListSlab_remove(&list, list.head));
  AG_CHECK(*list.head->val == 1);

  AG_CHECK(TestLinkedListSlab_remove(&list, list.tail));
  AG_CHECK(*list.tail->val == 3);

  AG_CHECK_EQ_INT(list.size, 2);

  TestLinkedListSlab_free(&list);
  slab_free_all(&s);
}

static void test_linked_list_slab_bounded_reuse(void) {
  // A tiny slab (its real capacity, after page rounding, is still small
  // relative to N below). Never holding more than one node alive at a
  // time, we push+pop N times - far more operations than there are
  // blocks. This only works if freed blocks are genuinely returned to
  // the slab and reused; the same workload would exhaust an arena almost
  // immediately since an arena never reclaims individual allocations.
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);

  TestLinkedListSlab list;
  AG_CHECK(TestLinkedListSlab_init(&alloc, &list));

  const int N = 5000;
  for (int i = 0; i < N; i++) {
    AG_CHECK(TestLinkedListSlab_push_tail(&list, i));
    int v;
    AG_CHECK(TestLinkedListSlab_pop_head(&list, &v));
    AG_CHECK(v == i);
  }
  AG_CHECK(TestLinkedListSlab_is_empty(&list));

  TestLinkedListSlab_free(&list);
  slab_free_all(&s);
}

static void test_linked_list_slab_exhaustion(void) {
  // Every push consumes 2 blocks (value + node), so the slab can hold
  // exactly capacity/2 nodes at once, however large capacity turns out
  // to be after page rounding.
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 4));
  sAllocator alloc = use_slab(&s);
  size_t maxNodes = s.capacity / 2;

  TestLinkedListSlab list;
  AG_CHECK(TestLinkedListSlab_init(&alloc, &list));

  for (size_t i = 0; i < maxNodes; i++)
    AG_CHECK(TestLinkedListSlab_push_tail(&list, (int)i));

  // The slab is exhausted - push must fail cleanly, and the list must be
  // left exactly as it was (no partial node, no corrupted size/links).
  AG_CHECK(!TestLinkedListSlab_push_tail(&list, 999));
  AG_CHECK_EQ_INT(list.size, maxNodes);

  int v;
  AG_CHECK(TestLinkedListSlab_pop_head(&list, &v));
  AG_CHECK_EQ_INT(list.size, maxNodes - 1);

  // Freeing a node frees up exactly enough blocks for one more push.
  AG_CHECK(TestLinkedListSlab_push_tail(&list, 999));
  AG_CHECK_EQ_INT(list.size, maxNodes);

  TestLinkedListSlab_free(&list);
  slab_free_all(&s);
}

static void test_linked_list_slab_invalid_args(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 64));
  sAllocator alloc = use_slab(&s);

  TestLinkedListSlab list;
  AG_CHECK(!TestLinkedListSlab_init(NULL, &list));
  AG_CHECK(!TestLinkedListSlab_init(&alloc, NULL));

  AG_CHECK(TestLinkedListSlab_is_empty(NULL));

  AG_CHECK(TestLinkedListSlab_init(&alloc, &list));
  int v;
  AG_CHECK(!TestLinkedListSlab_peek_head(&list, &v)); // empty list
  AG_CHECK(!TestLinkedListSlab_insert_after(&list, NULL, 1));
  AG_CHECK(!TestLinkedListSlab_remove(&list, NULL));
  AG_CHECK(!TestLinkedListSlab_remove(NULL, NULL));

  TestLinkedListSlab_free(&list);
  slab_free_all(&s);
}

void run_linked_list_slab_tests(void) {
  test_linked_list_slab_init_accepts_slab();
  test_linked_list_slab_basic();
  test_linked_list_slab_insert();
  test_linked_list_slab_remove();
  test_linked_list_slab_bounded_reuse();
  test_linked_list_slab_exhaustion();
  test_linked_list_slab_invalid_args();
}

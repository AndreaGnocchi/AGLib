#include "../include/ag.h"
#include "test_framework.h"

LinkedList(int, TestLinkedList)

static void test_linked_list_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestLinkedList list;
  AG_CHECK(TestLinkedList_init(&alloc, &list));
  AG_CHECK(TestLinkedList_is_empty(&list));

  AG_CHECK(TestLinkedList_push_head(&list, 2));
  AG_CHECK(TestLinkedList_push_head(&list, 1));
  AG_CHECK(TestLinkedList_push_tail(&list, 3));

  int v;
  AG_CHECK(TestLinkedList_peek_head(&list, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestLinkedList_peek_tail(&list, &v)); AG_CHECK(v == 3);
  AG_CHECK_EQ_INT(list.size, 3);

  AG_CHECK(TestLinkedList_pop_head(&list, &v)); AG_CHECK(v == 1);
  AG_CHECK(TestLinkedList_pop_tail(&list, &v)); AG_CHECK(v == 3);
  AG_CHECK_EQ_INT(list.size, 1);

  AG_CHECK(TestLinkedList_pop_head(&list, &v)); AG_CHECK(v == 2);
  AG_CHECK(TestLinkedList_is_empty(&list));
  AG_CHECK(!TestLinkedList_pop_head(&list, &v));
  AG_CHECK(!TestLinkedList_pop_tail(&list, &v));

  TestLinkedList_free(&list);
  arena_free(&a);
}

static void test_linked_list_insert(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestLinkedList list;
  AG_CHECK(TestLinkedList_init(&alloc, &list));

  AG_CHECK(TestLinkedList_push_tail(&list, 1));
  AG_CHECK(TestLinkedList_push_tail(&list, 3));
  // list: 1 -> 3

  TestLinkedListNode* first = list.head;
  AG_CHECK(TestLinkedList_insert_after(&list, first, 2));
  // list: 1 -> 2 -> 3
  AG_CHECK_EQ_INT(list.size, 3);
  AG_CHECK(*list.head->val == 1);
  AG_CHECK(*list.head->next->val == 2);
  AG_CHECK(*list.tail->val == 3);

  TestLinkedListNode* last = list.tail;
  AG_CHECK(TestLinkedList_insert_before(&list, last, 25));
  // list: 1 -> 2 -> 25 -> 3
  AG_CHECK_EQ_INT(list.size, 4);
  AG_CHECK(*list.tail->prev->val == 25);

  // insert_before the head should behave like push_head.
  AG_CHECK(TestLinkedList_insert_before(&list, list.head, 0));
  AG_CHECK_EQ_INT(list.size, 5);
  AG_CHECK(*list.head->val == 0);

  // insert_after the tail should extend the tail.
  AG_CHECK(TestLinkedList_insert_after(&list, list.tail, 99));
  AG_CHECK(*list.tail->val == 99);

  TestLinkedList_free(&list);
  arena_free(&a);
}

static void test_linked_list_remove(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestLinkedList list;
  AG_CHECK(TestLinkedList_init(&alloc, &list));

  for (int i = 0; i < 5; i++)
    AG_CHECK(TestLinkedList_push_tail(&list, i));
  // list: 0 1 2 3 4

  // Remove a middle node.
  TestLinkedListNode* two = list.head->next->next;
  AG_CHECK(*two->val == 2);
  AG_CHECK(TestLinkedList_remove(&list, two));
  AG_CHECK_EQ_INT(list.size, 4);

  int values[4];
  TestLinkedListNode* cur = list.head;
  for (int i = 0; i < 4; i++) { values[i] = *cur->val; cur = cur->next; }
  AG_CHECK(values[0] == 0 && values[1] == 1 && values[2] == 3 && values[3] == 4);

  // Remove head then tail via pointer.
  AG_CHECK(TestLinkedList_remove(&list, list.head));
  AG_CHECK(*list.head->val == 1);

  AG_CHECK(TestLinkedList_remove(&list, list.tail));
  AG_CHECK(*list.tail->val == 3);

  AG_CHECK_EQ_INT(list.size, 2);

  TestLinkedList_free(&list);
  arena_free(&a);
}

static void test_linked_list_invalid_args(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestLinkedList list;
  AG_CHECK(!TestLinkedList_init(NULL, &list));
  AG_CHECK(!TestLinkedList_init(&alloc, NULL));

  AG_CHECK(TestLinkedList_is_empty(NULL));

  AG_CHECK(TestLinkedList_init(&alloc, &list));
  int v;
  AG_CHECK(!TestLinkedList_peek_head(&list, &v)); // empty list
  AG_CHECK(!TestLinkedList_insert_after(&list, NULL, 1));
  AG_CHECK(!TestLinkedList_remove(&list, NULL));
  AG_CHECK(!TestLinkedList_remove(NULL, NULL));

  TestLinkedList_free(&list);
  arena_free(&a);
}

void run_linked_list_tests(void) {
  test_linked_list_basic();
  test_linked_list_insert();
  test_linked_list_remove();
  test_linked_list_invalid_args();
}

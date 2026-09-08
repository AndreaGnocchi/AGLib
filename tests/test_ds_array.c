#include "../include/ag.h"
#include "test_framework.h"

DynamicArray(int, TestIntArray)

static void test_array_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestIntArray arr;
  AG_CHECK(TestIntArray_init(&alloc, &arr, 2));
  AG_CHECK(TestIntArray_is_empty(&arr));

  for (int i = 0; i < 10; i++)
    AG_CHECK(TestIntArray_push(&arr, i));

  AG_CHECK(!TestIntArray_is_empty(&arr));
  AG_CHECK_EQ_INT(arr.size, 10);
  for (int i = 0; i < 10; i++)
    AG_CHECK(arr.items[i] == i);

  TestIntArray_clear(&arr);
  AG_CHECK(TestIntArray_is_empty(&arr));

  TestIntArray_free(&arr);
  arena_free(&a);
}

static void test_array_growth_stress(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(4)));
  sAllocator alloc = use_arena(&a);

  TestIntArray arr;
  // Start tiny so we exercise many capacity-doubling reallocations.
  AG_CHECK(TestIntArray_init(&alloc, &arr, 1));

  const int N = 10000;
  for (int i = 0; i < N; i++)
    AG_CHECK(TestIntArray_push(&arr, i * i));

  AG_CHECK_EQ_INT(arr.size, N);
  AG_CHECK(arr.capacity >= (size_t)N);
  for (int i = 0; i < N; i++)
    AG_CHECK(arr.items[i] == i * i);

  TestIntArray_free(&arr);
  arena_free(&a);
}

static void test_array_pop_basic(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestIntArray arr;
  AG_CHECK(TestIntArray_init(&alloc, &arr, 2));

  // Popping an empty array must fail and must not touch outVal.
  int sentinel = -999;
  AG_CHECK(!TestIntArray_pop(&arr, &sentinel));
  AG_CHECK(sentinel == -999);

  for (int i = 0; i < 10; i++)
    AG_CHECK(TestIntArray_push(&arr, i));
  AG_CHECK_EQ_INT(arr.size, 10);

  size_t capBeforePop = arr.capacity;

  // Pop must return values in LIFO order and never shrink capacity.
  for (int i = 9; i >= 0; i--) {
    int out = -1;
    AG_CHECK(TestIntArray_pop(&arr, &out));
    AG_CHECK(out == i);
    AG_CHECK_EQ_INT(arr.size, (size_t)i);
  }
  AG_CHECK(arr.capacity == capBeforePop);
  AG_CHECK(TestIntArray_is_empty(&arr));

  // Once drained, pop must fail again.
  AG_CHECK(!TestIntArray_pop(&arr, NULL));

  // outVal is optional - a NULL outVal should still remove the item.
  AG_CHECK(TestIntArray_push(&arr, 42));
  AG_CHECK(TestIntArray_pop(&arr, NULL));
  AG_CHECK(TestIntArray_is_empty(&arr));

  TestIntArray_free(&arr);
  arena_free(&a);
}

static void test_array_push_pop_interleaved(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestIntArray arr;
  AG_CHECK(TestIntArray_init(&alloc, &arr, 4));

  // Interleave push/pop like a stack and check against a shadow model.
  // Pops only happen on 1 in 3 iterations, so the worst case (every
  // iteration pushes) is what the shadow buffer must be sized for.
  const int ITERS = 200;
  int shadow[ITERS];
  int shadowSize = 0;

  for (int i = 0; i < ITERS; i++) {
    if (i % 3 != 0 || shadowSize == 0) {
      AG_CHECK(TestIntArray_push(&arr, i));
      shadow[shadowSize++] = i;
    } else {
      int out;
      AG_CHECK(TestIntArray_pop(&arr, &out));
      shadowSize--;
      AG_CHECK(out == shadow[shadowSize]);
    }
    AG_CHECK_EQ_INT(arr.size, (size_t)shadowSize);
  }

  TestIntArray_free(&arr);
  arena_free(&a);
}

static void test_array_reuse_after_clear(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestIntArray arr;
  AG_CHECK(TestIntArray_init(&alloc, &arr, 4));

  for (int i = 0; i < 5; i++) AG_CHECK(TestIntArray_push(&arr, i));
  TestIntArray_clear(&arr);
  AG_CHECK_EQ_INT(arr.size, 0);

  // Push again after clearing - capacity should still be usable.
  for (int i = 0; i < 3; i++) AG_CHECK(TestIntArray_push(&arr, i + 100));
  AG_CHECK_EQ_INT(arr.size, 3);
  AG_CHECK(arr.items[0] == 100);

  TestIntArray_free(&arr);
  arena_free(&a);
}

static void test_array_invalid_args(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  TestIntArray arr;
  AG_CHECK(!TestIntArray_init(NULL, &arr, 4));
  AG_CHECK(!TestIntArray_init(&alloc, NULL, 4));
  AG_CHECK(!TestIntArray_init(&alloc, &arr, 0));

  // A DynamicArray is documented as arena-only; a SLAB allocator must be rejected.
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 8));
  sAllocator slabAlloc = use_slab(&s);
  AG_CHECK(!TestIntArray_init(&slabAlloc, &arr, 4));
  slab_free_all(&s);

  AG_CHECK(TestIntArray_is_empty(NULL));
  AG_CHECK(!TestIntArray_pop(NULL, NULL));

  arena_free(&a);
}

void run_array_tests(void) {
  test_array_basic();
  test_array_growth_stress();
  test_array_pop_basic();
  test_array_push_pop_interleaved();
  test_array_reuse_after_clear();
  test_array_invalid_args();
}
#include "../include/ag.h"
#include "test_framework.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// NOTE: slab_init rounds its backing allocation up to a full OS page, so
// s.capacity is almost always >= the nBlocks requested, never less - never
// assume the two are equal. Tests below always read s.capacity back and
// size any tracking storage off of it dynamically.

static void test_slab_basic(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 8));
  AG_CHECK(s.capacity >= 8);
  AG_CHECK(s.blockSize >= sizeof(int));

  int* p1 = (int*)slab_alloc(&s, true);
  AG_CHECK(p1 != NULL);
  AG_CHECK(*p1 == 0); // zero-initialised

  *p1 = 42;
  int* p2 = (int*)slab_alloc(&s, false);
  AG_CHECK(p2 != NULL);
  AG_CHECK(p1 != p2);

  slab_free_all(&s);
}

static void test_slab_capacity_rounding(void) {
  // A tiny blockSize/nBlocks request still gets rounded up to at least a
  // full page's worth of blocks, and blockSize itself is rounded up to
  // pointer alignment (the free list is stored intrusively in each block).
  sSlab s;
  AG_CHECK(slab_init(&s, 1, 1));
  AG_CHECK(s.blockSize >= sizeof(void*));
  AG_CHECK((s.blockSize % sizeof(void*)) == 0);
  AG_CHECK(s.capacity >= 1);
  AG_CHECK(s.totSize >= s.blockSize);
  AG_CHECK(s.totSize == s.capacity * s.blockSize);

  slab_free_all(&s);
}

static void test_slab_exhaustion(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 4));

  void** ptrs = (void**)malloc(s.capacity * sizeof(void*));
  AG_CHECK(ptrs != NULL);

  for (size_t i = 0; i < s.capacity; i++) {
    ptrs[i] = slab_alloc(&s, false);
    AG_CHECK(ptrs[i] != NULL);
  }

  // The slab is now fully exhausted - one more request must fail cleanly
  // rather than overrun the backing block.
  AG_CHECK(slab_alloc(&s, false) == NULL);

  // Freeing one block makes exactly one allocation available again.
  slab_free(&s, ptrs[0]);
  void* reused = slab_alloc(&s, false);
  AG_CHECK(reused != NULL);
  AG_CHECK(reused == ptrs[0]); // intrusive free list is LIFO

  AG_CHECK(slab_alloc(&s, false) == NULL);

  free(ptrs);
  slab_free_all(&s);
}

static void test_slab_free_and_reuse(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 16));

  void** first = (void**)malloc(s.capacity * sizeof(void*));
  AG_CHECK(first != NULL);

  for (size_t i = 0; i < s.capacity; i++) {
    first[i] = slab_alloc(&s, false);
    AG_CHECK(first[i] != NULL);
  }

  // Free every block, then confirm the slab can satisfy a full new round
  // of allocations - unlike an arena, individual blocks really are
  // reclaimed and reused rather than only bulk-reset.
  for (size_t i = 0; i < s.capacity; i++)
    slab_free(&s, first[i]);

  size_t seenAgain = 0;
  for (size_t i = 0; i < s.capacity; i++) {
    void* p = slab_alloc(&s, false);
    AG_CHECK(p != NULL);
    for (size_t j = 0; j < s.capacity; j++) {
      if (first[j] == p) { seenAgain++; break; }
    }
  }
  AG_CHECK_EQ_INT(seenAgain, s.capacity);

  free(first);
  slab_free_all(&s);
}

static void test_slab_zero_flag_on_reuse(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 2));

  int* p = (int*)slab_alloc(&s, false);
  AG_CHECK(p != NULL);
  *p = 0xBEEF;

  slab_free(&s, p);

  // slab_free uses an intrusive free list - it writes the list pointer
  // into the freed block itself, so a block's prior contents are NOT
  // preserved across a free/alloc round trip (they get overwritten by
  // whatever the free-list pointer's bytes happen to be). A non-zeroing
  // allocation only promises "the memory is yours", never any particular
  // content. What we *can* verify is that the same block is genuinely
  // handed back on reuse, rather than a fresh one.
  int* reused = (int*)slab_alloc(&s, false);
  AG_CHECK(reused == p);

  slab_free(&s, reused);

  int* zeroed = (int*)slab_alloc(&s, true);
  AG_CHECK(zeroed == p);
  AG_CHECK(*zeroed == 0);

  slab_free_all(&s);
}

static void test_slab_reset(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 8));

  size_t capacity = s.capacity;
  for (size_t i = 0; i < capacity; i++)
    AG_CHECK(slab_alloc(&s, false) != NULL);

  AG_CHECK(slab_alloc(&s, false) == NULL); // exhausted

  slab_reset(&s);

  // Every block should be available again, and the backing memory is
  // retained (no new mapping was made).
  size_t count = 0;
  while (slab_alloc(&s, false) != NULL) count++;
  AG_CHECK_EQ_INT(count, capacity);

  slab_free_all(&s);
}

static void test_slab_out_of_bounds_free(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 4));

  size_t capacity = s.capacity;

  int* p = (int*)slab_alloc(&s, false);
  AG_CHECK(p != NULL);

  // A pointer that never came from this slab must be ignored, not
  // corrupt the free list.
  int stackVar = 0;
  slab_free(&s, &stackVar);

  // A pointer inside the slab's backing memory but not aligned to a
  // block boundary must also be ignored.
  unsigned char* misaligned = s.base + 1;
  slab_free(&s, misaligned);

  // The slab should still behave exactly as if neither bad call happened:
  // exactly capacity-1 further allocations should succeed (the one we
  // already hold, `p`, was never returned to the free list).
  size_t remaining = 0;
  while (slab_alloc(&s, false) != NULL) remaining++;
  AG_CHECK_EQ_INT(remaining, capacity - 1);

  slab_free_all(&s);
}

static void test_slab_invalid_args(void) {
  AG_CHECK(slab_init(NULL, sizeof(int), 4) == false);

  sSlab s;
  AG_CHECK(slab_init(&s, 0, 4) == false);
  AG_CHECK(slab_init(&s, sizeof(int), 0) == false);

  AG_CHECK(slab_init(&s, sizeof(int), 4));
  AG_CHECK(slab_alloc(NULL, false) == NULL);

  // NULL-safety on free/reset/free_all.
  slab_free(&s, NULL);
  slab_free(NULL, NULL);
  slab_reset(NULL);

  // Double free_all / free_all-after-free_all should not crash.
  slab_free_all(&s);
  slab_free_all(&s);

  // Operating on a freed slab should be a safe no-op, not a crash.
  slab_reset(&s);
  AG_CHECK(slab_alloc(&s, false) == NULL);
}

// ——— Generic sAllocator interface backed by a slab ————————————————————————

static void test_allocator_slab_alloc_free(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int64_t), 8));
  sAllocator alloc = use_slab(&s);
  AG_CHECK(alloc.type == SLAB);

  int64_t* p1 = (int64_t*)ag_alloc(&alloc, sizeof(int64_t), true);
  AG_CHECK(p1 != NULL);
  AG_CHECK(*p1 == 0);

  *p1 = 123456789;
  ag_free(&alloc, p1);

  int64_t* p2 = (int64_t*)ag_alloc(&alloc, sizeof(int64_t), false);
  AG_CHECK(p2 != NULL);
  AG_CHECK(p2 == p1); // reused the freed block

  slab_free_all(&s);
}

static void test_allocator_slab_rejects_oversized(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 8));
  sAllocator alloc = use_slab(&s);

  // A request larger than blockSize can never be satisfied by a slab -
  // ag_alloc must reject it outright instead of returning a short buffer.
  AG_CHECK(ag_alloc(&alloc, s.blockSize * 4, false) == NULL);

  void* ok = ag_alloc(&alloc, sizeof(int), false);
  AG_CHECK(ok != NULL);

  slab_free_all(&s);
}

static void test_allocator_slab_realloc(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, KB(1), 4));
  sAllocator alloc = use_slab(&s);

  char* buf = (char*)ag_alloc(&alloc, 16, false);
  AG_CHECK(buf != NULL);
  memcpy(buf, "hello", 6);

  // Growing (but still within blockSize) copies the overlapping bytes.
  char* grown = (char*)ag_realloc(&alloc, buf, 16, 64, false);
  AG_CHECK(grown != NULL);
  AG_CHECK(memcmp(grown, "hello", 6) == 0);

  // Growing past blockSize must fail cleanly.
  AG_CHECK(ag_realloc(&alloc, grown, 64, s.blockSize * 8, false) == NULL);

  ag_free(&alloc, grown);
  slab_free_all(&s);
}

static void test_allocator_slab_reset_and_destroy(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(int), 4));
  sAllocator alloc = use_slab(&s);

  size_t capacity = s.capacity;
  for (size_t i = 0; i < capacity; i++)
    AG_CHECK(ag_alloc(&alloc, sizeof(int), false) != NULL);
  AG_CHECK(ag_alloc(&alloc, sizeof(int), false) == NULL); // exhausted

  ag_reset(&alloc);
  size_t count = 0;
  while (ag_alloc(&alloc, sizeof(int), false) != NULL) count++;
  AG_CHECK_EQ_INT(count, capacity);

  ag_destroy(&alloc); // releases the backing mapping, like slab_free_all
  AG_CHECK(s.base == NULL);
}

static void test_allocator_use_slab_invalid_args(void) {
  sAllocator invalid = use_slab(NULL);
  AG_CHECK(invalid.type == INVALID_ALLOCATOR);
  AG_CHECK(ag_alloc(&invalid, sizeof(int), false) == NULL);
}

void run_slab_tests(void) {
  test_slab_basic();
  test_slab_capacity_rounding();
  test_slab_exhaustion();
  test_slab_free_and_reuse();
  test_slab_zero_flag_on_reuse();
  test_slab_reset();
  test_slab_out_of_bounds_free();
  test_slab_invalid_args();

  test_allocator_slab_alloc_free();
  test_allocator_slab_rejects_oversized();
  test_allocator_slab_realloc();
  test_allocator_slab_reset_and_destroy();
  test_allocator_use_slab_invalid_args();
}

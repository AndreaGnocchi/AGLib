#include "../include/ag.h"

#ifdef THREAD_SAFE_AGLIB_DS

#include "test_framework.h"
#include <stdint.h>
#include <stdlib.h>

// ═══════════════════════════════════════════════════════════════════════
// Tiny cross-platform thread wrapper, local to this test file only.
// The library itself never needs this - it's purely test infrastructure
// for exercising the thread-safe data-structure variants concurrently.
// ═══════════════════════════════════════════════════════════════════════

#ifdef _WIN32
  #include <windows.h>
  typedef HANDLE ag_thread_t;
  typedef DWORD (WINAPI *ag_thread_fn)(void*);

  static ag_thread_t ag_thread_start(ag_thread_fn fn, void* arg) {
    return CreateThread(NULL, 0, fn, arg, 0, NULL);
  }
  static void ag_thread_join(ag_thread_t t) {
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
  }
  #define AG_THREAD_RETURN(v) return (DWORD)(v)
  #define AG_THREAD_FN_SIG(name, argname) DWORD WINAPI name(void* argname)
#else
  #include <pthread.h>
  typedef pthread_t ag_thread_t;
  typedef void* (*ag_thread_fn)(void*);

  static ag_thread_t ag_thread_start(ag_thread_fn fn, void* arg) {
    pthread_t t;
    pthread_create(&t, NULL, fn, arg);
    return t;
  }
  static void ag_thread_join(ag_thread_t t) {
    pthread_join(t, NULL);
  }
  #define AG_THREAD_RETURN(v) return (void*)(intptr_t)(v)
  #define AG_THREAD_FN_SIG(name, argname) void* name(void* argname)
#endif

#define N_THREADS       8
#define OPS_PER_THREAD  2000

static uint64_t hash_int(int x) {
  uint64_t h = (uint64_t)x;
  h += 0x9E3779B97F4A7C15ULL;
  h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
  h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
  return h ^ (h >> 31);
}
static bool eq_int(int a, int b) { return a == b; }

Map(int, int, ThreadSafeMap, hash_int, eq_int)
DynamicArray(int, ThreadSafeArray)

typedef struct {
  int id;
  ThreadSafeMap*   map;
  ThreadSafeArray* arr;
} sMapThreadArgs;

static AG_THREAD_FN_SIG(map_worker, argRaw) {
  sMapThreadArgs* args = (sMapThreadArgs*)argRaw;

  for (int i = 0; i < OPS_PER_THREAD; i++) {
    // Each thread writes to a disjoint key range so we can verify every
    // single write landed correctly with no lost updates.
    int key = args->id * OPS_PER_THREAD + i;
    ThreadSafeMap_insert(args->map, key, key * 2);
    ThreadSafeArray_push(args->arr, key);
  }

  AG_THREAD_RETURN(0);
}

static void test_map_concurrent_writes(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(16)));
  sAllocator alloc = use_arena(&a);

  ThreadSafeMap map;
  AG_CHECK(ThreadSafeMap_init(&alloc, &map, 64));

  ThreadSafeArray arr;
  AG_CHECK(ThreadSafeArray_init(&alloc, &arr, 64));

  sMapThreadArgs args[N_THREADS];
  ag_thread_t    threads[N_THREADS];

  for (int i = 0; i < N_THREADS; i++) {
    args[i].id  = i;
    args[i].map = &map;
    args[i].arr = &arr;
    threads[i]  = ag_thread_start(map_worker, &args[i]);
  }

  for (int i = 0; i < N_THREADS; i++)
    ag_thread_join(threads[i]);

  // Every key from every thread must be present with the correct value -
  // if the lock were missing/broken, entries would be dropped or corrupted
  // under concurrent Robin-Hood displacement.
  AG_CHECK_EQ_INT(map.size, N_THREADS * OPS_PER_THREAD);
  AG_CHECK_EQ_INT(arr.size, N_THREADS * OPS_PER_THREAD);

  bool allCorrect = true;
  for (int t = 0; t < N_THREADS; t++) {
    for (int i = 0; i < OPS_PER_THREAD; i++) {
      int key = t * OPS_PER_THREAD + i;
      int out;
      if (!ThreadSafeMap_get(&map, key, &out) || out != key * 2) {
        allCorrect = false;
      }
    }
  }
  AG_CHECK(allCorrect);

  ThreadSafeMap_free(&map);
  ThreadSafeArray_free(&arr);
  arena_free(&a);
}

// ═══════════════════════════════════════════════════════════════════════
// DynamicArray: concurrent pop.
//
// Push is already exercised concurrently above (via map_worker), so this
// focuses on _pop: N threads race to pop from one shared array with no
// coordination between them, so per-thread counts aren't fixed - but the
// union of everything popped must account for every pushed value exactly
// once, with nothing lost, duplicated, or corrupted by the lock.
// ═══════════════════════════════════════════════════════════════════════

typedef struct {
  int id;
  ThreadSafeArray* arr;
} sArrayPushArgs;

static AG_THREAD_FN_SIG(array_push_only_worker, argRaw) {
  sArrayPushArgs* args = (sArrayPushArgs*)argRaw;

  for (int i = 0; i < OPS_PER_THREAD; i++) {
    int val = args->id * OPS_PER_THREAD + i;
    ThreadSafeArray_push(args->arr, val);
  }

  AG_THREAD_RETURN(0);
}

typedef struct {
  ThreadSafeArray* arr;
  int* buf;      // upper-bounded scratch buffer for this thread's pops
  int  popped;
} sArrayPopArgs;

static AG_THREAD_FN_SIG(array_pop_worker, argRaw) {
  sArrayPopArgs* args = (sArrayPopArgs*)argRaw;

  int val;
  while (ThreadSafeArray_pop(args->arr, &val))
    args->buf[args->popped++] = val;

  AG_THREAD_RETURN(0);
}

static void test_array_concurrent_push_then_pop(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(16)));
  sAllocator alloc = use_arena(&a);

  ThreadSafeArray arr;
  AG_CHECK(ThreadSafeArray_init(&alloc, &arr, 64));

  const int TOTAL = N_THREADS * OPS_PER_THREAD;

  // Phase 1 - concurrent pushes, disjoint value ranges per thread.
  sArrayPushArgs pushArgs[N_THREADS];
  ag_thread_t    threads[N_THREADS];

  for (int i = 0; i < N_THREADS; i++) {
    pushArgs[i].id  = i;
    pushArgs[i].arr = &arr;
    threads[i]      = ag_thread_start(array_push_only_worker, &pushArgs[i]);
  }
  for (int i = 0; i < N_THREADS; i++)
    ag_thread_join(threads[i]);

  AG_CHECK_EQ_INT(arr.size, (size_t)TOTAL);

  // Phase 2 - every thread pops until the array reports empty. Threads
  // race for whichever value is currently on top, so give each one a
  // TOTAL-sized scratch buffer since any single thread could in theory
  // drain the whole array.
  int*          buf[N_THREADS];
  sArrayPopArgs popArgs[N_THREADS];

  for (int i = 0; i < N_THREADS; i++) {
    buf[i] = (int*)calloc((size_t)TOTAL, sizeof(int));
    AG_CHECK(buf[i] != NULL);

    popArgs[i].arr    = &arr;
    popArgs[i].buf    = buf[i];
    popArgs[i].popped = 0;
    threads[i]        = ag_thread_start(array_pop_worker, &popArgs[i]);
  }
  for (int i = 0; i < N_THREADS; i++)
    ag_thread_join(threads[i]);

  AG_CHECK(ThreadSafeArray_is_empty(&arr));
  AG_CHECK_EQ_INT(arr.size, 0);

  bool* seen = (bool*)calloc((size_t)TOTAL, sizeof(bool));
  AG_CHECK(seen != NULL);

  int totalPopped = 0;
  for (int i = 0; i < N_THREADS; i++) {
    for (int j = 0; j < popArgs[i].popped; j++) {
      int v = buf[i][j];
      AG_CHECK(v >= 0 && v < TOTAL);
      AG_CHECK(!seen[v]);
      seen[v] = true;
      totalPopped++;
    }
    free(buf[i]);
  }
  AG_CHECK_EQ_INT(totalPopped, TOTAL);

  bool allPresent = true;
  for (int i = 0; i < TOTAL; i++)
    if (!seen[i]) allPresent = false;
  AG_CHECK(allPresent);

  free(seen);
  ThreadSafeArray_free(&arr);
  arena_free(&a);
}

typedef struct {
  ThreadSafeMap* map;
  int start, end;
  int successCount;
} sDeleteArgs;

static AG_THREAD_FN_SIG(delete_worker, argRaw) {
  sDeleteArgs* args = (sDeleteArgs*)argRaw;
  int count = 0;
  for (int k = args->start; k < args->end; k++)
    if (ThreadSafeMap_delete(args->map, k)) count++;
  args->successCount = count;
  AG_THREAD_RETURN(0);
}

static void test_map_concurrent_delete(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(16)));
  sAllocator alloc = use_arena(&a);

  ThreadSafeMap map;
  AG_CHECK(ThreadSafeMap_init(&alloc, &map, 64));

  const int TOTAL = N_THREADS * OPS_PER_THREAD;
  for (int i = 0; i < TOTAL; i++)
    ThreadSafeMap_insert(&map, i, i);

  sDeleteArgs args[N_THREADS];
  ag_thread_t threads[N_THREADS];
  int chunk = TOTAL / N_THREADS;

  for (int i = 0; i < N_THREADS; i++) {
    args[i].map          = &map;
    args[i].start        = i * chunk;
    args[i].end          = (i == N_THREADS - 1) ? TOTAL : (i + 1) * chunk;
    args[i].successCount = 0;
    threads[i]            = ag_thread_start(delete_worker, &args[i]);
  }

  int totalDeleted = 0;
  for (int i = 0; i < N_THREADS; i++) {
    ag_thread_join(threads[i]);
    totalDeleted += args[i].successCount;
  }

  // Disjoint ranges -> every delete should succeed exactly once, and the
  // map should end up empty with no double-counted or lost deletions.
  AG_CHECK_EQ_INT(totalDeleted, TOTAL);
  AG_CHECK_EQ_INT(map.size, 0);
  AG_CHECK(ThreadSafeMap_is_empty(&map));

  ThreadSafeMap_free(&map);
  arena_free(&a);
}

// ═══════════════════════════════════════════════════════════════════════
// Slab-backed containers under concurrency.
//
// LinkedList (and Stack/Queue, built on it) and TreeMap (and TreeSet)
// now accept a slab-backed sAllocator. A thread-safe container takes its
// own lock around every operation that touches the allocator, so a
// single container instance sharing one slab across threads is safe -
// the container's lock is what serializes the slab_alloc/slab_free
// calls underneath, exactly as documented for the arena case. These
// tests are the slab equivalent of the Map/DynamicArray tests above.
// ═══════════════════════════════════════════════════════════════════════

Stack(int, ThreadSafeStackSlab)

typedef struct {
  int id;
  ThreadSafeStackSlab* stack;
  int successCount;
} sStackThreadArgs;

static AG_THREAD_FN_SIG(stack_push_worker, argRaw) {
  sStackThreadArgs* args = (sStackThreadArgs*)argRaw;

  int count = 0;
  for (int i = 0; i < OPS_PER_THREAD; i++) {
    // Disjoint value range per thread, same idea as the map test - lets
    // us verify every push's value survived, not just the final count.
    int val = args->id * OPS_PER_THREAD + i;
    if (ThreadSafeStackSlab_push(args->stack, val)) count++;
  }
  args->successCount = count;

  AG_THREAD_RETURN(0);
}

static void test_stack_slab_concurrent_push(void) {
  const int TOTAL = N_THREADS * OPS_PER_THREAD;

  // Every push needs 2 blocks (value copy + node), so size the slab
  // generously above TOTAL * 2 to rule out spurious exhaustion.
  sSlab s;
  AG_CHECK(slab_init(&s, 3 * sizeof(void*), (size_t)TOTAL * 2 + 64));
  sAllocator alloc = use_slab(&s);

  ThreadSafeStackSlab stack;
  AG_CHECK(ThreadSafeStackSlab_init(&alloc, &stack));

  sStackThreadArgs args[N_THREADS];
  ag_thread_t      threads[N_THREADS];

  for (int i = 0; i < N_THREADS; i++) {
    args[i].id           = i;
    args[i].stack         = &stack;
    args[i].successCount = 0;
    threads[i]            = ag_thread_start(stack_push_worker, &args[i]);
  }

  int totalPushed = 0;
  for (int i = 0; i < N_THREADS; i++) {
    ag_thread_join(threads[i]);
    totalPushed += args[i].successCount;
  }

  AG_CHECK_EQ_INT(totalPushed, TOTAL);
  AG_CHECK_EQ_INT(stack.size, TOTAL);

  // Drain the stack and confirm every value from every thread is present
  // exactly once - if the lock failed to serialize slab_alloc/slab_free,
  // we'd expect corrupted nodes, lost values, or duplicated blocks.
  bool* seen = (bool*)calloc((size_t)TOTAL, sizeof(bool));
  AG_CHECK(seen != NULL);

  int v;
  int popped = 0;
  while (ThreadSafeStackSlab_pop(&stack, &v)) {
    AG_CHECK(v >= 0 && v < TOTAL);
    AG_CHECK(!seen[v]);
    seen[v] = true;
    popped++;
  }
  AG_CHECK_EQ_INT(popped, TOTAL);

  bool allPresent = true;
  for (int i = 0; i < TOTAL; i++)
    if (!seen[i]) allPresent = false;
  AG_CHECK(allPresent);

  free(seen);
  ThreadSafeStackSlab_free(&stack);
  slab_free_all(&s);
}

static int cmp_int_thread_safe(int a, int b) { return a - b; }

TreeMap(int, int, ThreadSafeTreeMapSlab, cmp_int_thread_safe)

typedef struct {
  int id;
  ThreadSafeTreeMapSlab* tree;
} sTreeThreadArgs;

static AG_THREAD_FN_SIG(tree_insert_worker, argRaw) {
  sTreeThreadArgs* args = (sTreeThreadArgs*)argRaw;

  for (int i = 0; i < OPS_PER_THREAD; i++) {
    int key = args->id * OPS_PER_THREAD + i;
    ThreadSafeTreeMapSlab_insert(args->tree, key, key * 2);
  }

  AG_THREAD_RETURN(0);
}

static void test_tree_map_slab_concurrent_writes(void) {
  const int TOTAL = N_THREADS * OPS_PER_THREAD;

  // One block per insert, plus one for the sentinel `nil` node.
  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(ThreadSafeTreeMapSlabNode), (size_t)TOTAL + 64));
  sAllocator alloc = use_slab(&s);

  ThreadSafeTreeMapSlab tree;
  AG_CHECK(ThreadSafeTreeMapSlab_init(&alloc, &tree));

  sTreeThreadArgs args[N_THREADS];
  ag_thread_t     threads[N_THREADS];

  for (int i = 0; i < N_THREADS; i++) {
    args[i].id   = i;
    args[i].tree = &tree;
    threads[i]   = ag_thread_start(tree_insert_worker, &args[i]);
  }

  for (int i = 0; i < N_THREADS; i++)
    ag_thread_join(threads[i]);

  AG_CHECK_EQ_INT(tree.size, TOTAL);

  bool allCorrect = true;
  for (int t = 0; t < N_THREADS; t++) {
    for (int i = 0; i < OPS_PER_THREAD; i++) {
      int key = t * OPS_PER_THREAD + i;
      int out;
      if (!ThreadSafeTreeMapSlab_find(&tree, key, &out) || out != key * 2)
        allCorrect = false;
    }
  }
  AG_CHECK(allCorrect);

  ThreadSafeTreeMapSlab_free(&tree);
  slab_free_all(&s);
}

typedef struct {
  ThreadSafeTreeMapSlab* tree;
  int start, end;
  int successCount;
} sTreeDeleteArgs;

static AG_THREAD_FN_SIG(tree_delete_worker, argRaw) {
  sTreeDeleteArgs* args = (sTreeDeleteArgs*)argRaw;
  int count = 0;
  for (int k = args->start; k < args->end; k++)
    if (ThreadSafeTreeMapSlab_remove(args->tree, k)) count++;
  args->successCount = count;
  AG_THREAD_RETURN(0);
}

static void test_tree_map_slab_concurrent_delete(void) {
  const int TOTAL = N_THREADS * OPS_PER_THREAD;

  sSlab s;
  AG_CHECK(slab_init(&s, sizeof(ThreadSafeTreeMapSlabNode), (size_t)TOTAL + 64));
  sAllocator alloc = use_slab(&s);

  ThreadSafeTreeMapSlab tree;
  AG_CHECK(ThreadSafeTreeMapSlab_init(&alloc, &tree));

  for (int i = 0; i < TOTAL; i++)
    ThreadSafeTreeMapSlab_insert(&tree, i, i);

  sTreeDeleteArgs args[N_THREADS];
  ag_thread_t     threads[N_THREADS];
  int chunk = TOTAL / N_THREADS;

  for (int i = 0; i < N_THREADS; i++) {
    args[i].tree         = &tree;
    args[i].start        = i * chunk;
    args[i].end          = (i == N_THREADS - 1) ? TOTAL : (i + 1) * chunk;
    args[i].successCount = 0;
    threads[i]            = ag_thread_start(tree_delete_worker, &args[i]);
  }

  int totalDeleted = 0;
  for (int i = 0; i < N_THREADS; i++) {
    ag_thread_join(threads[i]);
    totalDeleted += args[i].successCount;
  }

  // Disjoint ranges -> every delete should succeed exactly once. The
  // freed node blocks should also be fully reusable afterwards.
  AG_CHECK_EQ_INT(totalDeleted, TOTAL);
  AG_CHECK_EQ_INT(tree.size, 0);
  AG_CHECK(ThreadSafeTreeMapSlab_is_empty(&tree));

  for (int i = 0; i < TOTAL; i++)
    AG_CHECK(ThreadSafeTreeMapSlab_insert(&tree, i, i * 5));
  AG_CHECK_EQ_INT(tree.size, TOTAL);

  ThreadSafeTreeMapSlab_free(&tree);
  slab_free_all(&s);
}

void run_thread_safety_tests(void) {
  test_map_concurrent_writes();
  test_map_concurrent_delete();
  test_array_concurrent_push_then_pop();

  test_stack_slab_concurrent_push();
  test_tree_map_slab_concurrent_writes();
  test_tree_map_slab_concurrent_delete();
}

#endif // THREAD_SAFE_AGLIB_DS
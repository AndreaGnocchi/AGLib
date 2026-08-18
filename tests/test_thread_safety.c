#include "../include/ag.h"

#ifdef THREAD_SAFE_AGLIB_DS

#include "test_framework.h"
#include <stdint.h>

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

void run_thread_safety_tests(void) {
  test_map_concurrent_writes();
  test_map_concurrent_delete();
}

#endif // THREAD_SAFE_AGLIB_DS

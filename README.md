# aglib

A lightweight, cross-platform C utility library providing arena- and slab-based memory management (behind a single generic allocator interface), generic data structures, sorting algorithms, and I/O utilities.

> **Platform support:** Linux, macOS, and Windows. The `Makefile` can build and test all three from any single host (via cross-compilation), and CI runs the full suite on all three.

---

## Modules

- [Allocators](#allocators)
  - [Arena](#arena)
  - [Slab](#slab)
  - [Generic Allocator Interface](#generic-allocator-interface)
- [Data Structures](#data-structures)
  - [DynamicArray](#dynamicarray)
  - [String](#string)
  - [LinkedList](#linkedlist)
  - [Stack](#stack)
  - [Queue](#queue)
  - [Heap](#heap)
  - [Map](#map)
  - [Set](#set)
  - [TreeMap](#treemap)
  - [TreeSet](#treeset)
  - [Thread-safe variants](#thread-safe-variants)
- [Algorithms](#algorithms)
- [I/O](#io)
- [Helpers](#helpers)
- [Building](#building)
- [Testing](#testing)
- [Credits](#credits)

---

## Allocators

`include/aglib_allocator.h`

Every data structure, algorithm, and I/O function that needs to allocate memory does so through a generic `sAllocator` handle rather than a concrete allocator type. This lets the same generic containers run on top of an arena, a fixed-size slab, or the standard `malloc`/`free` family, without any code changes at the call site.

### Arena

`include/aglib_arena.h`

A linear allocator backed by an OS-level memory mapping (`mmap` on Linux/macOS, `VirtualAlloc` on Windows). Allocations are O(1) and freeing is done all at once by resetting or releasing the arena. Temporary sub-arenas allow scoped allocation within a larger arena.

```c
bool       arena_init         (sArena* a, size_t initSize);
void*      arena_alloc        (sArena* a, size_t size, bool zero);
void*      arena_alloc_aligned(sArena* a, size_t size, bool zero, size_t alignment);
void       arena_reset        (sArena* a);
void       arena_free         (sArena* a);
sTempArena arena_temp_start   (sArena* a);
void       arena_temp_end     (sTempArena temp);
```

| Function | Description |
|---|---|
| `arena_init` | Initialises the arena, allocating `initSize` bytes (rounded up to the nearest page). Returns `false` on failure. |
| `arena_alloc` | Allocates `size` bytes from the arena. If `zero` is `true` the memory is zero-initialised. Returns `NULL` on failure. |
| `arena_alloc_aligned` | Same as `arena_alloc` but with an explicit power-of-two `alignment`. |
| `arena_reset` | Resets the offset to zero, logically freeing all allocations. The underlying memory is retained. |
| `arena_free` | Releases the underlying OS-level mapping and zeroes the arena struct. |
| `arena_temp_start` | Saves the current arena offset and returns a `sTempArena` checkpoint. |
| `arena_temp_end` | Restores the arena to the offset saved by `arena_temp_start`, freeing all allocations made since. |

An arena never frees individual allocations — `ag_free` on an arena-backed `sAllocator` is a no-op. Reclaim memory by resetting or freeing the whole arena.

---

### Slab

`include/aglib_slab.h`

A fixed-size block allocator: one up-front allocation is carved into `nBlocks` blocks of `blockSize` bytes each, tracked with an intrusive free list. Individual blocks can be freed and reused, unlike an arena.

```c
bool  slab_init    (sSlab* s, size_t blockSize, size_t nBlocks);
void* slab_alloc   (sSlab* s, bool   zero);
void  slab_free    (sSlab* s, void*  ptr);
void  slab_reset   (sSlab* s);
void  slab_free_all(sSlab* s);
```

| Function | Description |
|---|---|
| `slab_init` | Allocates one backing block sized `blockSize * nBlocks` and initialises the free list. Returns `false` on failure. |
| `slab_alloc` | Pops a free block off the list. If `zero` is `true` the block is zero-initialised. Returns `NULL` if the slab is exhausted. |
| `slab_free` | Returns a block to the free list. |
| `slab_reset` | Rebuilds the free list so every block is available again, without releasing the backing memory. |
| `slab_free_all` | Releases the backing memory and zeroes the slab struct. |

A slab can only satisfy requests up to `blockSize` bytes, so it cannot back the growable containers below (`DynamicArray`, `LinkedList`, `Map`, `TreeMap`, and anything built on them) — their `_init` functions reject a slab-backed allocator outright. Use a slab directly for fixed-size, frequently allocated/freed objects, or via `ag_alloc`/`ag_free` for single fixed-size values.

---

### Generic Allocator Interface

`include/aglib_allocator.h`

`sAllocator` is a small tagged union wrapping an `sArena`, an `sSlab`, or the standard library allocator. It is what every data structure, `aglib_algo` function, and `aglib_io` function actually takes as its allocator argument.

```c
typedef enum {
  INVALID_ALLOCATOR = 0,
  ARENA,
  SLAB,
  STD
} eAllocatorType;

typedef struct {
  eAllocatorType type;
  union {
    sArena* arena;
    sSlab*  slab;
  };
} sAllocator;

sAllocator use_arena(sArena* a);
sAllocator use_slab (sSlab*  s);
sAllocator use_std  (void      );

void* ag_alloc  (sAllocator* allocator, size_t size, bool zero);
void  ag_free   (sAllocator* allocator, void*  ptr);
void* ag_realloc(sAllocator* allocator, void*  oldPtr, size_t oldSize, size_t newSize, bool zero);
void  ag_reset  (sAllocator* allocator);
void  ag_destroy(sAllocator* allocator);
```

| Function | Description |
|---|---|
| `use_arena` / `use_slab` / `use_std` | Wrap an existing `sArena*` / `sSlab*` / the standard allocator into an `sAllocator` value. |
| `ag_alloc` | Allocates `size` bytes through the wrapped allocator. Returns `NULL` on failure (including a slab request larger than its block size). |
| `ag_free` | Frees a single allocation. A no-op for `ARENA` (arenas are freed in bulk). |
| `ag_realloc` | Grows or shrinks an allocation, copying the overlapping bytes. For `STD` this delegates to `realloc`; for `ARENA`/`SLAB` it allocates fresh and copies. |
| `ag_reset` | Resets the wrapped allocator (`arena_reset` / `slab_reset`), retaining the backing memory. No-op for `STD`. |
| `ag_destroy` | Releases the wrapped allocator's backing memory entirely (`arena_free` / `slab_free_all`). No-op for `STD`. |

**Example**
```c
sArena a;
arena_init(&a, MB(4));
sAllocator alloc = use_arena(&a);

char* buf = ag_alloc(&alloc, 256, false);

// Every container below takes an sAllocator*, so the same code works
// unchanged if `alloc` instead wraps a slab (use_slab) or malloc (use_std).

ag_destroy(&alloc);   // releases the arena
```

---

## Data Structures

`include/aglib_ds.h`

All data structures are defined via macros that generate type-safe structs and functions for a given element type. All allocations go through a caller-provided `sAllocator`.

`aglib_ds.h` is an umbrella header — it just pulls in one file per container from `include/aglib_ds/` (or, if `THREAD_SAFE_AGLIB_DS` is defined, from `include/aglib_ds_safe_posix/` or `include/aglib_ds_safe_win/` depending on platform — see [Thread-safe variants](#thread-safe-variants)). Include it to get everything, or include an individual `aglib_ds/<container>.h` file if you only need one:

```
include/aglib_ds.h
include/aglib_ds/
├── array.h        (DynamicArray)
├── string.h       (String        — built on array.h)
├── linked_list.h  (LinkedList)
├── stack.h        (Stack         — built on linked_list.h)
├── queue.h        (Queue         — built on linked_list.h)
├── heap.h         (Heap          — built on array.h)
├── map.h          (Map)
├── set.h          (Set           — built on map.h)
├── tree_map.h     (TreeMap)
└── tree_set.h     (TreeSet       — built on tree_map.h)
```

Every container also has a matching `name_free(name*)` function that releases everything it owns through its allocator (an arena-backed container will find this a no-op per-allocation, but it still resets the struct — call `ag_destroy`/`arena_free` on the underlying allocator to actually reclaim the memory).

---

### DynamicArray

```c
DynamicArray(T, name)
```

Generates a growable array of type `T` named `name`. When the array is full, a new backing block of double the capacity is allocated through the allocator and the existing items are copied over.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* arr, size_t initCap)` | Initialises the array with an initial capacity. Fails if `a` wraps a slab. |
| `name_push(name* arr, T item)` | Appends an item, doubling capacity if needed. Returns `false` on allocation failure. |
| `name_is_empty(name* arr)` | Returns `true` if the array has no items. |
| `name_clear(name* arr)` | Resets size to zero without releasing the backing memory. |
| `name_free(name* arr)` | Frees the backing memory through the allocator and zeroes the struct. |

**Example**
```c
DynamicArray(int, IntArray)

sArena a;
arena_init(&a, KB(16));
sAllocator alloc = use_arena(&a);

IntArray arr;
IntArray_init(&alloc, &arr, 8);
IntArray_push(&arr, 42);
IntArray_push(&arr, 7);

for (size_t i = 0; i < arr.size; i++)
    printf("%d\n", arr.items[i]);

IntArray_free(&arr);
arena_free(&a);
```

---

### String

```c
String(name)
```

Generates a growable string type named `name`. Internally it is a `DynamicArray(char, ...)` that always maintains a null terminator; there is no separate slice/view type — index into `str.items` directly, or use `name_cstr_copy` to get an independent copy.

A search callback type is generated alongside each string type:

```c
typedef bool (*nameSearchCallback)(size_t idx, void* userdata);
```

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* str, const char* init)` | Initialises the string by copying `init`. Reserves at least `AG_SHORTEST_STRING` bytes (32 by default, overridable by defining the macro before including) so short strings don't immediately reallocate on the first append. |
| `name_append(name* str, const char* toAppend)` | Appends a C string, growing the buffer if needed. |
| `name_appendf(name* str, const char* fmt, ...)` | Appends a formatted result (like `sprintf`) to the existing content, growing the buffer if needed. |
| `name_to_lower(name* str)` / `name_to_upper(name* str)` | Lower/upper-cases ASCII letters in place. Leaves everything else untouched. |
| `name_normalize(name* str)` | In place: trims leading whitespace, collapses any run of whitespace to a single space, and applies sentence case — the first letter after the start or after `.`, `!`, or `?` is capitalised, every other letter is lower-cased. |
| `name_search_first(name* str, const char* needle, size_t* outIdx)` | Finds the first occurrence of `needle`. Returns `false` if not found. |
| `name_search_first_from(name* str, const char* needle, size_t startFrom, size_t* outIdx)` | Same as `name_search_first`, but starts searching at byte offset `startFrom`. |
| `name_search_last(name* str, const char* needle, size_t* outIdx)` | Finds the last occurrence of `needle`. |
| `name_search_all(name* str, const char* needle, nameSearchCallback cb, void* userdata)` | Calls `cb(idx, userdata)` for every non-overlapping occurrence of `needle`, left to right. The match cursor advances past each hit, so e.g. searching `"aa"` in `"aaaa"` reports 2 matches, not 3. |
| `name_replace_first(name* str, const char* needle, const char* rep)` / `name_replace_last(...)` | Replaces the first/last occurrence of `needle` with `rep`, growing or shrinking the buffer as needed. Returns `false` if `needle` isn't found. |
| `name_replace_all(name* str, const char* needle, const char* rep)` | Replaces every non-overlapping occurrence of `needle` with `rep`. Returns the number of replacements made. |
| `name_cstr_copy(name* str, char* buf, size_t bufSize)` | Copies the null-terminated contents into a caller-owned buffer, truncating (but always null-terminating) if `bufSize` is too small. Returns `false` on a `NULL`/zero-size buffer. |
| `name_clear(name* str)` | Resets size to zero without releasing the backing memory. |
| `name_free(name* str)` | Frees the backing memory through the allocator and zeroes the struct. |

**Example**
```c
String(MyStr)

sArena a;
arena_init(&a, KB(4));
sAllocator alloc = use_arena(&a);

MyStr s;
MyStr_init(&alloc, &s, "hello");
MyStr_append(&s, ", world");
MyStr_appendf(&s, " (%d)", 42);
printf("%s\n", s.items);  // hello, world (42)

MyStr_init(&alloc, &s, "  hello  world. this is a TEST!");
MyStr_normalize(&s);
printf("%s\n", s.items);  // Hello world. This is a test!

size_t idx;
if (MyStr_search_first(&s, "test", &idx))
    printf("found at %zu\n", idx);

MyStr_replace_all(&s, "l", "L");

char buf[64];
MyStr_cstr_copy(&s, buf, sizeof(buf));

MyStr_free(&s);
arena_free(&a);
```

---

### LinkedList

```c
LinkedList(T, name)
```

Generates a doubly linked list of type `T` named `name`. Each node (`nameNode`) stores a pointer to an allocator-owned copy of the value, plus `next` and `prev` pointers.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* list)` | Initialises an empty list. Fails if `a` wraps a slab. |
| `name_push_head / push_tail` | Prepends or appends a value. |
| `name_pop_head / pop_tail` | Removes and optionally returns the head or tail value. |
| `name_peek_head / peek_tail` | Reads the head or tail value without removing it. |
| `name_insert_after(name* list, nameNode* node, T val)` | Inserts a value immediately after `node`. |
| `name_insert_before(name* list, nameNode* node, T val)` | Inserts a value immediately before `node`. |
| `name_remove(name* list, nameNode* node)` | Unlinks and frees a node from the list. |
| `name_is_empty(name* list)` | Returns `true` if the list has no nodes. |
| `name_clear(name* list)` | Frees every node but keeps the list usable, resetting head, tail, and size. |
| `name_free(name* list)` | Equivalent to `name_clear` and additionally detaches the list from its allocator. |

---

### Stack

```c
Stack(T, name)
```

LIFO stack built on top of `LinkedList`. Push and pop operate on the head.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* s)` | Initialises an empty stack. |
| `name_push(name* s, T val)` | Pushes a value onto the top. |
| `name_pop(name* s, T* outVal)` | Pops the top value into `outVal`. Returns `false` if empty. |
| `name_peek(name* s, T* outVal)` | Reads the top value without removing it. |
| `name_is_empty / clear / free` | Standard empty check, reset, and teardown. |

---

### Queue

```c
Queue(T, name)
```

FIFO queue built on top of `LinkedList`. Enqueue appends to the tail, dequeue removes from the head.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* q)` | Initialises an empty queue. |
| `name_enqueue(name* q, T val)` | Adds a value to the back. |
| `name_dequeue(name* q, T* outVal)` | Removes the front value into `outVal`. Returns `false` if empty. |
| `name_peek / is_empty / clear / free` | Standard operations. |

---

### Heap

```c
Heap(T, name, cmp)
```

A binary heap (priority queue) backed by a `DynamicArray`. `cmp(a, b)` must return `true` when `a` should be closer to the top than `b`.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* h, size_t initCap)` | Initialises the heap. |
| `name_push(name* h, T val)` | Inserts a value and sifts up. |
| `name_pop(name* h, T* outVal)` | Removes the top value and sifts down. Returns `false` if empty. |
| `name_peek(name* h, T* outVal)` | Reads the top value without removing it. |
| `name_is_empty / clear / free` | Standard operations. |

**Example — min-heap of ints**
```c
Heap(int, MinHeap, ({ bool _cmp(int a, int b) { return a < b; } _cmp; }))

// Simpler with a named comparator:
static bool int_lt(int a, int b) { return a < b; }
Heap(int, MinHeap, int_lt)

sArena a;
arena_init(&a, KB(4));
sAllocator alloc = use_arena(&a);

MinHeap h;
MinHeap_init(&alloc, &h, 16);
MinHeap_push(&h, 5);
MinHeap_push(&h, 1);
MinHeap_push(&h, 3);

int top;
MinHeap_pop(&h, &top);  // top == 1

MinHeap_free(&h);
arena_free(&a);
```

---

### Map

```c
Map(Tk, Tv, name, hash_fn, eq_fn)
```

A hash map from keys of type `Tk` to values of type `Tv` using Robin Hood open addressing. `hash_fn(Tk) -> size_t` and `eq_fn(Tk, Tk) -> bool` must be provided.

`_insert` automatically resizes the backing block at 75% load factor by calling `_resize` internally, so manual resizing is rarely needed.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* hm, size_t initCap)` | Initialises the map with an initial capacity. Fails if `a` wraps a slab. |
| `name_insert(name* hm, Tk key, Tv val)` | Inserts or updates a key-value pair. Auto-resizes at 75% load. Returns `false` on allocation failure. |
| `name_get(name* hm, Tk key, Tv* outVal)` | Looks up a key. Writes the value to `outVal` if found. Returns `false` if not found. |
| `name_delete(name* hm, Tk key)` | Removes a key using Robin Hood backward shift. Returns `false` if the key is not present. |
| `name_resize(name* hm, size_t newCap)` | Allocates a new backing block of `newCap` and re-inserts all active entries, freeing the old block through the allocator. `newCap` must be greater than the current size. Returns `false` on allocation failure. |
| `name_is_empty / clear / free` | Standard operations. |

**Example**
```c
static size_t hash_str(const char* s) {
    size_t h = 5381;
    while (*s) h = h * 33 ^ (unsigned char)*s++;
    return h;
}
static bool eq_str(const char* a, const char* b) { return strcmp(a, b) == 0; }

Map(const char*, int, WordCount, hash_str, eq_str)

sArena a;
arena_init(&a, KB(64));
sAllocator alloc = use_arena(&a);

WordCount wc;
WordCount_init(&alloc, &wc, 64);

int count = 0;
if (!WordCount_get(&wc, "hello", &count)) count = 0;
WordCount_insert(&wc, "hello", count + 1);

WordCount_free(&wc);
arena_free(&a);
```

---

### Set

```c
Set(T, name, hash_fn, eq_fn)
```

A hash set built on top of `Map` with `bool` as the value type.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* s, size_t initCap)` | Initialises the set. |
| `name_insert(name* s, T key)` | Adds a key to the set. Auto-resizes at 75% load. |
| `name_contains(name* s, T key)` | Returns `true` if the key is present. |
| `name_remove(name* s, T key)` | Removes a key. Returns `false` if not present. |
| `name_resize(name* s, size_t newCap)` | Delegates to the underlying `Map` resize. |
| `name_is_empty / clear / free` | Standard operations. |

---

### TreeMap

```c
TreeMap(Tk, Tv, name, cmp_fn)
```

An ordered map from keys of type `Tk` to values of type `Tv`, implemented as a red-black tree. `cmp_fn(Tk, Tk) -> int` must behave like a standard comparator (negative, zero, or positive). Unlike `Map`, keys are kept in sorted order, so `_min` / `_max` and in-order traversal are supported.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* t)` | Initialises an empty tree. Fails if `a` wraps a slab. |
| `name_insert(name* t, Tk key, Tv val)` | Inserts a new key-value pair. Returns `false` if the key already exists. |
| `name_find(name* t, Tk key, Tv* out)` | Looks up a key. Writes the value to `out` if found. Returns `false` if not found. |
| `name_remove(name* t, Tk key)` | Removes a key, rebalancing the tree. Returns `false` if the key is not present. |
| `name_min(name* t, Tk* outKey, Tv* outVal)` | Writes the smallest key/value pair. Returns `false` if the tree is empty. |
| `name_max(name* t, Tk* outKey, Tv* outVal)` | Writes the largest key/value pair. Returns `false` if the tree is empty. |
| `name_is_empty(name* t)` | Returns `true` if the tree has no nodes. |
| `name_clear(name* t)` | Frees every node but keeps the tree usable, resetting it to empty. |
| `name_free(name* t)` | Equivalent to `name_clear` and additionally detaches the tree from its allocator. |

**Example**
```c
static int cmp_int(int a, int b) { return a - b; }

TreeMap(int, const char*, IntTree, cmp_int)

sArena a;
arena_init(&a, KB(16));
sAllocator alloc = use_arena(&a);

IntTree t;
IntTree_init(&alloc, &t);
IntTree_insert(&t, 5, "five");
IntTree_insert(&t, 1, "one");
IntTree_insert(&t, 9, "nine");

int minKey; const char* minVal;
IntTree_min(&t, &minKey, &minVal);  // minKey == 1, minVal == "one"

const char* v;
if (IntTree_find(&t, 5, &v))
    printf("%s\n", v);  // five

IntTree_free(&t);
arena_free(&a);
```

---

### TreeSet

```c
TreeSet(T, name, cmp_fn)
```

An ordered set built on top of `TreeMap` with `bool` as the value type. Like `TreeMap`, keys are kept in sorted order.

| Function | Description |
|---|---|
| `name_init(sAllocator* a, name* s)` | Initialises an empty set. |
| `name_insert(name* s, T key)` | Adds a key to the set. Returns `false` if already present. |
| `name_contains(name* s, T key)` | Returns `true` if the key is present. |
| `name_remove(name* s, T key)` | Removes a key. Returns `false` if not present. |
| `name_min(name* s, T* outKey)` | Writes the smallest key. Returns `false` if the set is empty. |
| `name_max(name* s, T* outKey)` | Writes the largest key. Returns `false` if the set is empty. |
| `name_is_empty / clear / free` | Standard operations. |

---

## Thread-safe variants

`include/aglib_ds_safe_posix/` (Linux, macOS) and `include/aglib_ds_safe_win/` (Windows)

Every container also has a thread-safe counterpart with the exact same struct layout and function names, so switching between them requires no code changes beyond the one macro below. Each safe container embeds a platform-native recursive lock — `pthread_mutex_t` on Linux/macOS, `CRITICAL_SECTION` on Windows — and takes it internally around every operation that reads or writes shared state, so a single container instance can be safely accessed from multiple threads without any external locking. `aglib_ds.h` picks the right variant for the host platform automatically; you never include `aglib_ds_safe_posix` or `aglib_ds_safe_win` directly.

To use the thread-safe versions, define `THREAD_SAFE_AGLIB_DS` before including `aglib_ds.h` (or `ag.h`):

```c
#define THREAD_SAFE_AGLIB_DS
#include "ag.h"
```

`aglib_ds.h` uses this macro to switch between `include/aglib_ds/` (unsafe, faster, single-threaded) and the locked `aglib_ds_safe_*` variant for the host platform. You cannot mix the two for the same container instance — the macro is global to the translation unit.

Unlike the unsafe variants, `name_free` on a safe container both releases its memory through the allocator *and* destroys the container's internal lock, since the two are only ever torn down together. Call it once the container will no longer be used; the container must not be reused without calling `name_init` again.

**Composite containers** (`String`, `Heap`, `Stack`, `Queue`, `Set`, `TreeSet`) are built directly on top of `DynamicArray`, `LinkedList`, `Map`, or `TreeMap` and share the exact same struct — and therefore the exact same lock — as their underlying container. Internally this uses a recursive lock, so a composite operation (e.g. `Heap_push`, which both appends to the backing array and sifts the heap) locks once for the entire operation and safely re-enters the lock when it calls through to the underlying container's own locked functions. The practical effect is that the whole composite operation is atomic, not just the inner call.

**What is *not* covered:** the allocator backing a safe container is not itself synchronized. If multiple threads share one container that's fine — its lock serializes all access, including the allocations it triggers. But if you share a single `sAllocator`/`sArena`/`sSlab` directly across *independent* containers (or call `ag_alloc` yourself) from multiple threads, you must synchronize that access yourself, or give each thread/container its own allocator. 

**Example**
```c
#define THREAD_SAFE_AGLIB_DS
#include "ag.h"

static size_t hash_int(int x) { return (size_t)x; }
static bool   eq_int(int a, int b) { return a == b; }
Map(int, int, Counts, hash_int, eq_int)

void* worker(void* arg) {
    Counts* c = (Counts*)arg;
    for (int i = 0; i < 1000; i++)
        Counts_insert(c, i, i * i);
    return NULL;
}

sArena a;
arena_init(&a, MB(4));
sAllocator alloc = use_arena(&a);

Counts c;
Counts_init(&alloc, &c, 64);

pthread_t t1, t2;
pthread_create(&t1, NULL, worker, &c);
pthread_create(&t2, NULL, worker, &c);
pthread_join(t1, NULL);
pthread_join(t2, NULL);

Counts_free(&c);
arena_free(&a);
```

On Linux/macOS, link with `-lpthread` (or `-pthread`) when using the thread-safe variants; the `Makefile`'s Linux and macOS targets already do this for you. Windows needs no extra link flag — `CRITICAL_SECTION` is part of the Win32 API.

---

## Algorithms

`include/aglib_algo.h`

All four sorting functions sort an array of `nmemb` elements each of `size` bytes using a comparator with the same signature as the standard `qsort` comparator. They are prefixed with `ag_` so they don't collide with sort functions already declared by platform headers (notably macOS's libc).

```c
void ag_heapsort (void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sAllocator* a);
void ag_pdqsort  (void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sAllocator* a);
void ag_mergesort(void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sAllocator* a);
void ag_timsort  (void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sAllocator* a);
```

| Function | Time (avg) | Time (worst) | Space | Notes |
|---|---|---|---|---|
| `ag_pdqsort` | O(n log n) | O(n log n) | O(1) via allocator | Pattern-defeating quicksort: median-of-9 pivot selection on large partitions, falls back to heapsort after too many bad partitions, and detects already-sorted runs to bail out early. Fastest in practice on most inputs. |
| `ag_mergesort` | O(n log n) | O(n log n) | O(n) via allocator | Stable sort. Requires O(n) scratch space, requested through the allocator. |
| `ag_heapsort` | O(n log n) | O(n log n) | O(1) via allocator | Not stable. Requests a single element-sized swap buffer through the allocator; used internally as `ag_pdqsort`'s worst-case fallback. |
| `ag_timsort` | O(n log n) | O(n log n) | O(n) via allocator | Stable sort. Detects existing ascending/descending runs in the input and merges them, so it's fastest on partially-sorted data. Requires O(n) scratch space, requested through the allocator. |

`ag_pdqsort` and `ag_mergesort` use insertion sort for sub-arrays of 16 elements or fewer; `ag_timsort` uses it to pad any detected run up to its computed minimum run length.

**Example**
```c
int cmp_int(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}

sArena a;
arena_init(&a, KB(64));
sAllocator alloc = use_arena(&a);

int arr[] = { 5, 2, 8, 1, 9, 3 };
ag_pdqsort(arr, 6, sizeof(int), cmp_int, &alloc);

// Nearly-sorted or chunk-wise sorted data favors timsort instead:
int mostlySorted[] = { 1, 2, 3, 9, 8, 7, 6, 4, 5 };
ag_timsort(mostlySorted, 9, sizeof(int), cmp_int, &alloc);

arena_free(&a);
```

---

## I/O

`include/aglib_io.h`, plus `include/aglib_io_posix.h` (Linux, macOS) or `include/aglib_io_win.h` (Windows)

### Console Input

All input functions loop until valid input is received. They return `false` only on an unrecoverable error (e.g. EOF).

```c
bool get_str        (const char* prompt, char** str,  size_t len,  sAllocator* a);
bool get_char       (const char* prompt, char*  val);
bool get_opt        (const char* prompt, char*  val,  size_t nOpt, const char opts[]);
bool get_int        (const char* prompt, int*   val);
bool get_float      (const char* prompt, float* val);
bool get_int_range  (const char* prompt, int*   val,  int   min,   int   max);
bool get_float_range(const char* prompt, float* val,  float min,   float max);
```

| Function | Description |
|---|---|
| `get_str` | Reads a line of up to `len` bytes into allocator-owned memory. Strips the trailing newline. |
| `get_char` | Reads a single character, discarding the rest of the line. |
| `get_opt` | Reads a character and validates it against the `opts` array. Re-prompts on invalid input. |
| `get_int / get_float` | Reads and parses a number across the full type range. |
| `get_int_range / get_float_range` | Same as above but rejects values outside `[min, max]`. |

### File

The platform-specific header (`aglib_io_posix.h` or `aglib_io_win.h`, selected automatically by `ag.h` based on `_WIN32`) declares the functions that need a native file handle. On POSIX this is an `int` file descriptor; on Windows it's a `HANDLE`, and file sizes use `int64_t` instead of `off_t`. Everything else is shared and declared in `aglib_io.h`.

```c
// aglib_io_posix.h
bool  read_all_file   (int fd,   sFileBuffer* out, sAllocator* a);
bool  write_all_file  (int fd,   const void* data, size_t len);
bool  append_to_file  (int fd,   const void* data, size_t len);
off_t get_file_size_fd(int fd);
bool  find_path       (int fd,   sPath* out, sAllocator* a);

// aglib_io_win.h
bool    read_all_file   (HANDLE hFile, sFileBuffer* out, sAllocator* a);
bool    write_all_file  (HANDLE hFile, const void* data, size_t len);
bool    append_to_file  (HANDLE hFile, const void* data, size_t len);
int64_t get_file_size_fd(HANDLE hFile);
bool    find_path       (HANDLE hFile, sPath* out, sAllocator* a);

// aglib_io.h (shared)
bool path_exists(const char* path);
bool is_file    (const char* path);
bool file_copy  (const char* src, const char* dst);
bool file_delete(const char* path);
bool path_join  (const char* parts[], size_t n, sPath* out, sAllocator* a);
```

| Function | Description |
|---|---|
| `read_all_file` | Reads the entire contents of the file into an allocator-owned `sFileBuffer`. Also resolves and stores the file path. |
| `write_all_file` | Writes `len` bytes to the file, retrying on interruption. |
| `append_to_file` | Seeks to end of file and writes `len` bytes. |
| `get_file_size_fd` | Returns the file size in bytes given an open handle/descriptor. Returns `-1` on error. |
| `get_file_size` | Returns the file size in bytes given a path. Returns `-1` on error. |
| `path_exists` | Returns `true` if the path exists (file or directory). |
| `is_file` | Returns `true` if the path exists and is a regular file. |
| `file_copy` | Copies `src` to `dst`, creating or truncating `dst`. |
| `file_delete` | Deletes a file by path. |
| `find_path` | Resolves the filesystem path of an open handle/descriptor. |
| `path_join` | Joins `n` path segments with `/` separators into an allocator-owned `sPath`. |

### Directory

```c
bool is_dir    (const char* path);
bool dir_create(const char* path, bool recursive);
bool dir_delete(const char* path, bool recursive);
bool dir_list  (const char* path, sDirList* out,  sAllocator* a);
bool dir_walk  (const char* root, bool recursive, WalkCallback cb, void* userdata, sAllocator* a);
```

| Function | Description |
|---|---|
| `is_dir` | Returns `true` if the path exists and is a directory. |
| `dir_create` | Creates a directory. If `recursive` is `true`, creates all intermediate components. |
| `dir_delete` | Removes a directory. If `recursive` is `true`, removes all contents first. |
| `dir_list` | Populates `out` with the names of all entries in `path`, excluding `.` and `..`. Names are allocator-owned. |
| `dir_walk` | Iteratively traverses a directory tree, calling `cb` for every entry. If `recursive` is `true`, descends into subdirectories. Traversal stops early if `cb` returns `false`. |

**WalkCallback**
```c
typedef bool (*WalkCallback)(const char* path, bool isDir, void* userdata);
```
Receives the full path of each entry, whether it is a directory, and the caller-supplied `userdata` pointer.

**Example**
```c
bool print_entry(const char* path, bool isDir, void* userdata) {
    printf("[%s] %s\n", isDir ? "DIR" : "FILE", path);
    return true;
}

sArena a;
arena_init(&a, MB(1));
sAllocator alloc = use_arena(&a);

dir_walk("/etc", true, print_entry, NULL, &alloc);
arena_free(&a);
```

---

## Helpers

`include/aglib_helpers.h`

Utility macros available throughout the library and to consumers.

| Macro | Description |
|---|---|
| `ERR(fmt, ...)` | Prints a formatted error to `stderr` prefixed with the file and line number. |
| `TODO(message)` | Prints a message to `stderr` and calls `abort()`. Marks unimplemented code paths. |
| `MAX(a, b)` | Returns the larger of two values. |
| `MIN(a, b)` | Returns the smaller of two values. |
| `SWAP(T, a, b)` | Swaps two values of type `T` using a temporary. |
| `KB(n) / MB(n) / GB(n)` | Converts `n` to bytes (e.g. `MB(4)` → `4194304`). |

---

## Building

The `Makefile` can build the static library for Linux, Windows, and macOS from any single host machine, using cross-compilation where needed.

```bash
make                 # build the static lib for THIS host's platform
make test            # run the (non-thread-safe) test suite for THIS host
make test-safe       # run the thread-safe test suite for THIS host

make all-linux / all-win / all-mac        # build just the static lib for that platform
make test-linux / test-win / test-mac     # build + run that platform's tests
make test-safe-linux / -win / -mac        # same, thread-safe variant

make clean           # remove every platform's build output
```

Each target produces `build/<platform>/libag.a`. Cross-compiling for Windows uses `x86_64-w64-mingw32-gcc` by default, and for macOS uses `o64-clang` by default (both overridable — see below); running the resulting test binaries on a non-native host additionally requires `wine`/`wine64` (Windows binaries) or real macOS hardware (macOS binaries can be cross-built anywhere, but only executed on macOS).

Every compiler is overridable per platform, e.g.:
```bash
make test-win WIN_CC=/path/to/my-mingw-gcc
make all-mac  MAC_CC=/path/to/my-osxcross-clang
```

To use in your own project, link against the platform's `libag.a` and add `include/` to your include path:

```bash
gcc main.c -Ipath/to/aglib/include -Lpath/to/aglib/build/linux -lag -o main
```

Or include `ag.h` to pull in all modules at once:

```c
#include "ag.h"
```

If you use the thread-safe data structures (`#define THREAD_SAFE_AGLIB_DS`, see [Thread-safe variants](#thread-safe-variants)) on Linux or macOS, also link `-lpthread`:

```bash
gcc main.c -Ipath/to/aglib/include -Lpath/to/aglib/build/linux -lag -lpthread -o main
```

CI (`.github/workflows/ci.yml`) builds and runs both `make test` and `make test-safe` natively on `ubuntu-latest`, `macos-latest`, and `windows-latest`.

---

## Testing

The test suite lives in `tests/` — a `test_main.c` driver plus one file per module (arena, algorithms, I/O, and every data structure), built on a small custom `test_framework.h`/`.c`. Run it after any change:

```bash
make test              # default (non-thread-safe) data structures, this host's platform
make test-safe         # rebuilds against the thread-safe variant (-DTHREAD_SAFE_AGLIB_DS)

make test-linux / test-win / test-mac             # same, for a specific platform
make test-safe-linux / test-safe-win / test-safe-mac
```

Each run prints a per-module pass/fail summary followed by an overall result; the process exits non-zero if any assertion failed.

---

## Credits

The test suite (`tests/`), the `Makefile`, and this README were written by Claude.
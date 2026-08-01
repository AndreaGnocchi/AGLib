# aglib

A lightweight, Linux-only C utility library providing arena-based memory management, generic data structures, sorting algorithms, and I/O utilities.

> **Platform requirement:** Linux only. The library will refuse to compile on other platforms.

---

## Modules

- [Arena](#arena)
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

---

## Arena

`include/aglib_arena.h`

A linear allocator backed by an `mmap` region. Allocations are O(1) and freeing is done all at once by resetting or releasing the arena. Temporary sub-arenas allow scoped allocation within a larger arena.

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
| `arena_free` | Releases the underlying `mmap` region and zeroes the arena struct. |
| `arena_temp_start` | Saves the current arena offset and returns a `sTempArena` checkpoint. |
| `arena_temp_end` | Restores the arena to the offset saved by `arena_temp_start`, freeing all allocations made since. |

**Example**
```c
sArena a;
arena_init(&a, MB(4));

char* buf = arena_alloc(&a, 256, false);

sTempArena tmp = arena_temp_start(&a);
int* scratch   = arena_alloc(&a, sizeof(int) * 1000, true);
// ... use scratch ...
arena_temp_end(tmp);   // scratch is freed, buf is unaffected

arena_free(&a);
```

---

## Data Structures

`include/aglib_ds.h`

All data structures are defined via macros that generate type-safe structs and functions for a given element type. All allocations go through a caller-provided `sArena`.

`aglib_ds.h` is an umbrella header — it just pulls in one file per container from `include/aglib_ds/` (or, if `THREAD_SAFE_AGLIB_DS` is defined, from `include/aglib_ds_safe/` — see [Thread-safe variants](#thread-safe-variants)). Include it to get everything, or include an individual `aglib_ds/<container>.h` file if you only need one:

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

---

### DynamicArray

```c
DynamicArray(T, name)
```

Generates a growable array of type `T` named `name`. When the array is full, a new backing block of double the capacity is allocated from the arena and the existing items are copied over.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* arr, size_t initCap)` | Initialises the array with an initial capacity. |
| `name_push(name* arr, T item)` | Appends an item, doubling capacity if needed. Returns `false` on allocation failure. |
| `name_clear(name* arr)` | Zeroes all items and resets size to zero. |
| `name_is_empty(name* arr)` | Returns `true` if the array has no items. |

**Example**
```c
DynamicArray(int, IntArray)

sArena a;
arena_init(&a, KB(16));

IntArray arr;
IntArray_init(&a, &arr, 8);
IntArray_push(&arr, 42);
IntArray_push(&arr, 7);

for (size_t i = 0; i < arr.size; i++)
    printf("%d\n", arr.items[i]);

arena_free(&a);
```

---

### String

```c
String(name)
```

Generates a growable, arena-backed string type named `name`. Internally it is a `DynamicArray(char, ...)` that always maintains a null terminator. A separate read-only `sStringView` type is used for non-owning slices.

```c
typedef struct {
    const char* ptr;
    size_t      len;
} sStringView;
```

| Function | Description |
|---|---|
| `name_init(sArena* a, name* str, const char* init)` | Initialises the string by copying `init`. |
| `name_append(name* str, const char* toAppend)` | Appends a C string, growing the buffer if needed. |
| `name_appendf(sArena* a, name* str, const char* fmt, ...)` | Replaces the string content with a formatted result (like `sprintf`). Grows the buffer if needed. |
| `name_slice(name* str, size_t start, size_t len)` | Returns an `sStringView` into the string. Returns a zeroed view if the range is out of bounds. |
| `name_cstr(name* str)` | Returns a `const char*` pointer to the null-terminated contents. |
| `name_clear(name* str)` | Resets size to zero and zeroes the backing buffer. |

**Example**
```c
String(MyStr)

sArena a;
arena_init(&a, KB(4));

MyStr s;
MyStr_init(&a, &s, "hello");
MyStr_append(&s, ", world");

printf("%s\n", MyStr_cstr(&s));  // hello, world

sStringView view = MyStr_slice(&s, 0, 5);
printf("%.*s\n", (int)view.len, view.ptr);  // hello

MyStr_appendf(&a, &s, "%s is %d", "count", 42);
printf("%s\n", MyStr_cstr(&s));  // count is 42  (appendf replaces the content, it doesn't append)

arena_free(&a);
```

---

### LinkedList

```c
LinkedList(T, name)
```

Generates a doubly linked list of type `T` named `name`. Each node (`nameNode`) stores a pointer to an arena-allocated copy of the value, plus `next` and `prev` pointers.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* list)` | Initialises an empty list. |
| `name_push_head / push_tail` | Prepends or appends a value. |
| `name_pop_head / pop_tail` | Removes and optionally returns the head or tail value. |
| `name_peek_head / peek_tail` | Reads the head or tail value without removing it. |
| `name_insert_after(name* list, nameNode* node, T val)` | Inserts a value immediately after `node`. |
| `name_insert_before(name* list, nameNode* node, T val)` | Inserts a value immediately before `node`. |
| `name_remove(name* list, nameNode* node)` | Unlinks a node from the list. |
| `name_clear(name* list)` | Resets head, tail, and size without freeing memory. |
| `name_is_empty(name* list)` | Returns `true` if the list has no nodes. |

---

### Stack

```c
Stack(T, name)
```

LIFO stack built on top of `LinkedList`. Push and pop operate on the head.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* s)` | Initialises an empty stack. |
| `name_push(name* s, T val)` | Pushes a value onto the top. |
| `name_pop(name* s, T* outVal)` | Pops the top value into `outVal`. Returns `false` if empty. |
| `name_peek(name* s, T* outVal)` | Reads the top value without removing it. |
| `name_is_empty / clear` | Standard empty check and reset. |

---

### Queue

```c
Queue(T, name)
```

FIFO queue built on top of `LinkedList`. Enqueue appends to the tail, dequeue removes from the head.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* q)` | Initialises an empty queue. |
| `name_enqueue(name* q, T val)` | Adds a value to the back. |
| `name_dequeue(name* q, T* outVal)` | Removes the front value into `outVal`. Returns `false` if empty. |
| `name_peek / is_empty / clear` | Standard operations. |

---

### Heap

```c
Heap(T, name, cmp)
```

A binary heap (priority queue) backed by a `DynamicArray`. `cmp(a, b)` must return `true` when `a` should be closer to the top than `b`.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* h, size_t initCap)` | Initialises the heap. |
| `name_push(name* h, T val)` | Inserts a value and sifts up. |
| `name_pop(name* h, T* outVal)` | Removes the top value and sifts down. Returns `false` if empty. |
| `name_peek(name* h, T* outVal)` | Reads the top value without removing it. |
| `name_is_empty / clear` | Standard operations. |

**Example — min-heap of ints**
```c
Heap(int, MinHeap, ({ bool _cmp(int a, int b) { return a < b; } _cmp; }))

// Simpler with a named comparator:
static bool int_lt(int a, int b) { return a < b; }
Heap(int, MinHeap, int_lt)

sArena a;
arena_init(&a, KB(4));

MinHeap h;
MinHeap_init(&a, &h, 16);
MinHeap_push(&h, 5);
MinHeap_push(&h, 1);
MinHeap_push(&h, 3);

int top;
MinHeap_pop(&h, &top);  // top == 1

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
| `name_init(sArena* a, name* hm, size_t initCap)` | Initialises the map with an initial capacity. |
| `name_insert(name* hm, Tk key, Tv val)` | Inserts or updates a key-value pair. Auto-resizes at 75% load. Returns `false` on allocation failure. |
| `name_get(name* hm, Tk key, Tv* outVal)` | Looks up a key. Writes the value to `outVal` if found. Returns `false` if not found. |
| `name_delete(name* hm, Tk key)` | Removes a key using Robin Hood backward shift. Returns `false` if the key is not present. |
| `name_resize(name* hm, size_t newCap)` | Allocates a new backing block of `newCap` and re-inserts all active entries. `newCap` must be greater than the current size. The old block is abandoned in the arena. Returns `false` on allocation failure. |
| `name_is_empty / clear` | Standard operations. |

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

WordCount wc;
WordCount_init(&a, &wc, 64);

int count = 0;
if (!WordCount_get(&wc, "hello", &count)) count = 0;
WordCount_insert(&wc, "hello", count + 1);

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
| `name_init(sArena* a, name* s, size_t initCap)` | Initialises the set. |
| `name_insert(name* s, T key)` | Adds a key to the set. Auto-resizes at 75% load. |
| `name_contains(name* s, T key)` | Returns `true` if the key is present. |
| `name_remove(name* s, T key)` | Removes a key. Returns `false` if not present. |
| `name_resize(name* s, size_t newCap)` | Delegates to the underlying `Map` resize. |
| `name_is_empty / clear` | Standard operations. |

---

### TreeMap

```c
TreeMap(Tk, Tv, name, cmp_fn)
```

An ordered map from keys of type `Tk` to values of type `Tv`, implemented as a red-black tree. `cmp_fn(Tk, Tk) -> int` must behave like a standard comparator (negative, zero, or positive). Unlike `Map`, keys are kept in sorted order, so `_min` / `_max` and in-order traversal are supported.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* t)` | Initialises an empty tree. |
| `name_insert(name* t, Tk key, Tv val)` | Inserts a new key-value pair. Returns `false` if the key already exists. |
| `name_find(name* t, Tk key, Tv* out)` | Looks up a key. Writes the value to `out` if found. Returns `false` if not found. |
| `name_remove(name* t, Tk key)` | Removes a key, rebalancing the tree. Returns `false` if the key is not present. |
| `name_min(name* t, Tk* outKey, Tv* outVal)` | Writes the smallest key/value pair. Returns `false` if the tree is empty. |
| `name_max(name* t, Tk* outKey, Tv* outVal)` | Writes the largest key/value pair. Returns `false` if the tree is empty. |
| `name_is_empty(name* t)` | Returns `true` if the tree has no nodes. |
| `name_clear(name* t)` | Resets the tree to empty. |

**Example**
```c
static int cmp_int(int a, int b) { return a - b; }

TreeMap(int, const char*, IntTree, cmp_int)

sArena a;
arena_init(&a, KB(16));

IntTree t;
IntTree_init(&a, &t);
IntTree_insert(&t, 5, "five");
IntTree_insert(&t, 1, "one");
IntTree_insert(&t, 9, "nine");

int minKey; const char* minVal;
IntTree_min(&t, &minKey, &minVal);  // minKey == 1, minVal == "one"

const char* v;
if (IntTree_find(&t, 5, &v))
    printf("%s\n", v);  // five

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
| `name_init(sArena* a, name* s)` | Initialises an empty set. |
| `name_insert(name* s, T key)` | Adds a key to the set. Returns `false` if already present. |
| `name_contains(name* s, T key)` | Returns `true` if the key is present. |
| `name_remove(name* s, T key)` | Removes a key. Returns `false` if not present. |
| `name_min(name* s, T* outKey)` | Writes the smallest key. Returns `false` if the set is empty. |
| `name_max(name* s, T* outKey)` | Writes the largest key. Returns `false` if the set is empty. |
| `name_is_empty / clear` | Standard operations. |

---

## Thread-safe variants

`include/aglib_ds_safe/`

Every container also has a thread-safe counterpart with the exact same struct layout and function names, so switching between them requires no code changes beyond the one macro below. Each safe container embeds a `pthread_mutex_t` and takes it internally around every operation that reads or writes shared state, so a single container instance can be safely accessed from multiple threads without any external locking.

To use the thread-safe versions, define `THREAD_SAFE_AGLIB_DS` before including `aglib_ds.h` (or `ag.h`):

```c
#define THREAD_SAFE_AGLIB_DS
#include "ag.h"
```

`aglib_ds.h` uses this macro to switch between `include/aglib_ds/` (unsafe, faster, single-threaded) and `include/aglib_ds_safe/` (locked, for shared use across threads). You cannot mix the two for the same container instance — the macro is global to the translation unit.

| Function | Description |
|---|---|
| `name_destroy(name* ds)` | Destroys the container's internal mutex. Call this once the container will no longer be used, after which the container must not be reused without calling `name_init` again. Unlike the unsafe variants, the arena still owns the container's memory — `name_destroy` only releases the OS-level mutex, it does not free anything from the arena. |

**Composite containers** (`String`, `Heap`, `Stack`, `Queue`, `Set`, `TreeSet`) are built directly on top of `DynamicArray`, `LinkedList`, `Map`, or `TreeMap` and share the exact same struct — and therefore the exact same mutex — as their underlying container. Internally this uses a recursive mutex, so a composite operation (e.g. `Heap_push`, which both appends to the backing array and sifts the heap) locks once for the entire operation and safely re-enters the lock when it calls through to the underlying container's own locked functions. The practical effect is that the whole composite operation is atomic, not just the inner call.

**What is *not* covered:** the `sArena` backing a safe container is not itself synchronized. If multiple threads share one container that's fine — its lock serializes all access, including the arena allocations it triggers. But if you share a single `sArena` directly across *independent* containers (or call `arena_alloc` yourself) from multiple threads, you must synchronize that access yourself, or give each thread/container its own arena.

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

Counts c;
Counts_init(&a, &c, 64);

pthread_t t1, t2;
pthread_create(&t1, NULL, worker, &c);
pthread_create(&t2, NULL, worker, &c);
pthread_join(t1, NULL);
pthread_join(t2, NULL);

Counts_destroy(&c);
arena_free(&a);
```

Link with `-lpthread` (or `-pthread`) when using the thread-safe variants.

---

## Algorithms

`include/aglib_algo.h`

All three sorting functions sort an array of `nmemb` elements each of `size` bytes using a comparator with the same signature as the standard `qsort` comparator.

```c
void pdqsort  (void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sArena* a);
void mergesort(void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sArena* a);
void heapsort (void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*));
```

| Function | Time (avg) | Time (worst) | Space | Notes |
|---|---|---|---|---|
| `pdqsort` | O(n log n) | O(n log n) | O(1) arena | Pattern-defeating quicksort; falls back to heapsort on adversarial input. Fastest in practice. |
| `mergesort` | O(n log n) | O(n log n) | O(n) arena | Stable sort. Requires O(n) scratch space from the arena. |
| `heapsort` | O(n log n) | O(n log n) | O(1) stack | No arena required. Uses a single stack-allocated swap buffer. |

Both `pdqsort` and `mergesort` use insertion sort for sub-arrays of 16 elements or fewer.

**Example**
```c
int cmp_int(const void* a, const void* b) {
    return *(int*)a - *(int*)b;
}

sArena a;
arena_init(&a, KB(64));

int arr[] = { 5, 2, 8, 1, 9, 3 };
pdqsort(arr, 6, sizeof(int), cmp_int, &a);

arena_free(&a);
```

---

## I/O

`include/aglib_io.h`

### Console Input

All input functions loop until valid input is received. They return `false` only on an unrecoverable error (e.g. EOF).

```c
bool get_str        (const char* prompt, char** str,  size_t len,  sArena* a);
bool get_char       (const char* prompt, char*  val);
bool get_opt        (const char* prompt, char*  val,  size_t nOpt, const char opts[]);
bool get_int        (const char* prompt, int*   val);
bool get_float      (const char* prompt, float* val);
bool get_int_range  (const char* prompt, int*   val,  int   min,   int   max);
bool get_float_range(const char* prompt, float* val,  float min,   float max);
```

| Function | Description |
|---|---|
| `get_str` | Reads a line of up to `len` bytes into arena-allocated memory. Strips the trailing newline. |
| `get_char` | Reads a single character, discarding the rest of the line. |
| `get_opt` | Reads a character and validates it against the `opts` array. Re-prompts on invalid input. |
| `get_int / get_float` | Reads and parses a number across the full type range. |
| `get_int_range / get_float_range` | Same as above but rejects values outside `[min, max]`. |

### File

```c
bool  read_all_file   (int fd,              sFileBuffer* out, sArena* a);
bool  write_all_file  (int fd,              const void* data, size_t len);
bool  append_to_file  (int fd,              const void* data, size_t len);
off_t get_file_size_fd(int fd);
off_t get_file_size   (const char* path);
bool  path_exists     (const char* path);
bool  is_file         (const char* path);
bool  file_copy       (const char* src,     const char* dst);
bool  file_delete     (const char* path);
bool  find_path       (int fd,              sPath* out, sArena* a);
bool  path_join       (const char* parts[], size_t n,   sPath* out, sArena* a);
```

| Function | Description |
|---|---|
| `read_all_file` | Reads the entire contents of `fd` into an arena-allocated `sFileBuffer`. Also resolves and stores the file path. |
| `write_all_file` | Writes `len` bytes to `fd`, retrying on `EINTR`. |
| `append_to_file` | Seeks to end of file and writes `len` bytes. |
| `get_file_size_fd` | Returns the file size in bytes given an open descriptor. Returns `-1` on error. |
| `get_file_size` | Returns the file size in bytes given a path. Returns `-1` on error. |
| `path_exists` | Returns `true` if the path exists (file or directory). |
| `is_file` | Returns `true` if the path exists and is a regular file. |
| `file_copy` | Copies `src` to `dst` using `sendfile`. Creates or truncates `dst`. |
| `file_delete` | Deletes a file by path. |
| `find_path` | Resolves the filesystem path of an open file descriptor via `/proc/self/fd`. |
| `path_join` | Joins `n` path segments with `/` separators into an arena-allocated `sPath`. |

### Directory

```c
bool is_dir    (const char* path);
bool dir_create(const char* path, bool recursive);
bool dir_delete(const char* path, bool recursive);
bool dir_list  (const char* path, sDirList* out,  sArena* a);
bool dir_walk  (const char* root, bool recursive, WalkCallback cb, void* userdata, sArena* a);
```

| Function | Description |
|---|---|
| `is_dir` | Returns `true` if the path exists and is a directory. |
| `dir_create` | Creates a directory. If `recursive` is `true`, creates all intermediate components. |
| `dir_delete` | Removes a directory. If `recursive` is `true`, removes all contents first. |
| `dir_list` | Populates `out` with the names of all entries in `path`, excluding `.` and `..`. Names are arena-allocated. |
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
dir_walk("/etc", true, print_entry, NULL, &a);
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

A static library `build/ag.a` is produced by the included `Makefile`.

```bash
make        # builds build/ag.a
make clean  # removes the build directory
```

To use in your own project, link against `ag.a` and add `include/` to your include path:

```bash
gcc main.c -Ipath/to/aglib/include -Lpath/to/aglib/build -lag -o main
```

Or include `ag.h` to pull in all modules at once:

```c
#include "ag.h"
```

If you use the thread-safe data structures (`#define THREAD_SAFE_AGLIB_DS`, see [Thread-safe variants](#thread-safe-variants)), also link `-lpthread`:

```bash
gcc main.c -Ipath/to/aglib/include -Lpath/to/aglib/build -lag -lpthread -o main
```
## Testing

A test suite (`test.c`) exercises every module — arena, algorithms,
I/O, and all data structures — using `assert`. Run it after any change:

    make test         # default (non-thread-safe) data structures
    make test-safe    # rebuilds against the thread-safe variant (-DTHREAD_SAFE_AGLIB_DS)

Both should print `All checks passed` with no output before it.
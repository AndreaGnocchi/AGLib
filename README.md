# aglib

A lightweight, Linux-only C utility library providing arena-based memory management, generic data structures, sorting algorithms, and I/O utilities.

> **Platform requirement:** Linux only. The library will refuse to compile on other platforms.

---

## Modules

- [Arena](#arena)
- [Data Structures](#data-structures)
- [Algorithms](#algorithms)
- [I/O](#io)
- [Helpers](#helpers)

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

### DynamicArray

```c
DynamicArray(T, name)
```

Generates a growable array of type `T` named `name`.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* arr, size_t initCap)` | Initialises the array with an initial capacity. |
| `name_push(name* arr, T item)` | Appends an item, doubling capacity if needed. Returns `false` on allocation failure. |
| `name_clear(name* arr)` | Zeroes all items and resets size to zero. |
| `name_is_empty(name* arr)` | Returns `true` if the array has no items. |

### LinkedList

```c
LinkedList(T, name)
```

Generates a doubly linked list of type `T` named `name`. Nodes are allocated from the arena.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* list)` | Initialises an empty list. |
| `name_push_head / push_tail` | Prepends or appends a value. |
| `name_pop_head / pop_tail` | Removes and optionally returns the head or tail value. |
| `name_peek_head / peek_tail` | Reads the head or tail value without removing it. |
| `name_insert_after / insert_before` | Inserts a value relative to a given node. |
| `name_remove(name* list, nameNode* node)` | Unlinks a node from the list. |
| `name_clear` | Resets head, tail, and size without freeing memory. |
| `name_is_empty` | Returns `true` if the list has no nodes. |

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

### Heap

```c
Heap(T, name, cmp)
```

A binary heap (priority queue) backed by a `DynamicArray`. `cmp(a, b)` must return `true` when `a` should be closer to the top than `b`.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* h, size_t initCap)` | Initialises the heap. |
| `name_push(name* h, T val)` | Inserts a value and sifts up. |
| `name_pop(name* h, T* outVal)` | Removes the top value, sifts down. Returns `false` if empty. |
| `name_peek(name* h, T* outVal)` | Reads the top value without removing it. |
| `name_is_empty / clear` | Standard operations. |

### Map

```c
Map(Tk, Tv, name, hash_fn, eq_fn)
```

A hash map from keys of type `Tk` to values of type `Tv` using Robin Hood open addressing. `hash_fn(Tk) -> size_t` and `eq_fn(Tk, Tk) -> bool` must be provided.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* hm, size_t initCap)` | Initialises the map with an initial capacity. |
| `name_insert(name* hm, Tk key, Tv val)` | Inserts or updates a key-value pair. Returns `false` if the map is full. |
| `name_get(name* hm, Tk key, Tv* outVal)` | Looks up a key. Writes the value to `outVal` if found. Returns `false` if not found. |
| `name_resize(name* hm, size_t newCap)` | Allocates a new entries block of `newCap` and re-inserts all active entries. `newCap` must be greater than the current size. The old block is abandoned in the arena. Returns `false` on allocation failure. |
| `name_is_empty / clear` | Standard operations. |

### Set

```c
Set(T, name, hash_fn, eq_fn)
```

A hash set built on top of `Map` with `bool` as the value type.

| Function | Description |
|---|---|
| `name_init(sArena* a, name* s, size_t initCap)` | Initialises the set. |
| `name_insert(name* s, T key)` | Adds a key to the set. |
| `name_contains(name* s, T key)` | Returns `true` if the key is present. |
| `name_resize(name* s, size_t newCap)` | Delegates to the underlying `Map` resize. |
| `name_is_empty / clear` | Standard operations. |

---

## Algorithms

`include/aglib_algo.h`

All three sorting functions sort an array of `nmemb` elements each of `size` bytes using the provided comparator, matching the signature of the standard `qsort` comparator.

```c
void pdqsort  (void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sArena* a);
void mergesort(void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*), sArena* a);
void heapsort (void* base, size_t nmemb, size_t size, int (*cmp)(const void*, const void*));
```

| Function | Time (avg) | Time (worst) | Space | Notes |
|---|---|---|---|---|
| `pdqsort` | O(n log n) | O(n log n) | O(1) arena | Pattern-defeating quicksort; falls back to heapsort on adversarial input. Fastest in practice. |
| `mergesort` | O(n log n) | O(n log n) | O(n) arena | Stable sort. Requires O(n) scratch space from the arena. |
| `heapsort` | O(n log n) | O(n log n) | O(1) | No arena required. Manages its own internal allocation. |

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
bool  read_all_file   (int fd,          sFileBuffer* out, sArena* a);
bool  write_all_file  (int fd,          const char*  data, size_t len);
bool  append_to_file  (int fd,          const char*  data, size_t len);
off_t get_file_size_fd(int fd);
off_t get_file_size   (const char* path);
bool  path_exists     (const char* path);
bool  is_file         (const char* path);
bool  file_copy       (const char* src,  const char* dst);
bool  file_delete     (const char* path);
bool  find_path       (int fd,           sPath* out, sArena* a);
bool  path_join       (const char* parts[], size_t n, sPath* out, sArena* a);
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
bool dir_list  (const char* path, sDirList* out, sArena* a);
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

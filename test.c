#include "include/ag.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// ═══════════════════════════════════════════════════════════════════════
// Shared helper functions (hash/eq/cmp) used across multiple test sections
// ═══════════════════════════════════════════════════════════════════════

static uint64_t hash_int(int x) {
    uint64_t h = (uint64_t)x;
    h += 0x9E3779B97F4A7C15ULL;
    h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
    h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
    return h ^ (h >> 31);
}
static bool eq_int(int a, int b) { return a == b; }
static int  cmp_int(int a, int b) { return a - b; }      // for TreeMap/TreeSet
static bool less_int(int a, int b) { return a < b; }     // for Heap (min-heap)

static int qcmp_int(const void* a, const void* b) {
    return (*(const int*)a) - (*(const int*)b);
}

// ═══════════════════════════════════════════════════════════════════════
// 1. Arena
// ═══════════════════════════════════════════════════════════════════════

static void test_arena(void) {
    sArena a;
    assert(arena_init(&a, KB(4)));

    int* p1 = (int*)arena_alloc(&a, sizeof(int), true);
    assert(p1 != NULL);
    assert(*p1 == 0);

    *p1 = 42;
    int* p2 = (int*)arena_alloc(&a, sizeof(int), false);
    assert(p2 != NULL);
    assert(p1 != p2);

    void* pa = arena_alloc_aligned(&a, 16, false, 16);
    assert(pa != NULL);
    assert(((uintptr_t)pa % 16) == 0);

    size_t usedBeforeReset = a.offset;
    (void)usedBeforeReset;
    arena_reset(&a);
    assert(a.offset == 0);

    void* beforeTemp = arena_alloc(&a, 8, false);
    (void)beforeTemp;
    size_t offsetBeforeTemp = a.offset;

    sTempArena temp = arena_temp_start(&a);
    arena_alloc(&a, 256, false);
    assert(a.offset != offsetBeforeTemp);
    arena_temp_end(temp);
    assert(a.offset == offsetBeforeTemp);

    arena_free(&a);
    printf("[OK] arena\n");
}

// ═══════════════════════════════════════════════════════════════════════
// 2. Algo (sorting)
// ═══════════════════════════════════════════════════════════════════════

static bool is_sorted(int* arr, size_t n) {
    for (size_t i = 1; i < n; i++)
        if (arr[i - 1] > arr[i]) return false;
    return true;
}

static void test_algo(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    int base[] = { 9, 2, 7, 1, 5, 5, 3, 8, 0, 6, 4 };
    size_t n = sizeof(base) / sizeof(base[0]);

    int arr1[sizeof(base) / sizeof(base[0])];
    memcpy(arr1, base, sizeof(base));
    pdqsort(arr1, n, sizeof(int), qcmp_int, &a);
    assert(is_sorted(arr1, n));

    int arr2[sizeof(base) / sizeof(base[0])];
    memcpy(arr2, base, sizeof(base));
    mergesort(arr2, n, sizeof(int), qcmp_int, &a);
    assert(is_sorted(arr2, n));

    int arr3[sizeof(base) / sizeof(base[0])];
    memcpy(arr3, base, sizeof(base));
    heapsort(arr3, n, sizeof(int), qcmp_int);
    assert(is_sorted(arr3, n));

    int empty[1];
    pdqsort(empty, 0, sizeof(int), qcmp_int, &a);

    int single[1] = { 42 };
    mergesort(single, 1, sizeof(int), qcmp_int, &a);
    assert(single[0] == 42);

    arena_free(&a);
    printf("[OK] algo (pdqsort, mergesort, heapsort)\n");
}

// ═══════════════════════════════════════════════════════════════════════
// 3. IO
// ═══════════════════════════════════════════════════════════════════════

#define TEST_FILE_A "test_io_a.tmp"
#define TEST_FILE_B "test_io_b.tmp"
#define TEST_DIR    "test_io_dir.tmp"

static void cleanup_io_artifacts(void) {
    file_delete(TEST_FILE_A);
    file_delete(TEST_FILE_B);
    dir_delete(TEST_DIR, true);
}

static void test_io(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    cleanup_io_artifacts();

    int fd = open(TEST_FILE_A, O_RDWR | O_CREAT | O_TRUNC, 0644);
    assert(fd >= 0);

    assert(write_all_file(fd, "hello", 5));
    assert(get_file_size_fd(fd) == 5);

    assert(append_to_file(fd, " world", 6));
    assert(get_file_size_fd(fd) == 11);
    close(fd);

    assert(get_file_size(TEST_FILE_A) == 11);

    fd = open(TEST_FILE_A, O_RDONLY);
    assert(fd >= 0);
    sFileBuffer buf;
    assert(read_all_file(fd, &buf, &a));
    assert(buf.size == 11);
    assert(memcmp(buf.data, "hello world", 11) == 0);
    close(fd);

    assert(path_exists(TEST_FILE_A));
    assert(is_file(TEST_FILE_A));
    assert(!path_exists("definitely_does_not_exist.tmp"));

    assert(file_copy(TEST_FILE_A, TEST_FILE_B));
    assert(get_file_size(TEST_FILE_B) == 11);
    fd = open(TEST_FILE_B, O_RDONLY);
    sFileBuffer buf2;
    assert(read_all_file(fd, &buf2, &a));
    assert(memcmp(buf2.data, "hello world", 11) == 0);
    close(fd);

    assert(!file_copy(TEST_FILE_A, TEST_FILE_A));
    assert(get_file_size(TEST_FILE_B) == 11);

    sPath joined;
    const char* parts[] = { "foo", "bar", "baz.txt" };
    assert(path_join(parts, 3, &joined, &a));
    assert(strcmp(joined.path, "foo/bar/baz.txt") == 0);

    assert(dir_create(TEST_DIR, true));
    assert(is_dir(TEST_DIR));

    sPath nestedFile;
    const char* nestedParts[] = { TEST_DIR, "inner.txt" };
    assert(path_join(nestedParts, 2, &nestedFile, &a));

    int nfd = open(nestedFile.path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    assert(nfd >= 0);
    assert(write_all_file(nfd, "x", 1));
    close(nfd);

    sDirList list;
    assert(dir_list(TEST_DIR, &list, &a));
    assert(list.count == 1);
    assert(strcmp(list.entries[0], "inner.txt") == 0);

    assert(dir_delete(TEST_DIR, true));
    assert(!path_exists(TEST_DIR));

    assert(file_delete(TEST_FILE_A));
    assert(!path_exists(TEST_FILE_A));
    assert(file_delete(TEST_FILE_B));

    arena_free(&a);
    printf("[OK] io\n");
}

// ═══════════════════════════════════════════════════════════════════════
// 4. Data structures
// ═══════════════════════════════════════════════════════════════════════

DynamicArray(int, IntArray)
String(TestStr)
LinkedList(int, IntLinkedList)
Stack(int, IntStack)
Queue(int, IntQueue)
Heap(int, IntHeap, less_int)
Map(int, int, IntMap, hash_int, eq_int)
Set(int, IntSet, hash_int, eq_int)
TreeMap(int, int, IntTreeMap, cmp_int)
TreeSet(int, IntTreeSet, cmp_int)

static void test_array(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntArray arr;
    IntArray_init(&a, &arr, 2);
    assert(IntArray_is_empty(&arr));

    for (int i = 0; i < 10; i++)
        assert(IntArray_push(&arr, i));

    assert(!IntArray_is_empty(&arr));
    assert(arr.size == 10);
    for (int i = 0; i < 10; i++)
        assert(arr.items[i] == i);

    IntArray_clear(&arr);
    assert(IntArray_is_empty(&arr));

    arena_free(&a);
    printf("[OK] DynamicArray\n");
}

static void test_string(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    TestStr s;
    TestStr_init(&a, &s, "hello");
    assert(strcmp(TestStr_cstr(&s), "hello") == 0);

    assert(TestStr_append(&s, " world"));
    assert(strcmp(TestStr_cstr(&s), "hello world") == 0);

    assert(TestStr_appendf(&a, &s, " %d/%d", 1, 2));
    assert(strcmp(TestStr_cstr(&s), "hello world 1/2") == 0);

    sStringView v = TestStr_slice(&s, 0, 5);
    assert(v.len == 5);
    assert(memcmp(v.ptr, "hello", 5) == 0);

    sStringView bad = TestStr_slice(&s, 1000, 5);
    assert(bad.ptr == NULL);

    TestStr_clear(&s);

    arena_free(&a);
    printf("[OK] String\n");
}

static void test_linked_list(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntLinkedList list;
    IntLinkedList_init(&a, &list);
    assert(IntLinkedList_is_empty(&list));

    assert(IntLinkedList_push_head(&list, 2));
    assert(IntLinkedList_push_head(&list, 1));
    assert(IntLinkedList_push_tail(&list, 3));

    int v;
    assert(IntLinkedList_peek_head(&list, &v)); assert(v == 1);
    assert(IntLinkedList_peek_tail(&list, &v)); assert(v == 3);
    assert(list.size == 3);

    assert(IntLinkedList_pop_head(&list, &v)); assert(v == 1);
    assert(IntLinkedList_pop_tail(&list, &v)); assert(v == 3);
    assert(list.size == 1);

    assert(IntLinkedList_pop_head(&list, &v)); assert(v == 2);
    assert(IntLinkedList_is_empty(&list));
    assert(!IntLinkedList_pop_head(&list, &v));

    arena_free(&a);
    printf("[OK] LinkedList\n");
}

static void test_stack(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntStack s;
    IntStack_init(&a, &s);
    assert(IntStack_is_empty(&s));

    assert(IntStack_push(&s, 1));
    assert(IntStack_push(&s, 2));
    assert(IntStack_push(&s, 3));

    int v;
    assert(IntStack_peek(&s, &v)); assert(v == 3);

    assert(IntStack_pop(&s, &v)); assert(v == 3);
    assert(IntStack_pop(&s, &v)); assert(v == 2);
    assert(IntStack_pop(&s, &v)); assert(v == 1);
    assert(IntStack_is_empty(&s));
    assert(!IntStack_pop(&s, &v));

    arena_free(&a);
    printf("[OK] Stack\n");
}

static void test_queue(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntQueue q;
    IntQueue_init(&a, &q);
    assert(IntQueue_is_empty(&q));

    assert(IntQueue_enqueue(&q, 1));
    assert(IntQueue_enqueue(&q, 2));
    assert(IntQueue_enqueue(&q, 3));

    int v;
    assert(IntQueue_peek(&q, &v)); assert(v == 1);

    assert(IntQueue_dequeue(&q, &v)); assert(v == 1);
    assert(IntQueue_dequeue(&q, &v)); assert(v == 2);
    assert(IntQueue_dequeue(&q, &v)); assert(v == 3);
    assert(IntQueue_is_empty(&q));
    assert(!IntQueue_dequeue(&q, &v));

    arena_free(&a);
    printf("[OK] Queue\n");
}

static void test_heap(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntHeap h;
    IntHeap_init(&a, &h, 4);
    assert(IntHeap_is_empty(&h));

    int values[] = { 5, 1, 8, 2, 9, 3 };
    for (size_t i = 0; i < 6; i++)
        assert(IntHeap_push(&h, values[i]));

    int prev = -1, v;
    while (IntHeap_pop(&h, &v)) {
        assert(v >= prev);
        prev = v;
    }
    assert(IntHeap_is_empty(&h));

    arena_free(&a);
    printf("[OK] Heap\n");
}

static void test_map(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntMap m;
    IntMap_init(&a, &m, 8);
    assert(IntMap_is_empty(&m));

    assert(IntMap_insert(&m, 1, 100));
    assert(IntMap_insert(&m, 2, 200));
    assert(IntMap_insert(&m, 3, 300));

    int out;
    assert(IntMap_get(&m, 2, &out)); assert(out == 200);
    assert(!IntMap_get(&m, 999, &out));

    assert(IntMap_insert(&m, 2, 999));
    assert(IntMap_get(&m, 2, &out)); assert(out == 999);

    assert(IntMap_delete(&m, 2));
    assert(!IntMap_get(&m, 2, &out));
    assert(!IntMap_delete(&m, 2));

    for (int i = 100; i < 200; i++)
        assert(IntMap_insert(&m, i, i * 2));
    for (int i = 100; i < 200; i++) {
        assert(IntMap_get(&m, i, &out));
        assert(out == i * 2);
    }

    arena_free(&a);
    printf("[OK] Map\n");
}

static void test_set(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntSet s;
    IntSet_init(&a, &s, 8);
    assert(IntSet_is_empty(&s));

    assert(IntSet_insert(&s, 1));
    assert(IntSet_insert(&s, 2));
    assert(IntSet_contains(&s, 1));
    assert(!IntSet_contains(&s, 999));

    assert(IntSet_remove(&s, 1));
    assert(!IntSet_contains(&s, 1));
    assert(IntSet_contains(&s, 2));

    arena_free(&a);
    printf("[OK] Set\n");
}

static void test_tree_map(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntTreeMap t;
    IntTreeMap_init(&a, &t);
    assert(IntTreeMap_is_empty(&t));

    int keys[] = { 50, 30, 70, 20, 40, 60, 80, 10 };
    for (size_t i = 0; i < 8; i++)
        assert(IntTreeMap_insert(&t, keys[i], keys[i] * 10));

    int out;
    assert(IntTreeMap_find(&t, 40, &out)); assert(out == 400);
    assert(!IntTreeMap_find(&t, 999, &out));

    assert(!IntTreeMap_insert(&t, 40, 111));

    int minK, minV, maxK, maxV;
    assert(IntTreeMap_min(&t, &minK, &minV)); assert(minK == 10);
    assert(IntTreeMap_max(&t, &maxK, &maxV)); assert(maxK == 80);

    assert(IntTreeMap_remove(&t, 40));
    assert(!IntTreeMap_find(&t, 40, &out));
    assert(!IntTreeMap_remove(&t, 40));

    arena_free(&a);
    printf("[OK] TreeMap\n");
}

static void test_tree_set(void) {
    sArena a;
    assert(arena_init(&a, MB(1)));

    IntTreeSet s;
    IntTreeSet_init(&a, &s);
    assert(IntTreeSet_is_empty(&s));

    int keys[] = { 5, 3, 8, 1, 4 };
    for (size_t i = 0; i < 5; i++)
        assert(IntTreeSet_insert(&s, keys[i]));

    assert(IntTreeSet_contains(&s, 4));
    assert(!IntTreeSet_contains(&s, 999));

    int minK, maxK;
    assert(IntTreeSet_min(&s, &minK)); assert(minK == 1);
    assert(IntTreeSet_max(&s, &maxK)); assert(maxK == 8);

    assert(IntTreeSet_remove(&s, 3));
    assert(!IntTreeSet_contains(&s, 3));

    arena_free(&a);
    printf("[OK] TreeSet\n");
}

// ═══════════════════════════════════════════════════════════════════════

int main(void) {
    test_arena();
    test_algo();
    test_io();

    test_array();
    test_string();
    test_linked_list();
    test_stack();
    test_queue();
    test_heap();
    test_map();
    test_set();
    test_tree_map();
    test_tree_set();

#ifdef THREAD_SAFE_AGLIB_DS
    printf("\nAll checks passed (thread-safe build).\n");
#else
    printf("\nAll checks passed (default build).\n");
#endif
    return 0;
}

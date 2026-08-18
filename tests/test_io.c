#include "../include/ag.h"
#include "test_framework.h"
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  typedef HANDLE ag_fd_t;
  #define AG_INVALID_FD INVALID_HANDLE_VALUE

  static ag_fd_t ag_open_rw_trunc(const char* path) {
    return CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  }
  static ag_fd_t ag_open_ro(const char* path) {
    return CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  }
  static void ag_close(ag_fd_t fd) { CloseHandle(fd); }
  static bool ag_fd_valid(ag_fd_t fd) { return fd != AG_INVALID_FD; }
#else
  #include <unistd.h>
  #include <fcntl.h>
  typedef int ag_fd_t;
  #define AG_INVALID_FD (-1)

  static ag_fd_t ag_open_rw_trunc(const char* path) {
    return open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
  }
  static ag_fd_t ag_open_ro(const char* path) {
    return open(path, O_RDONLY);
  }
  static void ag_close(ag_fd_t fd) { close(fd); }
  static bool ag_fd_valid(ag_fd_t fd) { return fd >= 0; }
#endif

#define TEST_FILE_A "test_io_a.tmp"
#define TEST_FILE_B "test_io_b.tmp"
#define TEST_FILE_EMPTY "test_io_empty.tmp"
#define TEST_DIR    "test_io_dir.tmp"
#define TEST_DIR_NESTED "test_io_dir.tmp/a/b/c"

static void cleanup_io_artifacts(void) {
  file_delete(TEST_FILE_A);
  file_delete(TEST_FILE_B);
  file_delete(TEST_FILE_EMPTY);
  dir_delete(TEST_DIR, true);
}

static void test_io_basic_rw(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  cleanup_io_artifacts();

  ag_fd_t fd = ag_open_rw_trunc(TEST_FILE_A);
  AG_CHECK(ag_fd_valid(fd));

  AG_CHECK(write_all_file(fd, "hello", 5));
  AG_CHECK_EQ_INT(get_file_size_fd(fd), 5);

  AG_CHECK(append_to_file(fd, " world", 6));
  AG_CHECK_EQ_INT(get_file_size_fd(fd), 11);
  ag_close(fd);

  AG_CHECK_EQ_INT(get_file_size(TEST_FILE_A), 11);

  fd = ag_open_ro(TEST_FILE_A);
  AG_CHECK(ag_fd_valid(fd));
  sFileBuffer buf;
  AG_CHECK(read_all_file(fd, &buf, &alloc));
  AG_CHECK_EQ_INT(buf.size, 11);
  AG_CHECK(memcmp(buf.data, "hello world", 11) == 0);
  ag_close(fd);

  AG_CHECK(path_exists(TEST_FILE_A));
  AG_CHECK(is_file(TEST_FILE_A));
  AG_CHECK(!path_exists("definitely_does_not_exist.tmp"));

  AG_CHECK(file_delete(TEST_FILE_A));
  AG_CHECK(!path_exists(TEST_FILE_A));

  arena_free(&a);
}

static void test_io_empty_file(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  cleanup_io_artifacts();

  ag_fd_t fd = ag_open_rw_trunc(TEST_FILE_EMPTY);
  AG_CHECK(ag_fd_valid(fd));
  ag_close(fd);

  AG_CHECK(path_exists(TEST_FILE_EMPTY));
  AG_CHECK_EQ_INT(get_file_size(TEST_FILE_EMPTY), 0);

  // Writing zero bytes should fail cleanly rather than corrupt anything.
  fd = ag_open_rw_trunc(TEST_FILE_EMPTY);
  AG_CHECK(!write_all_file(fd, "x", 0));
  ag_close(fd);

  AG_CHECK(file_delete(TEST_FILE_EMPTY));
  (void)alloc;
  arena_free(&a);
}

static void test_io_copy(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  cleanup_io_artifacts();

  ag_fd_t fd = ag_open_rw_trunc(TEST_FILE_A);
  AG_CHECK(write_all_file(fd, "hello world", 11));
  ag_close(fd);

  AG_CHECK(file_copy(TEST_FILE_A, TEST_FILE_B));
  AG_CHECK_EQ_INT(get_file_size(TEST_FILE_B), 11);

  fd = ag_open_ro(TEST_FILE_B);
  sFileBuffer buf2;
  AG_CHECK(read_all_file(fd, &buf2, &alloc));
  AG_CHECK(memcmp(buf2.data, "hello world", 11) == 0);
  ag_close(fd);

  // Copying a file onto itself must be rejected.
  AG_CHECK(!file_copy(TEST_FILE_A, TEST_FILE_A));
  AG_CHECK_EQ_INT(get_file_size(TEST_FILE_B), 11);

  file_delete(TEST_FILE_A);
  file_delete(TEST_FILE_B);
  arena_free(&a);
}

static void test_io_path_join(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  sPath joined;
  const char* parts[] = { "foo", "bar", "baz.txt" };
  AG_CHECK(path_join(parts, 3, &joined, &alloc));
  AG_CHECK(strstr(joined.path, "foo") != NULL);
  AG_CHECK(strstr(joined.path, "bar") != NULL);
  AG_CHECK(strstr(joined.path, "baz.txt") != NULL);

  // Empty parts should be skipped, not produce doubled separators or crash.
  sPath joined2;
  const char* partsWithEmpty[] = { "foo", "", "bar" };
  AG_CHECK(path_join(partsWithEmpty, 3, &joined2, &alloc));

  // Invalid args
  AG_CHECK(!path_join(NULL, 0, &joined, &alloc));
  AG_CHECK(!path_join(parts, 0, &joined, &alloc));

  arena_free(&a);
}

static void test_io_directories(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);
  
  cleanup_io_artifacts();
  AG_CHECK(dir_create(TEST_DIR, true));
  AG_CHECK(is_dir(TEST_DIR));

  // Creating an already-existing dir should succeed (idempotent), not fail.
  AG_CHECK(dir_create(TEST_DIR, true));

  // Deeply nested recursive creation.
  AG_CHECK(dir_create(TEST_DIR_NESTED, true));
  AG_CHECK(is_dir(TEST_DIR_NESTED));

  sPath nestedFile;
  const char* nestedParts[] = { TEST_DIR, "inner.txt" };
  AG_CHECK(path_join(nestedParts, 2, &nestedFile, &alloc));

  ag_fd_t nfd = ag_open_rw_trunc(nestedFile.path);
  AG_CHECK(ag_fd_valid(nfd));
  AG_CHECK(write_all_file(nfd, "x", 1));
  ag_close(nfd);

  sDirList list;
  AG_CHECK(dir_list(TEST_DIR, &list, &alloc));
  // TEST_DIR now contains "inner.txt" and the "a" subdirectory.
  AG_CHECK(list.count == 2);

  AG_CHECK(dir_delete(TEST_DIR, true));
  AG_CHECK(!path_exists(TEST_DIR));

  // Deleting something that no longer exists should fail cleanly.
  AG_CHECK(!dir_delete(TEST_DIR, true));

  arena_free(&a);
}

typedef struct {
  int fileCount;
  int dirCount;
} sWalkCounts;

static bool count_walk_cb(const char* path, bool isDir, void* userdata) {
  (void)path;
  sWalkCounts* counts = (sWalkCounts*)userdata;
  if (isDir) counts->dirCount++;
  else       counts->fileCount++;
  return true; // keep walking
}

static void test_io_dir_walk(void) {
  sArena a;
  AG_CHECK(arena_init(&a, MB(1)));
  sAllocator alloc = use_arena(&a);

  cleanup_io_artifacts();
  AG_CHECK(dir_create(TEST_DIR_NESTED, true));

  sPath f1, f2;
  const char* p1[] = { TEST_DIR, "root_file.txt" };
  const char* p2[] = { TEST_DIR_NESTED, "nested_file.txt" };
  AG_CHECK(path_join(p1, 2, &f1, &alloc));
  AG_CHECK(path_join(p2, 2, &f2, &alloc));

  ag_fd_t fd1 = ag_open_rw_trunc(f1.path); AG_CHECK(write_all_file(fd1, "a", 1)); ag_close(fd1);
  ag_fd_t fd2 = ag_open_rw_trunc(f2.path); AG_CHECK(write_all_file(fd2, "b", 1)); ag_close(fd2);

  sWalkCounts counts = {0, 0};
  AG_CHECK(dir_walk(TEST_DIR, true, count_walk_cb, &counts, &alloc));

  AG_CHECK(counts.fileCount == 2);
  AG_CHECK(counts.dirCount == 3); // a, a/b, a/b/c

  dir_delete(TEST_DIR, true);
  arena_free(&a);
}

void run_io_tests(void) {
  test_io_basic_rw();
  test_io_empty_file();
  test_io_copy();
  test_io_path_join();
  test_io_directories();
  test_io_dir_walk();
  cleanup_io_artifacts();
}

#include "../include/ag.h"
#include "test_framework.h"
#include <string.h>

// The I/O helpers that take an sAllocator* (read_all_file, path_join,
// dir_list, dir_walk) never reject a slab-backed allocator - they just
// pass every request straight through to ag_alloc. That works fine as
// long as every individual allocation they make (a file's contents, a
// joined path, a directory-entry array, ...) fits inside one slab block.
// This makes a slab a poor fit for arbitrarily large files, but a solid
// one for small, bounded I/O work - both are exercised below.

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

#define TEST_FILE_A     "test_io_slab_a.tmp"
#define TEST_FILE_BIG   "test_io_slab_big.tmp"
#define TEST_DIR        "test_io_slab_dir.tmp"
#define TEST_DIR_NESTED "test_io_slab_dir.tmp/a/b"

#define SLAB_BLOCK_SIZE KB(4)

static void cleanup_io_slab_artifacts(void) {
  file_delete(TEST_FILE_A);
  file_delete(TEST_FILE_BIG);
  dir_delete(TEST_DIR, true);
}

static void test_io_slab_basic_rw(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 16));
  sAllocator alloc = use_slab(&s);

  cleanup_io_slab_artifacts();

  ag_fd_t fd = ag_open_rw_trunc(TEST_FILE_A);
  AG_CHECK(ag_fd_valid(fd));
  AG_CHECK(write_all_file(fd, "hello world", 11));
  ag_close(fd);

  fd = ag_open_ro(TEST_FILE_A);
  AG_CHECK(ag_fd_valid(fd));

  sFileBuffer buf;
  AG_CHECK(read_all_file(fd, &buf, &alloc));
  AG_CHECK_EQ_INT(buf.size, 11);
  AG_CHECK(memcmp(buf.data, "hello world", 11) == 0);
  ag_close(fd);

  file_delete(TEST_FILE_A);
  slab_free_all(&s);
}

static void test_io_slab_rejects_file_larger_than_block(void) {
  // A slab can only ever hand back blockSize bytes at a time, so reading
  // a file bigger than that must fail cleanly through ag_alloc rather
  // than silently truncating the read.
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 16));
  sAllocator alloc = use_slab(&s);

  cleanup_io_slab_artifacts();

  ag_fd_t fd = ag_open_rw_trunc(TEST_FILE_BIG);
  AG_CHECK(ag_fd_valid(fd));

  char chunk[512];
  memset(chunk, 'x', sizeof(chunk));
  size_t written = 0;
  while (written < SLAB_BLOCK_SIZE + 1024) {
    AG_CHECK(write_all_file(fd, chunk, sizeof(chunk)));
    written += sizeof(chunk);
  }
  ag_close(fd);

  fd = ag_open_ro(TEST_FILE_BIG);
  AG_CHECK(ag_fd_valid(fd));

  sFileBuffer buf;
  AG_CHECK(!read_all_file(fd, &buf, &alloc));
  ag_close(fd);

  file_delete(TEST_FILE_BIG);
  slab_free_all(&s);
}

static void test_io_slab_path_join(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 16));
  sAllocator alloc = use_slab(&s);

  sPath joined;
  const char* parts[] = { "foo", "bar", "baz.txt" };
  AG_CHECK(path_join(parts, 3, &joined, &alloc));
  AG_CHECK(strstr(joined.path, "foo") != NULL);
  AG_CHECK(strstr(joined.path, "bar") != NULL);
  AG_CHECK(strstr(joined.path, "baz.txt") != NULL);

  sPath joined2;
  const char* partsWithEmpty[] = { "foo", "", "bar" };
  AG_CHECK(path_join(partsWithEmpty, 3, &joined2, &alloc));

  AG_CHECK(!path_join(NULL, 0, &joined, &alloc));
  AG_CHECK(!path_join(parts, 0, &joined, &alloc));

  slab_free_all(&s);
}

static void test_io_slab_directories(void) {
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 16));
  sAllocator alloc = use_slab(&s);

  cleanup_io_slab_artifacts();
  AG_CHECK(dir_create(TEST_DIR, true));
  AG_CHECK(is_dir(TEST_DIR));

  sPath nestedFile;
  const char* nestedParts[] = { TEST_DIR, "inner.txt" };
  AG_CHECK(path_join(nestedParts, 2, &nestedFile, &alloc));

  ag_fd_t nfd = ag_open_rw_trunc(nestedFile.path);
  AG_CHECK(ag_fd_valid(nfd));
  AG_CHECK(write_all_file(nfd, "x", 1));
  ag_close(nfd);

  sDirList list;
  AG_CHECK(dir_list(TEST_DIR, &list, &alloc));
  AG_CHECK(list.count == 1);
  AG_CHECK_STR_EQ(list.entries[0], "inner.txt");

  AG_CHECK(dir_delete(TEST_DIR, true));
  slab_free_all(&s);
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
  return true;
}

static void test_io_slab_dir_walk(void) {
  // dir_walk pushes/pops through an internal Stack(char*, ...) - which is
  // exactly the LinkedList-derived container that now accepts a slab, so
  // this doubles as an integration test for that path through real I/O
  // code rather than a hand-written test container.
  sSlab s;
  AG_CHECK(slab_init(&s, SLAB_BLOCK_SIZE, 16));
  sAllocator alloc = use_slab(&s);

  cleanup_io_slab_artifacts();
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
  AG_CHECK(counts.dirCount == 2); // a, a/b

  dir_delete(TEST_DIR, true);
  slab_free_all(&s);
}

void run_io_slab_tests(void) {
  test_io_slab_basic_rw();
  test_io_slab_rejects_file_larger_than_block();
  test_io_slab_path_join();
  test_io_slab_directories();
  test_io_slab_dir_walk();
  cleanup_io_slab_artifacts();
}

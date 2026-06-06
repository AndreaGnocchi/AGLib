#include "../include/aglib_io.h"
#include "../include/aglib_ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <dirent.h>

#define BUF_SIZE 512

// ——— I/O Func —————————————————————————————————————————————————————————————————————————————————————


bool get_str(const char* prompt, char** str , size_t len , sArena* a) {
  if (!prompt || !str || len == 0 || !a) return false;
  
  char* buffer = (char*)arena_alloc(a, len, false);
  if (!buffer) return false;
  
  printf("%s", prompt);
  fflush(stdout);
  
  if (fgets(buffer, (int)len, stdin)) {
    size_t last = strlen(buffer) - 1;
    if (buffer[last] == '\n') {
      buffer[last] = '\0';
    }
    
    *str = buffer;
    return true;
  }
  
  return false;
}

bool get_char(const char* prompt, char*  val) {
  if (!prompt || !val) return false;
  
  printf("%s", prompt);
  fflush(stdout);
  
  int c = getchar();
  if (c == EOF) return false;
    
  *val = (char)c;
  
  int next;
  while ((next = getchar()) != '\n' && next != EOF);
  
  return true;
}

bool get_opt(const char* prompt, char*  val , size_t nOpt, const char opts[]) {
  if (!prompt || !val || !opts || nOpt == 0) return false;
  
  while (1) {
    printf("%s", prompt);
    fflush(stdout);
    
    int input = getchar();
    if (input == EOF) return false;
    
    int  next;
    bool extraInp = false;
    while ((next = getchar()) != '\n' && next != EOF) {
      if (next != ' ' && next != '\t') extraInp = true;
    }

    if (extraInp) {
      printf("Invalid input. Please enter a single character.\n");
      continue;
    }
    
    for (size_t i = 0; i < nOpt; ++i) {
      if ((char)input == opts[i]) {
        *val = (char)input;
        return true;
      }
    }
    
    printf("Invalid option. Please try again.\n");
  }
}

bool get_int(const char* prompt, int* val) {
  return get_int_range(prompt, val, INT_MIN, INT_MAX);
}

bool get_float(const char* prompt, float* val) {
  return get_float_range(prompt, val, -FLT_MAX, FLT_MAX);
}


bool get_int_range(const char* prompt, int* val , int min , int max) {
  if (!prompt || !val || max < min) return false;

  while (1) {
    printf("%s", prompt);
    fflush(stdout);
    
    char buf[BUF_SIZE];
    if (!(fgets(buf, sizeof(buf), stdin))) return false;
    buf[strcspn(buf, "\n")] = '\0';
    
    if (buf[0] == '\0') {
      printf("Input cannot be empty.\n");
      continue;
    }
    
    
    errno       = 0;
    char* end;
    long  value = strtol(buf, &end, 10);
    if (end == buf || *end != '\0' || errno == ERANGE || value < min || value > max) {
      printf("Please enter a number between %d and %d.\n", min, max);
      continue;
    }

    *val = (int)value;
    return true;
  }
}

bool get_float_range(const char* prompt, float* val , float min , float max) {
  if (!prompt || !val || max < min) return false;

  while (1) {
    printf("%s", prompt);
    fflush(stdout);
    
    char buf[BUF_SIZE];
    if (!(fgets(buf, sizeof(buf), stdin))) return false;
    buf[strcspn(buf, "\n")] = '\0';
    
    if (buf[0] == '\0') {
      printf("Input cannot be empty.\n");
      continue;
    }
    
    
    errno       = 0;
    char* end;
    float value = strtof(buf, &end);
    if (end == buf || *end != '\0' || errno == ERANGE || value < min || value > max || isnan(value)) {
      printf("Please enter a number between %.2f and %.2f.\n", min, max);
      continue;
    }

    *val = value;
    return true;
  }
}

// ——— File —————————————————————————————————————————————————————————————————————————————————————————

bool read_all_file(int fd, sFileBuffer* out , sArena* a) {
  if (fd < 0 || !out || !a) return false;
  
  off_t size = get_file_size_fd(fd);
  if (size < 0) return false;
  
  char* buf = (char*)arena_alloc(a, size + 1, false);
  if (!buf) return false;

  if (lseek(fd, 0, SEEK_SET) == (off_t)-1) return false;

  if (read(fd, buf, size) != size) return false;
  buf[size] = '\0';

  if (!(find_path(fd, &out->path, a ))) return false;
  out->data = buf;
  out->size = size;
  return true;
}

bool write_all_file(int fd, const char* data, size_t  len) {
  if (fd < 0 || !data || len == 0) return false;

  size_t totWritten = 0;
  while (totWritten < len) {
    ssize_t written = write(fd, data + totWritten, len - totWritten);
    if (written < 0) {
      if (errno == EINTR) continue;
      return false;
    }

    if (written == 0) return false;

    totWritten += (size_t)written;
  }

  return true;
}

bool append_to_file(int fd, const char* data, size_t len) {
  if (fd < 0 || !data || len == 0) return false;

  if (lseek(fd, 0, SEEK_END) == (off_t)-1) return false;

  return write_all_file(fd, data, len);
}

off_t get_file_size_fd(int fd) {
  if (fd < 0) return -1;
  
  struct stat st;
  if (fstat(fd, &st) == -1) return -1;

  return st.st_size; 
}

off_t get_file_size(const char* path) {
  if (!path) return -1;

  int fd = open(path, O_RDONLY);
  if (fd == -1) return -1;

  struct stat st;
  if (fstat(fd, &st) == -1) return -1;
  off_t size = st.st_size;

  close(fd);
  return size; 
}

bool path_exists(const char *path) {
  if (!path) return false;

  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

bool is_file(const char *path) {
  if (!path) return false;
  
  struct stat buffer;
  if (stat(path, &buffer) != 0) return false;
  
  return S_ISREG(buffer.st_mode);
}

bool file_copy(const char* src, const char* dst) {
  if (!src || !dst) return false;

  int srcFd = open(src, O_RDONLY);
  if (srcFd < 0) return false;
  
  off_t size = get_file_size_fd(srcFd);
  if (size < 0) { close(srcFd); return false; }
  
  int dstFd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dstFd < 0) { close(srcFd); return false; }
  
  off_t offset    = 0;
  off_t remaining = size;
  bool  ok        = true;
  
  while (remaining > 0) {
    ssize_t sent = sendfile(dstFd, srcFd, &offset, (size_t)remaining);
    if (sent < 0) {
      if (errno == EINTR) continue;
      ok = false;
      break;
    }
    if (sent == 0) { ok = false; break; }
    remaining -= sent;
  }
  
  close(srcFd);
  close(dstFd);
  return ok;
}

bool file_delete(const char* path) {
  if (!path) return false;
  if (unlink(path) == 0) return true;
  return false;
}

bool find_path(int fd, sPath* out, sArena* a) {
  char procPath[64];
  snprintf(procPath, sizeof(procPath), "/proc/self/fd/%d", fd);

  char buffer[PATH_MAX];
  ssize_t len = readlink(procPath, buffer, sizeof(buffer) - 1);

  if (len == -1) return false;
  buffer[len] = '\0';

  out->path = (char*)arena_alloc(a, len + 1, false);
  if (!out->path) return false;
  
  memcpy(out->path, buffer, len + 1);
  out->pathLen = (size_t)len;
  
  return true;
}

bool path_join(const char* parts[], size_t n, sPath* out, sArena* a) {
  if (!parts || n == 0 || !out || !a) return false;

  size_t totLen = 0;
  for (size_t i = 0; i < n; ++i)
    if (parts[i])
      totLen += strlen(parts[i]) + 1;

  char* buf = (char*)arena_alloc(a, totLen, false);
  if (!buf) return false;

  char* ptr = buf;
  for (size_t i = 0; i < n; ++i) {
    if (!parts[i] || strlen(parts[i]) == 0) continue;

    size_t len = strlen(parts[i]);
    memcpy(ptr, parts[i], len);
    ptr += len;

    if (i < n - 1 && *(ptr - 1) != '/') {
      *ptr++ = '/';
    }
  }

  *ptr = '\0';

  out->path    = buf;
  out->pathLen = (size_t)(ptr - buf);
  return true;
}

// ——— Directory ———————————————————————————————————————————————————————————————————————————————————

bool is_dir(const char* path) {
  if (!path) return false;
  
  struct stat buffer;
  if (stat(path, &buffer) != 0) return false;
  
  return S_ISDIR(buffer.st_mode);
}

bool dir_create(const char* path, bool recursive) {
  if (!path) return false;

  if (!recursive)
    return (mkdir(path, 0777) == 0 || errno == EEXIST);

  char tmp[PATH_MAX];
  strncpy(tmp, path, sizeof(tmp) - 1);

  char* ptr = tmp;

  if (*ptr == '/') ptr++;

  while ((ptr = strchr(ptr, '/')) != NULL) {
    *ptr = '\0';
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST) return false;
    *ptr = '/';
    ptr++;
  }

  return (mkdir(tmp, 0777) == 0 || errno == EEXIST);
}

bool dir_delete(const char* path , bool recursive) {
  if (!path) return false;

  if (!recursive)
    return rmdir(path) == 0;

  DIR* d = opendir(path);
  if (!d) return false;

  struct dirent* p;
  bool   success = true;
  

  while ((p = readdir(d))) {
    if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) continue;

    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s/%s", path, p->d_name);

    if (is_dir(buf)) {
      if (!dir_delete(buf, true)) {
        success = false;
        break;
      }
    } else {
      if (unlink(buf) != 0) {
        success = false;
        break;
      }
    }
  }
  closedir(d);

  return success && (rmdir(path) == 0);
}

bool dir_list (const char* path , sDirList* out, sArena* a) {
  if (!out || !a || !path) return false;

  DIR* dir = opendir(path);
  if (!dir) return false;
  struct dirent* entry;
  size_t capacity = 16;
  out->count = 0;
  out->entries = (char**)arena_alloc(a, sizeof(char*) * capacity, false);

  while ((entry = readdir(dir))) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

    if (out->count >= capacity) {
      capacity *= 2;
      char** newEntries = (char**)arena_alloc(a, sizeof(char*) * capacity, false);
      memcpy(newEntries, out->entries, sizeof(char*) * out->count);
      out->entries = newEntries;
    }

    size_t len     = strlen(entry->d_name);
    char* nameCopy = (char*)arena_alloc(a, len + 1, false);
    memcpy(nameCopy, entry->d_name, len + 1);
    
    out->entries[out->count++] = nameCopy;
  }

  closedir(dir);
  return true;
}

Stack(char*, PathStack)

bool dir_walk  (const char* root, bool recursive, WalkCallback cb, void* userdata, sArena* a) {
  if (!root || !cb || !a) return false;

  PathStack stack;
  PathStack_init(a, &stack);

  size_t rootLen  = strlen(root);
  char*  rootCopy = (char*)arena_alloc(a, rootLen + 1, false);
  if (!rootCopy) return false;
  memcpy(rootCopy, root, rootLen + 1);
  
  while (rootLen > 1 && rootCopy[rootLen - 1] == '/') 
        rootCopy[--rootLen] = '\0';

  if (!PathStack_push(&stack, rootCopy)) return false;

  char* current;
  while (PathStack_pop(&stack, &current)) {
    DIR* d = opendir(current);
    if (!d) return false;
    
    struct dirent* entry;
    while ((entry = readdir(d))) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
      
      size_t curLen  = strlen(current);
      size_t nameLen = strlen(entry->d_name);
      size_t fullLen = curLen + 1 + nameLen;

      char* fullPath = (char*)arena_alloc(a, fullLen + 1, false);
      if (!fullPath) { closedir(d); return false; }
      
      snprintf(fullPath, fullLen + 1, "%s/%s", current, entry->d_name);

      bool isDir = is_dir(fullPath);

      if (!cb(fullPath, isDir, userdata)) { closedir(d); return false; }
      
      if (recursive && isDir) {
        if (!PathStack_push(&stack, fullPath)) { closedir(d); return false; }
      }
    }
    
    closedir(d);
  }

  return true;
}

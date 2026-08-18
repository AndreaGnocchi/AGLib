#ifdef _WIN32

#include "../include/aglib_io_win.h"
#include "../include/aglib_ds.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ——— File —————————————————————————————————————————————————————————————————————————————————————————

bool file_copy(const char* src, const char* dst) {
  if (!src || !dst) return false;
  if (strcmp(src, dst) == 0) return false;
  
  return CopyFileA(src, dst, FALSE) != 0;
}

bool find_path(HANDLE hFile, sPath* out, sAllocator* a) {
  if (hFile == INVALID_HANDLE_VALUE || !out || !a) return false;

  DWORD len = GetFinalPathNameByHandleA(hFile, NULL, 0, FILE_NAME_NORMALIZED);
  if (len == 0) return false;

  DWORD bufLen = len + 1;
  out->path = (char*)ag_alloc(a, bufLen, false);
  if (!out->path) return false;
 
  DWORD res = GetFinalPathNameByHandleA(hFile, out->path, bufLen, FILE_NAME_NORMALIZED);
  if (res == 0 || res >= bufLen) return false;
 
  if (strncmp(out->path, "\\\\?\\", 4) == 0) {
      memmove(out->path, out->path + 4, res - 3);
      res -= 4;
  }
 
  out->pathLen = res;
  return true;
}

bool read_all_file(HANDLE hFile, sFileBuffer* out, sAllocator* a) {
  if (hFile == INVALID_HANDLE_VALUE || !out || !a) return false;
  
  int64_t size = get_file_size_fd(hFile);
  if (size < 0) return false;
  
  void* buf = (void*)ag_alloc(a, (size_t)size + 1, false);
  if (!buf) return false;

  LARGE_INTEGER zero = {0};
  if (!SetFilePointerEx(hFile, zero, NULL, FILE_BEGIN)) return false;

  DWORD bytesRead;
  if (!ReadFile(hFile, buf, (DWORD)size, &bytesRead, NULL) || bytesRead != size) return false;
  
  ((char*)buf)[size] = '\0';

  if (!(find_path(hFile, &out->path, a))) return false;
  out->data = buf;
  out->size = (size_t)size;
  return true;
}

bool write_all_file(HANDLE hFile, const void* data, size_t len) {
  if (hFile == INVALID_HANDLE_VALUE || !data || len == 0) return false;

  size_t totWritten = 0;
  while (totWritten < len) {
    DWORD written = 0;
    DWORD toWrite = (DWORD)(len - totWritten);
    
    if (!WriteFile(hFile, (const char*)data + totWritten, toWrite, &written, NULL)) {
      return false;
    }
    if (written == 0) return false;
    totWritten += written;
  }

  return true;
}

bool append_to_file(HANDLE hFile, const void* data, size_t len) {
  if (hFile == INVALID_HANDLE_VALUE || !data || len == 0) return false;

  LARGE_INTEGER zero = {0};
  if (!SetFilePointerEx(hFile, zero, NULL, FILE_END)) return false;

  return write_all_file(hFile, data, len);
}

int64_t get_file_size_fd(HANDLE hFile) {
  if (hFile == INVALID_HANDLE_VALUE) return -1;
  
  LARGE_INTEGER size;
  if (!GetFileSizeEx(hFile, &size)) return -1;

  return size.QuadPart; 
}

int64_t get_file_size(const char* path) {
  if (!path) return -1;

  WIN32_FILE_ATTRIBUTE_DATA st;
  if (!GetFileAttributesExA(path, GetFileExInfoStandard, &st)) return -1;

  LARGE_INTEGER size;
  size.HighPart = st.nFileSizeHigh;
  size.LowPart  = st.nFileSizeLow;
  return size.QuadPart;
}

bool path_exists(const char* path) {
  if (!path) return false;
  return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool is_file(const char* path) {
  if (!path) return false;
  
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) return false;
  
  return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool file_delete(const char* path) {
  if (!path) return false;
  return DeleteFileA(path) != 0;
}

bool path_join(const char* parts[], size_t n, sPath* out, sAllocator* a) {
  if (!parts || n == 0 || !out || !a) return false;

  size_t totLen = 0;
  for (size_t i = 0; i < n; ++i)
    if (parts[i])
      totLen += strlen(parts[i]) + 1;

  char* buf = (char*)ag_alloc(a, totLen, false);
  if (!buf) return false;

  char* ptr = buf;
  for (size_t i = 0; i < n; ++i) {
    if (!parts[i] || strlen(parts[i]) == 0) continue;

    size_t len = strlen(parts[i]);
    memcpy(ptr, parts[i], len);
    ptr += len;

    if (i < n - 1 && *(ptr - 1) != '/' && *(ptr - 1) != '\\') {
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
  
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) return false;
  
  return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool dir_create(const char* path, bool recursive) {
  if (!path) return false;

  if (!recursive) {
    return CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
  }

  char tmp[MAX_PATH];
  strncpy(tmp, path, sizeof(tmp) - 1);
  tmp[MAX_PATH - 1] = '\0';

  char* ptr = tmp;
  if (*ptr == '/' || *ptr == '\\') ptr++;
  if (strlen(tmp) >= 3 && tmp[1] == ':') ptr += 3;

  char* slash;
  while ((slash = strchr(ptr, '/')) != NULL || (slash = strchr(ptr, '\\')) != NULL) {
    char sep = *slash;
    *slash = '\0';
    if (!CreateDirectoryA(tmp, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
      return false;
    }
    *slash = sep;
    ptr = slash + 1;
  }
  
  return CreateDirectoryA(tmp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool dir_delete(const char* path , bool recursive) {
  if (!path) return false;

  if (!recursive)
    return RemoveDirectoryA(path) != 0;

  char searchPath[MAX_PATH];
  snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

  WIN32_FIND_DATAA fd;
  HANDLE hFind = FindFirstFileA(searchPath, &fd);
  if (hFind == INVALID_HANDLE_VALUE) return false;

  bool success = true;

  do {
    if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;

    char buf[MAX_PATH];
    snprintf(buf, sizeof(buf), "%s\\%s", path, fd.cFileName);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (!dir_delete(buf, true)) {
        success = false;
        break;
      }
    } else {
      if (!DeleteFileA(buf)) {
        success = false;
        break;
      }
    }
  } while (FindNextFileA(hFind, &fd));

  FindClose(hFind);

  return success && (RemoveDirectoryA(path) != 0);
}

bool dir_list(const char* path, sDirList* out, sAllocator* a) {
  if (!out || !a || !path) return false;

  char searchPath[MAX_PATH];
  snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

  WIN32_FIND_DATAA fd;
  HANDLE hFind = FindFirstFileA(searchPath, &fd);
  if (hFind == INVALID_HANDLE_VALUE) return false;

  size_t capacity = 16;
  out->count = 0;
  out->entries = (char**)ag_alloc(a, sizeof(char*) * capacity, false);
  if (!out->entries) { FindClose(hFind); return false; }

  do {
    if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

    if (out->count >= capacity) {
      size_t oldCapacity = capacity;
      capacity *= 2;
      char** newEntries = (char**)ag_realloc(a, out->entries,
        sizeof(char*) * oldCapacity, sizeof(char*) * capacity, false);
      if (!newEntries) { FindClose(hFind); return false; }
      out->entries = newEntries;
    }

    size_t len = strlen(fd.cFileName);
    char* nameCopy = (char*)ag_alloc(a, len + 1, false);
    if (!nameCopy) { FindClose(hFind); return false; }
    memcpy(nameCopy, fd.cFileName, len + 1);
    
    out->entries[out->count++] = nameCopy;
  } while (FindNextFileA(hFind, &fd));

  FindClose(hFind);
  return true;
}

Stack(char*, PathStack)

bool dir_walk(const char* root, bool recursive, WalkCallback cb, void* userdata, sAllocator* a) {
  if (!root || !cb || !a) return false;

  PathStack stack;
  if (!PathStack_init(a, &stack)) return false;

  size_t rootLen  = strlen(root);
  char*  rootCopy = (char*)ag_alloc(a, rootLen + 1, false);
  if (!rootCopy) return false;
  memcpy(rootCopy, root, rootLen + 1);
  
  while (rootLen > 1 && (rootCopy[rootLen - 1] == '/' || rootCopy[rootLen - 1] == '\\')) 
        rootCopy[--rootLen] = '\0';

  if (!PathStack_push(&stack, rootCopy)) return false;

  char* current;
  while (PathStack_pop(&stack, &current)) {
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", current);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return false;
    
    do {
      if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
      
      size_t curLen  = strlen(current);
      size_t nameLen = strlen(fd.cFileName);
      size_t fullLen = curLen + 1 + nameLen;

      char* fullPath = (char*)ag_alloc(a, fullLen + 1, false);
      if (!fullPath) { FindClose(hFind); return false; }
      
      snprintf(fullPath, fullLen + 1, "%s\\%s", current, fd.cFileName);

      bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

      if (!cb(fullPath, isDir, userdata)) { FindClose(hFind); return false; }
      
      if (recursive && isDir) {
        if (!PathStack_push(&stack, fullPath)) { FindClose(hFind); return false; }
      }
    } while (FindNextFileA(hFind, &fd));
    
    FindClose(hFind);
  }

  return true;
}

#endif // _WIN32

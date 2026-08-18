#ifdef __APPLE__

#include "../include/aglib_io_posix.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <copyfile.h>

bool file_copy(const char* src, const char* dst) {
  if (!src || !dst)          return false;
  if (strcmp(src, dst) == 0) return false;
  
  if (copyfile(src, dst, NULL, COPYFILE_ALL) < 0) return false;
  
  return true;
}

bool find_path(int fd, sPath* out, sAllocator* a) {
  char buffer[MAXPATHLEN];
  
  if (fcntl(fd, F_GETPATH, buffer) == -1) return false;
  
  size_t len = strlen(buffer);
  out->path = (char*)ag_alloc(a, len + 1, false);
  if (!out->path) return false;
  
  memcpy(out->path, buffer, len + 1);
  out->pathLen = len;
  
  return true;
}

#endif // __APPLE__

#ifdef __linux__

#include "../include/aglib_io_posix.h"
#include <sys/sendfile.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

bool file_copy(const char* src, const char* dst) {
  if (!src || !dst)          return false;
  if (strcmp(src, dst) == 0) return false;

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

bool find_path(int fd, sPath* out, sAllocator* a) {
  char procPath[64];
  snprintf(procPath, sizeof(procPath), "/proc/self/fd/%d", fd);

  char buffer[PATH_MAX];
  ssize_t len = readlink(procPath, buffer, sizeof(buffer) - 1);

  if (len == -1) return false;
  buffer[len] = '\0';

  out->path = (char*)ag_alloc(a, len + 1, false);
  if (!out->path) return false;
  
  memcpy(out->path, buffer, len + 1);
  out->pathLen = (size_t)len;
  
  return true;
}

#endif //  __linux__

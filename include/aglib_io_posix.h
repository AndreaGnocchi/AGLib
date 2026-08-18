#ifndef AG_LIB_IO_POSIX
#define AG_LIB_IO_POSIX

#include <sys/types.h>
#include "aglib_io.h"

// ——— File ——————————————————————————————————————————————————————————————————

bool  read_all_file   (int         fd      , sFileBuffer* out , sAllocator* a  );
bool  write_all_file  (int         fd      , const void*  data, size_t  len);
bool  append_to_file  (int         fd      , const void*  data, size_t  len);
off_t get_file_size_fd(int         fd     );
off_t get_file_size   (const char* path   );
bool  find_path       (int         fd      , sPath*       out , sAllocator* a);

#endif // AG_LIB_IO_POSIX

#ifndef AG_LIB_IO_WIN
#define AG_LIB_IO_WIN

#include <windows.h>
#include <stdint.h>
#include "aglib_io.h"

// ——— File ——————————————————————————————————————————————————————————————————

bool    read_all_file    (HANDLE       hFile   , sFileBuffer* out  , sAllocator* a  );
bool    write_all_file   (HANDLE       hFile   , const void*  data , size_t      len);
bool    append_to_file   (HANDLE       hFile   , const void*  data , size_t      len);
int64_t get_file_size_fd (HANDLE       hFile  );
int64_t get_file_size    (const char*  path   );
bool    find_path        (HANDLE       hFile   , sPath*       out  , sAllocator* a  );

#endif // AG_LIB_IO_WIN

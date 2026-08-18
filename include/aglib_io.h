#ifndef AG_LIB_IO
#define AG_LIB_IO

#include <stdbool.h>
#include <stddef.h> 
#include "../include/aglib_allocator.h"

// ——— Stdin Func ———————————————————————————————————————————————————————————————————————————————————

bool get_str        (const char* prompt, char** str , size_t len , sAllocator* a     );
bool get_char       (const char* prompt, char*  val);
bool get_opt        (const char* prompt, char*  val , size_t nOpt, const char opts[]);
bool get_int        (const char* prompt, int*   val);
bool get_float      (const char* prompt, float* val);
bool get_int_range  (const char* prompt, int*   val , int    min , int        max   );
bool get_float_range(const char* prompt, float* val , float  min , float      max   );

// ——— File ——————————————————————————————————————————————————————————————————

typedef struct {
  char*  path;
  size_t pathLen;
} sPath;

typedef struct {
  void*  data;
  size_t size;
  sPath  path;
} sFileBuffer;

bool  path_exists     (const char* path   );
bool  is_file         (const char* path   );
bool  file_copy       (const char* src     , const char*  dst);
bool  file_delete     (const char* path   );
bool  path_join       (const char*  parts[], size_t       n   , sPath*      out, sAllocator* a);

// ——— Directory ——————————————————————————————————————————————————————————————

typedef bool (*WalkCallback)(const char* path, bool isDir, void* userdata);

typedef struct {
  char** entries;
  size_t count;
} sDirList;

bool is_dir    (const char* path);
bool dir_create(const char* path, bool recursive);
bool dir_delete(const char* path, bool recursive);
bool dir_list  (const char* path, sDirList* out, sAllocator* a);
bool dir_walk  (const char* root, bool recursive, WalkCallback cb, void* userdata, sAllocator* a);

#endif // AG_LIB_IO

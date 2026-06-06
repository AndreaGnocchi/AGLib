#ifndef AG_LIB_IO
#define AG_LIB_IO

#include <stdbool.h>
#include <sys/types.h>
#include <stddef.h> 
#include "../include/aglib_arena.h"

// ——— I/O Func ———————————————————————————————————————————————————————————————————————————————————

bool get_str        (const char* prompt, char** str , size_t len , sArena*    a     );
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
  char*  data;
  size_t size;
  sPath  path;
} sFileBuffer;

bool  read_all_file   (int         fd      , sFileBuffer* out , sArena* a  );
bool  write_all_file  (int         fd      , const char*  data, size_t  len);
bool  append_to_file  (int         fd      , const char*  data, size_t  len);
off_t get_file_size_fd(int         fd     );
off_t get_file_size   (const char* path   );
bool  path_exists     (const char* path   );
bool  is_file         (const char* path   );
bool  file_copy       (const char* src     , const char*  dst);
bool  file_delete     (const char* path   );
bool  find_path       (int         fd      , sPath*       out , sArena* a);
bool  path_join       (const char* parts[] , size_t       n   , sPath*  out , sArena* a);

// ——— Directory ——————————————————————————————————————————————————————————————

typedef bool (*WalkCallback)(const char* path, bool isDir, void* userdata);

typedef struct {
  char** entries;
  size_t count;
} sDirList;

bool is_dir    (const char* path);
bool dir_create(const char* path , bool      recursive);
bool dir_delete(const char* path , bool      recursive);
bool dir_list  (const char* path , sDirList* out       , sArena* a);
bool dir_walk  (const char* root, bool recursive, WalkCallback cb, void* userdata, sArena* a);

#endif // AG_LIB_IO

#ifndef AG_LIB_DS
#define AG_LIB_DS

#ifndef THREAD_SAFE_AGLIB_DS

#include "aglib_ds/array.h"
#include "aglib_ds/string.h"
#include "aglib_ds/linked_list.h"
#include "aglib_ds/stack.h"
#include "aglib_ds/queue.h"
#include "aglib_ds/heap.h"
#include "aglib_ds/map.h"
#include "aglib_ds/set.h"
#include "aglib_ds/tree_map.h"
#include "aglib_ds/tree_set.h"


#else

#ifndef _WIN32

#include "aglib_ds_safe_posix/array.h"
#include "aglib_ds_safe_posix/string.h"
#include "aglib_ds_safe_posix/linked_list.h"
#include "aglib_ds_safe_posix/stack.h"
#include "aglib_ds_safe_posix/queue.h"
#include "aglib_ds_safe_posix/heap.h"
#include "aglib_ds_safe_posix/map.h"
#include "aglib_ds_safe_posix/set.h"
#include "aglib_ds_safe_posix/tree_map.h"
#include "aglib_ds_safe_posix/tree_set.h"

#else

#include "aglib_ds_safe_win/array.h"
#include "aglib_ds_safe_win/string.h"
#include "aglib_ds_safe_win/linked_list.h"
#include "aglib_ds_safe_win/stack.h"
#include "aglib_ds_safe_win/queue.h"
#include "aglib_ds_safe_win/heap.h"
#include "aglib_ds_safe_win/map.h"
#include "aglib_ds_safe_win/set.h"
#include "aglib_ds_safe_win/tree_map.h"
#include "aglib_ds_safe_win/tree_set.h"

#endif // _WIN32

#endif // THREAD_SAFE_AGLIB_DS

#endif // AG_LIB_DS

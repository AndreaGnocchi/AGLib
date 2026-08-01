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

#include "aglib_ds_safe/array.h"
#include "aglib_ds_safe/string.h"
#include "aglib_ds_safe/linked_list.h"
#include "aglib_ds_safe/stack.h"
#include "aglib_ds_safe/queue.h"
#include "aglib_ds_safe/heap.h"
#include "aglib_ds_safe/map.h"
#include "aglib_ds_safe/set.h"
#include "aglib_ds_safe/tree_map.h"
#include "aglib_ds_safe/tree_set.h"

#endif // THREAD_SAFE_AGLIB_DS

#endif // AG_LIB_DS

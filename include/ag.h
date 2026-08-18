#ifndef AG_LIB_H
#define AG_LIB_H

#ifdef _WIN32

#include "aglib_io_win.h"

#else

#include "aglib_io_posix.h"

#endif // _WIN32

#include "aglib_arena.h"
#include "aglib_slab.h"
#include "aglib_io.h"
#include "aglib_algo.h"
#include "aglib_allocator.h"
#include "aglib_slab.h"
#include "aglib_ds.h"
#include "aglib_helpers.h"

#endif // AG_LIB_H

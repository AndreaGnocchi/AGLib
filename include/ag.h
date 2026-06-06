#ifndef AG_LIB_H
#define AG_LIB_H

#if !defined(__linux__)
  #error "You need a Linux machine to use this library"
#endif // __linux__

#include "aglib_algo.h"
#include "aglib_ds.h"
#include "aglib_io.h"
#include "aglib_arena.h"
#include "aglib_helpers.h"

#endif // AG_LIB_H

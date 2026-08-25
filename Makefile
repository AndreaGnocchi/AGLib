# ═══════════════════════════════════════════════════════════════════════
# aglib Makefile — builds & tests Linux, Windows, and macOS from any host.
#
#   make                 build the static lib for THIS host's platform
#   make test            run the (non-thread-safe) test suite for THIS host
#   make test-safe       run the thread-safe test suite for THIS host
#
#   make all-linux / all-win / all-mac        build just the static lib
#   make test-linux / test-win / test-mac     build + run that platform's tests
#   make test-safe-linux / -win / -mac        same, thread-safe variant
#
#   make clean           remove every platform's build output
#
#   Every compiler is overridable, e.g.:
#     make test-win WIN_CC=/path/to/my-mingw-gcc
# ═══════════════════════════════════════════════════════════════════════

CFLAGS  = -Wall -Wextra -O2
AR      = ar
ARFLAGS = rcs

BUILD_DIR ?= build

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
  HOST_PLATFORM := win
else ifeq ($(UNAME_S),Darwin)
  HOST_PLATFORM := mac
else
  HOST_PLATFORM := linux
endif

ifeq ($(HOST_PLATFORM),linux)
  LINUX_CC ?= gcc
else
  LINUX_CC ?= gcc
endif

ifeq ($(HOST_PLATFORM),win)
  WIN_CC ?= gcc
else
  WIN_CC ?= x86_64-w64-mingw32-gcc
endif

ifeq ($(HOST_PLATFORM),mac)
  MAC_CC ?= clang
else
  MAC_CC ?= o64-clang
endif

RUN_linux :=

ifeq ($(HOST_PLATFORM),win)
  RUN_win :=
else
  WINE := $(shell command -v wine64 2>/dev/null || command -v wine 2>/dev/null || \
                  (test -x /usr/lib/wine/wine64 && echo /usr/lib/wine/wine64) 2>/dev/null)
  ifeq ($(WINE),)
    RUN_win = @echo "wine/wine64 not found on PATH - install wine to run cross-compiled Windows binaries (the build itself succeeded)."; exit 1;
  else
    RUN_win := $(WINE)
  endif
endif

ifeq ($(HOST_PLATFORM),mac)
  RUN_mac :=
else
  RUN_mac = @echo "skipping run: a macOS binary can only be executed on real macOS (the cross-build itself succeeded).";
endif

SAFE_FLAGS_linux := -pthread
SAFE_FLAGS_mac   := -pthread
SAFE_FLAGS_win   :=

COMMON_SRCS = src/aglib_algo.c src/aglib_allocator.c src/aglib_helpers.c src/aglib_io.c

LINUX_SRCS = $(COMMON_SRCS) src/aglib_arena.c src/aglib_slab.c src/aglib_io_posix.c src/aglib_io_linux.c
MAC_SRCS   = $(COMMON_SRCS) src/aglib_arena.c src/aglib_slab.c src/aglib_io_posix.c src/aglib_io_mac.c
WIN_SRCS   = $(COMMON_SRCS) src/aglib_arena.c src/aglib_arena_win.c src/aglib_slab.c src/aglib_slab_win.c src/aglib_io_win.c

TEST_SRCS = tests/test_main.c              \
            tests/test_framework.c         \
            tests/test_arena.c             \
            tests/test_slab.c              \
            tests/test_algo.c              \
            tests/test_io.c                \
            tests/test_io_slab.c           \
            tests/test_ds_array.c          \
            tests/test_ds_string.c         \
            tests/test_ds_linked_list.c    \
            tests/test_ds_linked_list_slab.c \
            tests/test_ds_stack.c          \
            tests/test_ds_stack_slab.c     \
            tests/test_ds_queue.c          \
            tests/test_ds_queue_slab.c     \
            tests/test_ds_heap.c           \
            tests/test_ds_map.c            \
            tests/test_ds_set.c            \
            tests/test_ds_tree_map.c       \
            tests/test_ds_tree_map_slab.c  \
            tests/test_ds_tree_set.c       \
            tests/test_ds_tree_set_slab.c  \
            tests/test_thread_safety.c

define PLATFORM_RULES

BUILD_$(1)  := $(BUILD_DIR)/$(1)
LIB_$(1)    := $$(BUILD_$(1))/libag.a

OBJS_$(1)            := $$(patsubst src/%.c,$$(BUILD_$(1))/obj/%.o,$(5))
TEST_OBJS_$(1)        := $$(patsubst tests/%.c,$$(BUILD_$(1))/test_obj/%.o,$(TEST_SRCS))
TEST_SAFE_OBJS_$(1)   := $$(patsubst tests/%.c,$$(BUILD_$(1))/test_safe_obj/%.o,$(TEST_SRCS))

TEST_BIN_$(1)      := $$(BUILD_$(1))/test$(7)
TEST_SAFE_BIN_$(1) := $$(BUILD_$(1))/test_safe$(7)

.PHONY: all-$(1) test-$(1) test-safe-$(1) clean-$(1)

all-$(1): $$(LIB_$(1))

$$(LIB_$(1)): $$(OBJS_$(1)) | $$(BUILD_$(1))
	$(AR) $(ARFLAGS) $$@ $$^

$$(BUILD_$(1))/obj/%.o: src/%.c | $$(BUILD_$(1))/obj
	$(2) $(CFLAGS) $(3) -MMD -c $$< -o $$@

$$(BUILD_$(1))/test_obj/%.o: tests/%.c | $$(BUILD_$(1))/test_obj
	$(2) $(CFLAGS) $(3) -MMD -c $$< -o $$@

$$(BUILD_$(1))/test_safe_obj/%.o: tests/%.c | $$(BUILD_$(1))/test_safe_obj
	$(2) $(CFLAGS) $(3) $(4) -DTHREAD_SAFE_AGLIB_DS -MMD -c $$< -o $$@

$$(TEST_BIN_$(1)): $$(TEST_OBJS_$(1)) $$(LIB_$(1))
	$(2) $(CFLAGS) $(3) $$(TEST_OBJS_$(1)) -o $$@ -L$$(BUILD_$(1)) -lag

$$(TEST_SAFE_BIN_$(1)): $$(TEST_SAFE_OBJS_$(1)) $$(LIB_$(1))
	$(2) $(CFLAGS) $(3) $(4) -DTHREAD_SAFE_AGLIB_DS $$(TEST_SAFE_OBJS_$(1)) -o $$@ -L$$(BUILD_$(1)) -lag $(4)

test-$(1): $$(TEST_BIN_$(1))
	$(6) $$(TEST_BIN_$(1))

test-safe-$(1): $$(TEST_SAFE_BIN_$(1))
	$(6) $$(TEST_SAFE_BIN_$(1))

clean-$(1):
	rm -rf $$(BUILD_$(1))

$$(BUILD_$(1)) $$(BUILD_$(1))/obj $$(BUILD_$(1))/test_obj $$(BUILD_$(1))/test_safe_obj:
	mkdir -p $$@

-include $$(OBJS_$(1):.o=.d)
-include $$(TEST_OBJS_$(1):.o=.d)
-include $$(TEST_SAFE_OBJS_$(1):.o=.d)

endef

$(eval $(call PLATFORM_RULES,linux,$(LINUX_CC),,$(SAFE_FLAGS_linux),$(LINUX_SRCS),$(RUN_linux),))
$(eval $(call PLATFORM_RULES,win,$(WIN_CC),,$(SAFE_FLAGS_win),$(WIN_SRCS),$(RUN_win),.exe))
$(eval $(call PLATFORM_RULES,mac,$(MAC_CC),,$(SAFE_FLAGS_mac),$(MAC_SRCS),$(RUN_mac),))

.PHONY: all test test-safe clean

all: all-$(HOST_PLATFORM)
test: test-$(HOST_PLATFORM)
test-safe: test-safe-$(HOST_PLATFORM)

clean:
	rm -rf $(BUILD_DIR)

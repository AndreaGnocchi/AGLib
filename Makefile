CC      = gcc
CFLAGS  = -Wall -Wextra -O2
AR      = ar
ARFLAGS = rcs

BUILD_DIR = build
LIB       = $(BUILD_DIR)/libag.a

SRCS = src/aglib_algo.c    \
       src/aglib_arena.c   \
       src/aglib_helpers.c \
       src/aglib_io.c

OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

TEST_SRC    = test.c
TEST_TARGET = $(BUILD_DIR)/test
TEST_SAFE   = $(BUILD_DIR)/test_safe

.PHONY: all test test-safe clean

all: $(LIB)

$(LIB): $(OBJS) | $(BUILD_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# default (non-thread-safe) data structures
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -lag

# same test.c, rebuilt against the thread-safe (mutex-protected) variant
test-safe: $(TEST_SAFE)
	./$(TEST_SAFE)

$(TEST_SAFE): $(TEST_SRC) $(LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -DTHREAD_SAFE_AGLIB_DS $< -o $@ -L$(BUILD_DIR) -lag -pthread

-include $(DEPS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

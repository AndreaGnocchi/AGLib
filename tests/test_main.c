#include "../include/ag.h"
#include "test_framework.h"
#include <stdio.h>

void run_arena_tests(void);
void run_slab_tests(void);
void run_algo_tests(void);
void run_io_tests(void);
void run_io_slab_tests(void);
void run_array_tests(void);
void run_string_tests(void);
void run_linked_list_tests(void);
void run_linked_list_slab_tests(void);
void run_stack_tests(void);
void run_stack_slab_tests(void);
void run_queue_tests(void);
void run_queue_slab_tests(void);
void run_heap_tests(void);
void run_map_tests(void);
void run_set_tests(void);
void run_tree_map_tests(void);
void run_tree_map_slab_tests(void);
void run_tree_set_tests(void);
void run_tree_set_slab_tests(void);

#ifdef THREAD_SAFE_AGLIB_DS
void run_thread_safety_tests(void);
#endif

int main(void) {
#ifdef THREAD_SAFE_AGLIB_DS
  printf("aglib test suite (thread-safe build)\n\n");
#else
  printf("aglib test suite (default build)\n\n");
#endif

  AG_RUN("Arena",             run_arena_tests);
  AG_RUN("Slab",              run_slab_tests);
  AG_RUN("Algo",              run_algo_tests);
  AG_RUN("IO",                run_io_tests);
  AG_RUN("IO (slab)",         run_io_slab_tests);
  AG_RUN("DynamicArray",      run_array_tests);
  AG_RUN("String",            run_string_tests);
  AG_RUN("LinkedList",        run_linked_list_tests);
  AG_RUN("LinkedList (slab)", run_linked_list_slab_tests);
  AG_RUN("Stack",             run_stack_tests);
  AG_RUN("Stack (slab)",      run_stack_slab_tests);
  AG_RUN("Queue",             run_queue_tests);
  AG_RUN("Queue (slab)",      run_queue_slab_tests);
  AG_RUN("Heap",              run_heap_tests);
  AG_RUN("Map",               run_map_tests);
  AG_RUN("Set",               run_set_tests);
  AG_RUN("TreeMap",           run_tree_map_tests);
  AG_RUN("TreeMap (slab)",    run_tree_map_slab_tests);
  AG_RUN("TreeSet",           run_tree_set_tests);
  AG_RUN("TreeSet (slab)",    run_tree_set_slab_tests);

#ifdef THREAD_SAFE_AGLIB_DS
  AG_RUN("ThreadSafety",      run_thread_safety_tests);
#endif

  return ag_test_summary();
}

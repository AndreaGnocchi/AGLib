#include "test_framework.h"

long g_ag_test_total     = 0;
long g_ag_test_failures  = 0;
const char* g_ag_current_suite = NULL;

static long s_suite_start_total    = 0;
static long s_suite_start_failures = 0;

void ag_test_suite_begin(const char* name) {
  g_ag_current_suite      = name;
  s_suite_start_total     = g_ag_test_total;
  s_suite_start_failures  = g_ag_test_failures;
  printf("-- %s --\n", name);
}

void ag_test_suite_end(void) {
  long ran    = g_ag_test_total    - s_suite_start_total;
  long failed = g_ag_test_failures - s_suite_start_failures;

  if (failed == 0) {
    printf("[OK] %s (%ld checks)\n", g_ag_current_suite, ran);
  } else {
    printf("[FAILED] %s (%ld/%ld checks failed)\n", g_ag_current_suite, failed, ran);
  }
}

int ag_test_summary(void) {
  printf("\n═══════════════════════════════════════════\n");
  if (g_ag_test_failures == 0) {
    printf("All checks passed (%ld/%ld)\n", g_ag_test_total, g_ag_test_total);
    return 0;
  }

  printf("%ld/%ld checks FAILED\n", g_ag_test_failures, g_ag_test_total);
  return 1;
}

#ifndef AG_TEST_FRAMEWORK_H
#define AG_TEST_FRAMEWORK_H

#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════
// Minimal cross-platform test framework.
//
// Unlike plain assert(), AG_CHECK does NOT abort the process on failure —
// it records the failure and keeps running the rest of the test case, so
// one bad assertion doesn't hide every other bug in the same test.
//
// Counters live in test_framework.c and are shared across every test
// translation unit (declared extern here, defined once there).
// ═══════════════════════════════════════════════════════════════════════

extern long g_ag_test_total;
extern long g_ag_test_failures;
extern const char* g_ag_current_suite;

void ag_test_suite_begin(const char* name);
void ag_test_suite_end(void);
int  ag_test_summary(void); // returns 0 on success, 1 if any failures

#define AG_CHECK(cond)                                                          \
  do {                                                                          \
    g_ag_test_total++;                                                         \
    if (!(cond)) {                                                              \
      g_ag_test_failures++;                                                    \
      fprintf(stderr, "  [FAIL] %s:%d in %s(): %s\n",                          \
              __FILE__, __LINE__, __func__, #cond);                            \
    }                                                                           \
  } while (0)

#define AG_CHECK_EQ_INT(actual, expected)                                       \
  do {                                                                          \
    g_ag_test_total++;                                                         \
    long long _a = (long long)(actual);                                        \
    long long _e = (long long)(expected);                                      \
    if (_a != _e) {                                                            \
      g_ag_test_failures++;                                                    \
      fprintf(stderr, "  [FAIL] %s:%d in %s(): %s == %s (got %lld, want %lld)\n",\
              __FILE__, __LINE__, __func__, #actual, #expected, _a, _e);       \
    }                                                                           \
  } while (0)

#define AG_CHECK_STR_EQ(actual, expected)                                       \
  do {                                                                          \
    g_ag_test_total++;                                                         \
    const char* _a = (actual);                                                 \
    const char* _e = (expected);                                               \
    if (!_a || !_e || strcmp(_a, _e) != 0) {                                   \
      g_ag_test_failures++;                                                    \
      fprintf(stderr, "  [FAIL] %s:%d in %s(): %s == %s (got \"%s\", want \"%s\")\n",\
              __FILE__, __LINE__, __func__, #actual, #expected,                \
              _a ? _a : "(null)", _e ? _e : "(null)");                         \
    }                                                                           \
  } while (0)

#define AG_RUN(suite_name, fn)                                                  \
  do {                                                                          \
    ag_test_suite_begin(suite_name);                                           \
    fn();                                                                       \
    ag_test_suite_end();                                                        \
  } while (0)

#endif // AG_TEST_FRAMEWORK_H

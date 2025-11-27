#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;
extern const char *g_current_suite;
extern bool g_current_test_passed;

#define TEST_SUITE(name)                                                       \
  do {                                                                         \
    g_current_suite = name;                                                    \
    printf("\n\033[1;34m=== %s ===\033[0m\n", name);                           \
  } while (0)

#define TEST(name)                                                             \
  static void test_##name(void);                                               \
  static void run_test_##name(void) {                                          \
    g_tests_run++;                                                             \
    g_current_test_passed = true;                                              \
    printf("  %-50s ", #name);                                                 \
    fflush(stdout);                                                            \
    test_##name();                                                             \
    if (g_current_test_passed) {                                               \
      g_tests_passed++;                                                        \
      printf("\033[32mPASS\033[0m\n");                                         \
    }                                                                          \
  }                                                                            \
  static void test_##name(void)

#define RUN_TEST(name) run_test_##name()

#define PASS()                                                                 \
  do {                                                                         \
    return;                                                                    \
  } while (0)

#define FAIL(msg)                                                              \
  do {                                                                         \
    g_current_test_passed = false;                                             \
    g_tests_failed++;                                                          \
    printf("\033[31mFAIL\033[0m\n");                                           \
    printf("    \033[31m%s\033[0m\n", msg);                                    \
    return;                                                                    \
  } while (0)

#define FAIL_FMT(fmt, ...)                                                     \
  do {                                                                         \
    g_current_test_passed = false;                                             \
    g_tests_failed++;                                                          \
    printf("\033[31mFAIL\033[0m\n");                                           \
    printf("    \033[31m" fmt "\033[0m\n", __VA_ARGS__);                       \
    return;                                                                    \
  } while (0)

#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond))                                                               \
      FAIL("Assertion failed: " #cond);                                        \
  } while (0)

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if ((a) != (b))                                                            \
      FAIL_FMT("Expected %s == %s", #a, #b);                                   \
  } while (0)

#define ASSERT_EQ_INT(expected, actual)                                        \
  do {                                                                         \
    int _e = (expected);                                                       \
    int _a = (actual);                                                         \
    if (_e != _a)                                                              \
      FAIL_FMT("Expected %d, got %d", _e, _a);                                 \
  } while (0)

#define ASSERT_EQ_SIZE(expected, actual)                                       \
  do {                                                                         \
    size_t _e = (expected);                                                    \
    size_t _a = (actual);                                                      \
    if (_e != _a)                                                              \
      FAIL_FMT("Expected %zu, got %zu", _e, _a);                               \
  } while (0)

#define ASSERT_EQ_STR(expected, actual)                                        \
  do {                                                                         \
    const char *_e = (expected);                                               \
    const char *_a = (actual);                                                 \
    if (_e == NULL && _a == NULL)                                              \
      break;                                                                   \
    if (_e == NULL || _a == NULL || strcmp(_e, _a) != 0)                       \
      FAIL_FMT("Expected \"%s\", got \"%s\"", _e ? _e : "(null)",              \
               _a ? _a : "(null)");                                            \
  } while (0)

#define ASSERT_NEAR(expected, actual, epsilon)                                 \
  do {                                                                         \
    double _e = (expected);                                                    \
    double _a = (actual);                                                      \
    if (fabs(_e - _a) > (epsilon))                                             \
      FAIL_FMT("Expected %.6f, got %.6f (epsilon=%.6f)", _e, _a,               \
               (double)(epsilon));                                             \
  } while (0)

#define ASSERT_NOT_NULL(ptr)                                                   \
  do {                                                                         \
    if ((ptr) == NULL)                                                         \
      FAIL("Expected non-NULL pointer: " #ptr);                                \
  } while (0)

#define ASSERT_NULL(ptr)                                                       \
  do {                                                                         \
    if ((ptr) != NULL)                                                         \
      FAIL("Expected NULL pointer: " #ptr);                                    \
  } while (0)

#define ASSERT_TRUE(cond)                                                      \
  do {                                                                         \
    if (!(cond))                                                               \
      FAIL("Expected true: " #cond);                                           \
  } while (0)

#define ASSERT_FALSE(cond)                                                     \
  do {                                                                         \
    if ((cond))                                                                \
      FAIL("Expected false: " #cond);                                          \
  } while (0)

static inline void print_test_summary(void) {
  printf("\n\033[1;36m=== Test Summary ===\033[0m\n");
  printf("  Total:  %d\n", g_tests_run);
  printf("  \033[32mPassed: %d\033[0m\n", g_tests_passed);
  if (g_tests_failed > 0) {
    printf("  \033[31mFailed: %d\033[0m\n", g_tests_failed);
  } else {
    printf("  Failed: %d\n", g_tests_failed);
  }
  printf("\n");
  if (g_tests_failed == 0) {
    printf("\033[1;32mAll tests passed!\033[0m\n");
  } else {
    printf("\033[1;31mSome tests failed!\033[0m\n");
  }
}

static inline int get_test_result(void) { return g_tests_failed > 0 ? 1 : 0; }

#endif

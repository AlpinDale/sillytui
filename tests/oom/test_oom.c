#include "../test_framework.h"

#define SILLYTUI_FAULT_INJECTION
#include "chat/history.h"
#include "core/config.h"
#include "core/fault.h"

TEST(oom_history_add_first_alloc) {
  fault_init();
  fault_enable_malloc(0, 1);

  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Test message");

  ASSERT_EQ_SIZE(SIZE_MAX, idx);
  ASSERT_EQ_INT(1, fault_get_malloc_fail_count());

  fault_disable_malloc();
  history_free(&h);
  PASS();
}

TEST(oom_history_add_second_alloc) {
  fault_init();
  fault_enable_malloc(1, 1);

  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Test message");

  ASSERT_EQ_SIZE(SIZE_MAX, idx);

  fault_disable_malloc();
  history_free(&h);
  PASS();
}

TEST(oom_history_survives_and_recovers) {
  fault_init();

  ChatHistory h;
  history_init(&h);

  history_add(&h, "Message 1");

  fault_enable_malloc(0, 1);
  size_t failed_idx = history_add(&h, "Should fail");
  ASSERT_EQ_SIZE(SIZE_MAX, failed_idx);
  fault_disable_malloc();

  ASSERT_EQ_SIZE(1, h.count);
  ASSERT_EQ_STR("Message 1", history_get(&h, 0));

  size_t success_idx = history_add(&h, "Message 2");
  ASSERT_EQ_SIZE(1, success_idx);
  ASSERT_EQ_SIZE(2, h.count);

  history_free(&h);
  PASS();
}

TEST(oom_history_add_swipe) {
  fault_init();

  ChatHistory h;
  history_init(&h);
  history_add(&h, "Original");

  fault_enable_malloc(0, 1);
  size_t swipe_idx = history_add_swipe(&h, 0, "Swipe content");
  ASSERT_EQ_SIZE(SIZE_MAX, swipe_idx);
  fault_disable_malloc();

  ASSERT_EQ_SIZE(1, history_get_swipe_count(&h, 0));

  history_free(&h);
  PASS();
}

TEST(oom_stress_repeated_failures) {
  fault_init();

  ChatHistory h;
  history_init(&h);

  int successes = 0;
  int failures = 0;

  for (int i = 0; i < 100; i++) {
    if (i % 3 == 0) {
      fault_enable_malloc(0, 1);
    }

    char msg[32];
    snprintf(msg, sizeof(msg), "Message %d", i);
    size_t idx = history_add(&h, msg);

    if (idx == SIZE_MAX) {
      failures++;
    } else {
      successes++;
    }

    fault_disable_malloc();
  }

  ASSERT(failures > 0);
  ASSERT(successes > 0);
  ASSERT_EQ_SIZE((size_t)successes, h.count);

  history_free(&h);
  PASS();
}

TEST(oom_no_memory_leaks) {
  fault_init();

  for (int fail_point = 0; fail_point < 10; fail_point++) {
    ChatHistory h;
    history_init(&h);

    fault_enable_malloc(fail_point, 1);

    history_add(&h, "Test 1");
    history_add(&h, "Test 2");
    history_add(&h, "Test 3");
    history_add_swipe(&h, 0, "Swipe");

    fault_disable_malloc();
    history_free(&h);
  }

  PASS();
}

void run_oom_tests(void);
void run_oom_tests(void) {
  TEST_SUITE("OOM (Out of Memory) Testing");
  RUN_TEST(oom_history_add_first_alloc);
  RUN_TEST(oom_history_add_second_alloc);
  RUN_TEST(oom_history_survives_and_recovers);
  RUN_TEST(oom_history_add_swipe);
  RUN_TEST(oom_stress_repeated_failures);
  RUN_TEST(oom_no_memory_leaks);
}

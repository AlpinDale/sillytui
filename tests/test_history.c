#include "chat/history.h"
#include "test_framework.h"
#include <stdint.h>

TEST(history_init_sets_defaults) {
  ChatHistory h;
  history_init(&h);
  ASSERT_EQ_SIZE(0, h.count);
  ASSERT_EQ_SIZE(0, h.capacity);
  ASSERT_NULL(h.messages);
  PASS();
}

TEST(history_add_single_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Hello, world!");
  ASSERT_EQ_SIZE(0, idx);
  ASSERT_EQ_SIZE(1, h.count);
  ASSERT_EQ_STR("Hello, world!", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_add_multiple_messages) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Message 1");
  history_add(&h, "Message 2");
  history_add(&h, "Message 3");
  ASSERT_EQ_SIZE(3, h.count);
  ASSERT_EQ_STR("Message 1", history_get(&h, 0));
  ASSERT_EQ_STR("Message 2", history_get(&h, 1));
  ASSERT_EQ_STR("Message 3", history_get(&h, 2));
  history_free(&h);
  PASS();
}

TEST(history_get_out_of_bounds_returns_null) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");
  ASSERT_NULL(history_get(&h, 1));
  ASSERT_NULL(history_get(&h, 100));
  history_free(&h);
  PASS();
}

TEST(history_update_message) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Original");
  history_update(&h, 0, "Updated");
  ASSERT_EQ_STR("Updated", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_update_out_of_bounds_noop) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");
  history_update(&h, 5, "Should not crash");
  ASSERT_EQ_STR("Test", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_delete_message) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "A");
  history_add(&h, "B");
  history_add(&h, "C");
  ASSERT_TRUE(history_delete(&h, 1));
  ASSERT_EQ_SIZE(2, h.count);
  ASSERT_EQ_STR("A", history_get(&h, 0));
  ASSERT_EQ_STR("C", history_get(&h, 1));
  history_free(&h);
  PASS();
}

TEST(history_delete_first_message) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "First");
  history_add(&h, "Second");
  ASSERT_TRUE(history_delete(&h, 0));
  ASSERT_EQ_SIZE(1, h.count);
  ASSERT_EQ_STR("Second", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_delete_last_message) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "A");
  history_add(&h, "B");
  ASSERT_TRUE(history_delete(&h, 1));
  ASSERT_EQ_SIZE(1, h.count);
  ASSERT_EQ_STR("A", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_delete_out_of_bounds) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");
  ASSERT_FALSE(history_delete(&h, 5));
  ASSERT_EQ_SIZE(1, h.count);
  history_free(&h);
  PASS();
}

TEST(history_swipes_basic) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Swipe 0");
  ASSERT_EQ_SIZE(1, history_get_swipe_count(&h, 0));
  ASSERT_EQ_SIZE(0, history_get_active_swipe(&h, 0));
  size_t s1 = history_add_swipe(&h, 0, "Swipe 1");
  ASSERT_EQ_SIZE(1, s1);
  ASSERT_EQ_SIZE(2, history_get_swipe_count(&h, 0));
  ASSERT_EQ_SIZE(1, history_get_active_swipe(&h, 0));
  ASSERT_EQ_STR("Swipe 1", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_set_active_swipe) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Original");
  history_add_swipe(&h, 0, "Alt 1");
  history_add_swipe(&h, 0, "Alt 2");
  ASSERT_TRUE(history_set_active_swipe(&h, 0, 0));
  ASSERT_EQ_STR("Original", history_get(&h, 0));
  ASSERT_TRUE(history_set_active_swipe(&h, 0, 2));
  ASSERT_EQ_STR("Alt 2", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_set_active_swipe_invalid) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");
  ASSERT_FALSE(history_set_active_swipe(&h, 0, 5));
  ASSERT_FALSE(history_set_active_swipe(&h, 5, 0));
  history_free(&h);
  PASS();
}

TEST(history_get_swipe) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Swipe 0");
  history_add_swipe(&h, 0, "Swipe 1");
  ASSERT_EQ_STR("Swipe 0", history_get_swipe(&h, 0, 0));
  ASSERT_EQ_STR("Swipe 1", history_get_swipe(&h, 0, 1));
  ASSERT_NULL(history_get_swipe(&h, 0, 2));
  ASSERT_NULL(history_get_swipe(&h, 1, 0));
  history_free(&h);
  PASS();
}

TEST(history_update_swipe) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Original");
  history_add_swipe(&h, 0, "Alt");
  history_update_swipe(&h, 0, 0, "Modified Original");
  history_update_swipe(&h, 0, 1, "Modified Alt");
  ASSERT_EQ_STR("Modified Original", history_get_swipe(&h, 0, 0));
  ASSERT_EQ_STR("Modified Alt", history_get_swipe(&h, 0, 1));
  history_free(&h);
  PASS();
}

TEST(history_token_counts) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test message");
  history_set_token_count(&h, 0, 0, 42);
  ASSERT_EQ_INT(42, history_get_token_count(&h, 0, 0));
  ASSERT_EQ_INT(0, history_get_token_count(&h, 0, 1));
  ASSERT_EQ_INT(0, history_get_token_count(&h, 1, 0));
  history_free(&h);
  PASS();
}

TEST(history_gen_times) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");
  history_set_gen_time(&h, 0, 0, 123.45);
  ASSERT_NEAR(123.45, history_get_gen_time(&h, 0, 0), 0.001);
  ASSERT_NEAR(0.0, history_get_gen_time(&h, 0, 1), 0.001);
  history_free(&h);
  PASS();
}

TEST(history_output_tps) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");
  history_set_output_tps(&h, 0, 0, 50.5);
  ASSERT_NEAR(50.5, history_get_output_tps(&h, 0, 0), 0.001);
  history_free(&h);
  PASS();
}

TEST(history_free_null_safe) {
  ChatHistory h;
  history_init(&h);
  history_free(&h);
  history_free(&h);
  PASS();
}

TEST(history_capacity_grows) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 100; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(100, h.count);
  ASSERT_TRUE(h.capacity >= 100);
  ASSERT_EQ_STR("Message 0", history_get(&h, 0));
  ASSERT_EQ_STR("Message 99", history_get(&h, 99));
  history_free(&h);
  PASS();
}

void run_history_tests(void) {
  TEST_SUITE("Chat History");
  RUN_TEST(history_init_sets_defaults);
  RUN_TEST(history_add_single_message);
  RUN_TEST(history_add_multiple_messages);
  RUN_TEST(history_get_out_of_bounds_returns_null);
  RUN_TEST(history_update_message);
  RUN_TEST(history_update_out_of_bounds_noop);
  RUN_TEST(history_delete_message);
  RUN_TEST(history_delete_first_message);
  RUN_TEST(history_delete_last_message);
  RUN_TEST(history_delete_out_of_bounds);
  RUN_TEST(history_swipes_basic);
  RUN_TEST(history_set_active_swipe);
  RUN_TEST(history_set_active_swipe_invalid);
  RUN_TEST(history_get_swipe);
  RUN_TEST(history_update_swipe);
  RUN_TEST(history_token_counts);
  RUN_TEST(history_gen_times);
  RUN_TEST(history_output_tps);
  RUN_TEST(history_free_null_safe);
  RUN_TEST(history_capacity_grows);
}

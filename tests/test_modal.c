#include "test_framework.h"
#include "ui/modal.h"

TEST(modal_init_type_none) {
  Modal m;
  modal_init(&m);
  ASSERT_EQ(MODAL_NONE, m.type);
  PASS();
}

TEST(modal_init_win_null) {
  Modal m;
  modal_init(&m);
  ASSERT_NULL(m.win);
  PASS();
}

TEST(modal_init_field_index_zero) {
  Modal m;
  modal_init(&m);
  ASSERT_EQ_INT(0, m.field_index);
  PASS();
}

TEST(modal_is_open_false_initially) {
  Modal m;
  modal_init(&m);
  ASSERT_FALSE(modal_is_open(&m));
  PASS();
}

TEST(modal_close_resets_type) {
  Modal m;
  modal_init(&m);
  m.type = MODAL_MESSAGE;
  modal_close(&m);
  ASSERT_EQ(MODAL_NONE, m.type);
  PASS();
}

TEST(modal_close_safe_on_null_win) {
  Modal m;
  modal_init(&m);
  m.win = NULL;
  m.type = MODAL_MODEL_SET;
  modal_close(&m);
  ASSERT_EQ(MODAL_NONE, m.type);
  PASS();
}

TEST(modal_exit_dont_ask_default_false) {
  Modal m;
  modal_init(&m);
  ASSERT_FALSE(modal_get_exit_dont_ask(&m));
  PASS();
}

TEST(modal_get_edit_msg_index_returns_stored) {
  Modal m;
  modal_init(&m);
  m.edit_msg_index = 42;
  ASSERT_EQ_INT(42, modal_get_edit_msg_index(&m));
  PASS();
}

TEST(modal_get_edit_content_returns_buffer) {
  Modal m;
  modal_init(&m);
  strcpy(m.edit_buffer, "Test content");
  ASSERT_EQ_STR("Test content", modal_get_edit_content(&m));
  PASS();
}

TEST(modal_fields_initialized_empty) {
  Modal m;
  modal_init(&m);
  for (int i = 0; i < 6; i++) {
    ASSERT_EQ_INT(0, m.fields[i][0]);
  }
  PASS();
}

TEST(modal_field_cursor_initialized_zero) {
  Modal m;
  modal_init(&m);
  for (int i = 0; i < 6; i++) {
    ASSERT_EQ_INT(0, m.field_cursor[i]);
    ASSERT_EQ_INT(0, m.field_len[i]);
  }
  PASS();
}

TEST(modal_api_type_selection_default) {
  Modal m;
  modal_init(&m);
  ASSERT_EQ(API_TYPE_OPENAI, m.api_type_selection);
  PASS();
}

TEST(modal_list_selection_initialized) {
  Modal m;
  modal_init(&m);
  ASSERT_EQ_INT(0, m.list_selection);
  ASSERT_EQ_INT(0, m.list_scroll);
  PASS();
}

TEST(modal_message_buffer_size) {
  Modal m;
  modal_init(&m);
  char long_msg[600];
  memset(long_msg, 'x', 599);
  long_msg[599] = '\0';
  strncpy(m.message, long_msg, sizeof(m.message) - 1);
  m.message[sizeof(m.message) - 1] = '\0';
  ASSERT(strlen(m.message) < 512);
  PASS();
}

TEST(modal_edit_buffer_can_hold_content) {
  Modal m;
  modal_init(&m);
  const char *content = "This is a test message that should fit in the buffer";
  strcpy(m.edit_buffer, content);
  ASSERT_EQ_STR(content, m.edit_buffer);
  PASS();
}

TEST(modal_sampler_fields_initialized) {
  Modal m;
  modal_init(&m);
  ASSERT_EQ_INT(0, m.sampler_field_index);
  ASSERT_EQ_INT(0, m.sampler_scroll);
  ASSERT_FALSE(m.sampler_adding_custom);
  PASS();
}

void run_modal_tests(void) {
  TEST_SUITE("Modal State");
  RUN_TEST(modal_init_type_none);
  RUN_TEST(modal_init_win_null);
  RUN_TEST(modal_init_field_index_zero);
  RUN_TEST(modal_is_open_false_initially);
  RUN_TEST(modal_close_resets_type);
  RUN_TEST(modal_close_safe_on_null_win);
  RUN_TEST(modal_exit_dont_ask_default_false);
  RUN_TEST(modal_get_edit_msg_index_returns_stored);
  RUN_TEST(modal_get_edit_content_returns_buffer);
  RUN_TEST(modal_fields_initialized_empty);
  RUN_TEST(modal_field_cursor_initialized_zero);
  RUN_TEST(modal_api_type_selection_default);
  RUN_TEST(modal_list_selection_initialized);
  RUN_TEST(modal_message_buffer_size);
  RUN_TEST(modal_edit_buffer_can_hold_content);
  RUN_TEST(modal_sampler_fields_initialized);
}

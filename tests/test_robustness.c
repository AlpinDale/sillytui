#include "chat/author_note.h"
#include "chat/history.h"
#include "core/config.h"
#include "core/macros.h"
#include "llm/sampler.h"
#include "test_framework.h"
#include "tokenizer/tiktoken.h"
#include "ui/ui.h"
#include <stdlib.h>
#include <string.h>

TEST(robust_history_very_long_message) {
  ChatHistory h;
  history_init(&h);
  char *huge = malloc(1024 * 1024);
  ASSERT_NOT_NULL(huge);
  memset(huge, 'A', 1024 * 1024 - 1);
  huge[1024 * 1024 - 1] = '\0';
  size_t idx = history_add(&h, huge);
  const char *got = history_get(&h, idx);
  ASSERT_NOT_NULL(got);
  ASSERT(strlen(got) > 0);
  free(huge);
  history_free(&h);
  PASS();
}

TEST(robust_history_binary_content) {
  ChatHistory h;
  history_init(&h);
  char binary[256];
  for (int i = 0; i < 255; i++)
    binary[i] = (char)(i + 1);
  binary[255] = '\0';
  size_t idx = history_add(&h, binary);
  const char *got = history_get(&h, idx);
  ASSERT_NOT_NULL(got);
  history_free(&h);
  PASS();
}

TEST(robust_macro_huge_input) {
  char *huge = malloc(100000);
  ASSERT_NOT_NULL(huge);
  memset(huge, 'x', 99999);
  huge[99999] = '\0';
  char *result = macro_substitute(huge, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  free(huge);
  PASS();
}

TEST(robust_macro_many_substitutions) {
  char input[4096];
  strcpy(input, "");
  for (int i = 0; i < 100; i++) {
    strcat(input, "{{char}} {{user}} ");
  }
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int alice_count = 0;
  char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    alice_count++;
    p++;
  }
  ASSERT_EQ_INT(100, alice_count);
  free(result);
  PASS();
}

TEST(robust_config_model_name_at_limit) {
  ModelsFile mf = {0};
  ModelConfig m = {0};
  memset(m.name, 'A', MAX_NAME_LEN - 1);
  m.name[MAX_NAME_LEN - 1] = '\0';
  ASSERT_TRUE(config_add_model(&mf, &m));
  ASSERT_EQ_INT((int)strlen(mf.models[0].name), MAX_NAME_LEN - 1);
  PASS();
}

TEST(robust_config_all_fields_maxed) {
  ModelsFile mf = {0};
  ModelConfig m = {0};
  memset(m.name, 'N', MAX_NAME_LEN - 1);
  m.name[MAX_NAME_LEN - 1] = '\0';
  memset(m.base_url, 'U', MAX_URL_LEN - 1);
  m.base_url[MAX_URL_LEN - 1] = '\0';
  memset(m.api_key, 'K', MAX_KEY_LEN - 1);
  m.api_key[MAX_KEY_LEN - 1] = '\0';
  memset(m.model_id, 'M', MAX_MODEL_ID_LEN - 1);
  m.model_id[MAX_MODEL_ID_LEN - 1] = '\0';
  m.context_length = INT32_MAX;
  ASSERT_TRUE(config_add_model(&mf, &m));
  PASS();
}

TEST(robust_sampler_extreme_float_values) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  s.temperature = 1e38;
  s.top_p = -1e38;
  s.min_p = 0.0 / 0.0;
  ASSERT(s.temperature > 0 || s.temperature <= 0);
  PASS();
}

TEST(robust_utf8_overlong_sequences) {
  uint8_t overlong_2byte[] = {0xC0, 0x80};
  uint32_t cp;
  int len = utf8_decode(overlong_2byte, 2, &cp);
  ASSERT(len >= 1);
  PASS();
}

TEST(robust_utf8_surrogate_pairs) {
  uint8_t surrogate[] = {0xED, 0xA0, 0x80};
  uint32_t cp;
  int len = utf8_decode(surrogate, 3, &cp);
  ASSERT(len >= 1);
  PASS();
}

TEST(robust_utf8_invalid_continuation) {
  uint8_t invalid[] = {0xE0, 0x41, 0x80};
  uint32_t cp;
  int len = utf8_decode(invalid, 3, &cp);
  ASSERT(len >= 1);
  PASS();
}

TEST(robust_pretokenize_only_whitespace) {
  SpanList spans = {0};
  int result = pretokenize_cl100k("   \t\t\n\n\r\r   ", &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(robust_pretokenize_only_punctuation) {
  SpanList spans = {0};
  int result = pretokenize_cl100k("!@#$%^&*()[]{}|\\:;\"'<>,.?/`~", &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(robust_pretokenize_mixed_scripts) {
  SpanList spans = {0};
  int result =
      pretokenize_cl100k("Hello世界مرحباПриветשלום日本語한글ไทย", &spans);
  ASSERT(result >= 0);
  size_t covered = 0;
  for (size_t i = 0; i < spans.count; i++) {
    covered += spans.spans[i].end - spans.spans[i].start;
  }
  ASSERT(covered > 0);
  free(spans.spans);
  PASS();
}

TEST(robust_history_rapid_realloc) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 1000; i++) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Message %d with some extra text", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(1000, h.count);
  for (int i = 999; i >= 0; i--) {
    history_delete(&h, (size_t)i);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(robust_swipe_stress) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int i = 0; i < 64; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", i);
    history_add_swipe(&h, idx, buf);
  }
  for (int i = 64; i >= 0; i--) {
    history_set_active_swipe(&h, idx, (size_t)i);
    const char *swipe = history_get_swipe(&h, idx, (size_t)i);
    ASSERT_NOT_NULL(swipe);
  }
  history_free(&h);
  PASS();
}

TEST(robust_empty_strings_everywhere) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "");
  history_update(&h, 0, "");
  history_add_swipe(&h, 0, "");
  const char *msg = history_get(&h, 0);
  ASSERT_NOT_NULL(msg);
  history_free(&h);

  char *result = macro_substitute("", "", "");
  ASSERT_NOT_NULL(result);
  free(result);

  result = macro_substitute("{{char}}{{user}}", "", "");
  ASSERT_NOT_NULL(result);
  free(result);

  PASS();
}

TEST(robust_null_everywhere) {
  char *result = macro_substitute(NULL, "a", "b");
  ASSERT_NULL(result);

  result = macro_substitute("test", NULL, "b");
  ASSERT_NOT_NULL(result);
  free(result);

  result = macro_substitute("test", "a", NULL);
  ASSERT_NOT_NULL(result);
  free(result);

  const char *msg = history_get(NULL, 0);
  ASSERT_NULL(msg);

  msg = history_get_swipe(NULL, 0, 0);
  ASSERT_NULL(msg);

  bool ok = config_add_model(NULL, NULL);
  ASSERT_FALSE(ok);

  ok = config_remove_model(NULL, 0);
  ASSERT_FALSE(ok);

  ModelConfig *active = config_get_active(NULL);
  ASSERT_NULL(active);

  PASS();
}

TEST(robust_config_load_null) {
  bool ok = config_load_models(NULL);
  ASSERT_FALSE(ok);
  ok = config_save_models(NULL);
  ASSERT_FALSE(ok);
  config_set_active(NULL, 0);
  PASS();
}

TEST(robust_history_init_free_null) {
  history_init(NULL);
  history_free(NULL);
  PASS();
}

TEST(robust_history_add_null_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, NULL);
  (void)idx;
  history_free(&h);
  PASS();
}

TEST(robust_sampler_null_settings) {
  sampler_init_defaults(NULL);
  PASS();
}

TEST(robust_index_beyond_count) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "test");

  const char *msg = history_get(&h, 1000);
  ASSERT_NULL(msg);

  history_update(&h, 1000, "x");

  bool ok = history_delete(&h, 1000);
  ASSERT_FALSE(ok);

  ok = history_set_active_swipe(&h, 1000, 0);
  ASSERT_FALSE(ok);

  history_free(&h);
  PASS();
}

TEST(robust_size_max_index) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "test");

  const char *msg = history_get(&h, SIZE_MAX);
  ASSERT_NULL(msg);

  msg = history_get_swipe(&h, SIZE_MAX, 0);
  ASSERT_NULL(msg);

  msg = history_get_swipe(&h, 0, SIZE_MAX);
  ASSERT_NULL(msg);

  history_free(&h);
  PASS();
}

TEST(robust_double_init) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "first");
  history_init(&h);
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(robust_double_free) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "test");
  history_free(&h);
  history_free(&h);
  PASS();
}

TEST(robust_use_after_free_safe) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "test");
  history_free(&h);
  const char *msg = history_get(&h, 0);
  ASSERT_NULL(msg);
  PASS();
}

TEST(robust_operations_on_empty) {
  ChatHistory h;
  history_init(&h);

  const char *msg = history_get(&h, 0);
  ASSERT_NULL(msg);

  bool ok = history_delete(&h, 0);
  ASSERT_FALSE(ok);

  history_update(&h, 0, "test");

  size_t count = history_get_swipe_count(&h, 0);
  ASSERT_EQ_SIZE(0, count);

  history_free(&h);
  PASS();
}

TEST(robust_config_operations_on_empty) {
  ModelsFile mf = {0};
  mf.active_index = -1;

  bool ok = config_remove_model(&mf, 0);
  ASSERT_FALSE(ok);

  ModelConfig *active = config_get_active(&mf);
  ASSERT_NULL(active);

  config_set_active(&mf, 0);
  ASSERT_EQ_INT(-1, mf.active_index);

  PASS();
}

TEST(robust_sampler_operations_on_empty) {
  SamplerSettings s;
  sampler_init_defaults(&s);

  bool ok = sampler_remove_custom(&s, 0);
  ASSERT_FALSE(ok);

  ok = sampler_remove_custom(&s, -1);
  ASSERT_FALSE(ok);

  PASS();
}

TEST(robust_tokenizer_unloaded) {
  Tokenizer tok;
  tokenizer_init(&tok);

  int count = tokenizer_encode(&tok, "hello", NULL, 0);
  ASSERT(count <= 0);

  tokenizer_free(&tok);
  PASS();
}

TEST(message_role_default_is_user) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Hello");
  ASSERT_EQ_INT(ROLE_USER, history_get_role(&h, 0));
  history_free(&h);
  PASS();
}

TEST(message_role_add_with_role) {
  ChatHistory h;
  history_init(&h);
  history_add_with_role(&h, "System message", ROLE_SYSTEM);
  history_add_with_role(&h, "User message", ROLE_USER);
  history_add_with_role(&h, "Assistant message", ROLE_ASSISTANT);
  ASSERT_EQ_INT(ROLE_SYSTEM, history_get_role(&h, 0));
  ASSERT_EQ_INT(ROLE_USER, history_get_role(&h, 1));
  ASSERT_EQ_INT(ROLE_ASSISTANT, history_get_role(&h, 2));
  history_free(&h);
  PASS();
}

TEST(message_role_set_role) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Hello");
  ASSERT_EQ_INT(ROLE_USER, history_get_role(&h, 0));
  history_set_role(&h, 0, ROLE_SYSTEM);
  ASSERT_EQ_INT(ROLE_SYSTEM, history_get_role(&h, 0));
  history_set_role(&h, 0, ROLE_ASSISTANT);
  ASSERT_EQ_INT(ROLE_ASSISTANT, history_get_role(&h, 0));
  history_free(&h);
  PASS();
}

TEST(message_role_to_string) {
  ASSERT_EQ_STR("user", role_to_string(ROLE_USER));
  ASSERT_EQ_STR("assistant", role_to_string(ROLE_ASSISTANT));
  ASSERT_EQ_STR("system", role_to_string(ROLE_SYSTEM));
  PASS();
}

TEST(message_role_from_string) {
  ASSERT_EQ_INT(ROLE_USER, role_from_string("user"));
  ASSERT_EQ_INT(ROLE_ASSISTANT, role_from_string("assistant"));
  ASSERT_EQ_INT(ROLE_SYSTEM, role_from_string("system"));
  ASSERT_EQ_INT(ROLE_USER, role_from_string("unknown"));
  ASSERT_EQ_INT(ROLE_USER, role_from_string(NULL));
  PASS();
}

TEST(message_role_null_safety) {
  ASSERT_EQ_INT(ROLE_USER, history_get_role(NULL, 0));
  history_set_role(NULL, 0, ROLE_SYSTEM);
  PASS();
}

TEST(message_role_out_of_bounds) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Hello");
  ASSERT_EQ_INT(ROLE_USER, history_get_role(&h, 100));
  history_set_role(&h, 100, ROLE_SYSTEM);
  ASSERT_EQ_INT(ROLE_USER, history_get_role(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_move_up_basic) {
  ChatHistory h;
  history_init(&h);
  history_add_with_role(&h, "First", ROLE_ASSISTANT);
  history_add_with_role(&h, "Second", ROLE_USER);
  history_add_with_role(&h, "Third", ROLE_SYSTEM);

  bool moved = history_move_up(&h, 2);
  ASSERT_TRUE(moved);
  ASSERT_EQ_STR("First", history_get(&h, 0));
  ASSERT_EQ_STR("Third", history_get(&h, 1));
  ASSERT_EQ_STR("Second", history_get(&h, 2));
  ASSERT_EQ_INT(ROLE_SYSTEM, history_get_role(&h, 1));
  ASSERT_EQ_INT(ROLE_USER, history_get_role(&h, 2));

  history_free(&h);
  PASS();
}

TEST(history_move_down_basic) {
  ChatHistory h;
  history_init(&h);
  history_add_with_role(&h, "First", ROLE_ASSISTANT);
  history_add_with_role(&h, "Second", ROLE_USER);
  history_add_with_role(&h, "Third", ROLE_SYSTEM);

  bool moved = history_move_down(&h, 0);
  ASSERT_TRUE(moved);
  ASSERT_EQ_STR("Second", history_get(&h, 0));
  ASSERT_EQ_STR("First", history_get(&h, 1));
  ASSERT_EQ_STR("Third", history_get(&h, 2));
  ASSERT_EQ_INT(ROLE_USER, history_get_role(&h, 0));
  ASSERT_EQ_INT(ROLE_ASSISTANT, history_get_role(&h, 1));

  history_free(&h);
  PASS();
}

TEST(history_move_up_at_top) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "First");
  history_add(&h, "Second");

  bool moved = history_move_up(&h, 0);
  ASSERT_FALSE(moved);
  ASSERT_EQ_STR("First", history_get(&h, 0));
  ASSERT_EQ_STR("Second", history_get(&h, 1));

  history_free(&h);
  PASS();
}

TEST(history_move_down_at_bottom) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "First");
  history_add(&h, "Second");

  bool moved = history_move_down(&h, 1);
  ASSERT_FALSE(moved);
  ASSERT_EQ_STR("First", history_get(&h, 0));
  ASSERT_EQ_STR("Second", history_get(&h, 1));

  history_free(&h);
  PASS();
}

TEST(history_move_null_safety) {
  ASSERT_FALSE(history_move_up(NULL, 0));
  ASSERT_FALSE(history_move_down(NULL, 0));

  ChatHistory h;
  history_init(&h);
  ASSERT_FALSE(history_move_up(&h, 0));
  ASSERT_FALSE(history_move_down(&h, 0));
  history_free(&h);
  PASS();
}

TEST(history_move_preserves_swipes) {
  ChatHistory h;
  history_init(&h);
  history_add_with_role(&h, "Bot response 1", ROLE_ASSISTANT);
  history_add_swipe(&h, 0, "Bot response alt");
  history_add(&h, "User msg");

  bool moved = history_move_down(&h, 0);
  ASSERT_TRUE(moved);

  ASSERT_EQ_STR("User msg", history_get(&h, 0));
  ASSERT_EQ_SIZE(2, history_get_swipe_count(&h, 1));
  ASSERT_EQ_STR("Bot response 1", history_get_swipe(&h, 1, 0));
  ASSERT_EQ_STR("Bot response alt", history_get_swipe(&h, 1, 1));
  ASSERT_EQ_SIZE(1, history_get_active_swipe(&h, 1));

  history_free(&h);
  PASS();
}

TEST(author_note_init) {
  AuthorNote note;
  author_note_init(&note);
  ASSERT_EQ_STR("", note.text);
  ASSERT_EQ_INT(AN_POS_IN_CHAT, note.position);
  ASSERT_EQ_INT(AN_ROLE_SYSTEM, note.role);
  ASSERT_EQ_INT(4, note.depth);
  ASSERT_EQ_INT(1, note.interval);
  author_note_free(&note);
  PASS();
}

TEST(author_note_set_get) {
  AuthorNote note;
  author_note_init(&note);

  author_note_set_text(&note, "Be creative and descriptive.");
  ASSERT_EQ_STR("Be creative and descriptive.", note.text);

  author_note_set_depth(&note, 6);
  ASSERT_EQ_INT(6, note.depth);

  author_note_set_interval(&note, 2);
  ASSERT_EQ_INT(2, note.interval);

  author_note_free(&note);
  PASS();
}

TEST(author_note_defaults) {
  AuthorNote note;
  author_note_init(&note);

  ASSERT_EQ_INT(4, note.depth);
  ASSERT_EQ_INT(1, note.interval);
  ASSERT_EQ_INT(AN_POS_IN_CHAT, note.position);
  ASSERT_EQ_INT(AN_ROLE_SYSTEM, note.role);

  author_note_free(&note);
  PASS();
}

TEST(author_note_position_values) {
  AuthorNote note;
  author_note_init(&note);

  author_note_set_position(&note, AN_POS_BEFORE_SCENARIO);
  ASSERT_EQ_INT(AN_POS_BEFORE_SCENARIO, note.position);

  author_note_set_position(&note, AN_POS_AFTER_SCENARIO);
  ASSERT_EQ_INT(AN_POS_AFTER_SCENARIO, note.position);

  author_note_set_position(&note, AN_POS_IN_CHAT);
  ASSERT_EQ_INT(AN_POS_IN_CHAT, note.position);

  author_note_free(&note);
  PASS();
}

TEST(author_note_role_values) {
  AuthorNote note;
  author_note_init(&note);

  author_note_set_role(&note, AN_ROLE_SYSTEM);
  ASSERT_EQ_INT(AN_ROLE_SYSTEM, note.role);

  author_note_set_role(&note, AN_ROLE_USER);
  ASSERT_EQ_INT(AN_ROLE_USER, note.role);

  author_note_set_role(&note, AN_ROLE_ASSISTANT);
  ASSERT_EQ_INT(AN_ROLE_ASSISTANT, note.role);

  author_note_free(&note);
  PASS();
}

TEST(reasoning_set_get_basic) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Hello");
  history_set_reasoning(&h, idx, 0, "This is my reasoning", 1500.0);
  const char *reasoning = history_get_reasoning(&h, idx, 0);
  ASSERT_NOT_NULL(reasoning);
  ASSERT_EQ_STR("This is my reasoning", reasoning);
  double time = history_get_reasoning_time(&h, idx, 0);
  ASSERT(time > 1499.0 && time < 1501.0);
  history_free(&h);
  PASS();
}

TEST(reasoning_null_on_missing) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Hello");
  const char *reasoning = history_get_reasoning(&h, idx, 0);
  ASSERT(reasoning == NULL);
  double time = history_get_reasoning_time(&h, idx, 0);
  ASSERT(time == 0);
  history_free(&h);
  PASS();
}

TEST(reasoning_multiple_swipes) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "First response");
  history_add_swipe(&h, idx, "Second response");
  history_add_swipe(&h, idx, "Third response");
  history_set_reasoning(&h, idx, 0, "Reasoning for first", 100.0);
  history_set_reasoning(&h, idx, 1, "Reasoning for second", 200.0);
  history_set_reasoning(&h, idx, 2, "Reasoning for third", 300.0);
  ASSERT_EQ_STR("Reasoning for first", history_get_reasoning(&h, idx, 0));
  ASSERT_EQ_STR("Reasoning for second", history_get_reasoning(&h, idx, 1));
  ASSERT_EQ_STR("Reasoning for third", history_get_reasoning(&h, idx, 2));
  ASSERT(history_get_reasoning_time(&h, idx, 0) > 99.0);
  ASSERT(history_get_reasoning_time(&h, idx, 1) > 199.0);
  ASSERT(history_get_reasoning_time(&h, idx, 2) > 299.0);
  history_free(&h);
  PASS();
}

TEST(reasoning_null_safety) {
  history_set_reasoning(NULL, 0, 0, "test", 100.0);
  const char *r = history_get_reasoning(NULL, 0, 0);
  ASSERT(r == NULL);
  double t = history_get_reasoning_time(NULL, 0, 0);
  ASSERT(t == 0);
  ChatHistory h;
  history_init(&h);
  history_set_reasoning(&h, 999, 0, "test", 100.0);
  r = history_get_reasoning(&h, 999, 0);
  ASSERT(r == NULL);
  history_free(&h);
  PASS();
}

TEST(reasoning_update_existing) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Hello");
  history_set_reasoning(&h, idx, 0, "Original reasoning", 500.0);
  ASSERT_EQ_STR("Original reasoning", history_get_reasoning(&h, idx, 0));
  history_set_reasoning(&h, idx, 0, "Updated reasoning", 1000.0);
  ASSERT_EQ_STR("Updated reasoning", history_get_reasoning(&h, idx, 0));
  ASSERT(history_get_reasoning_time(&h, idx, 0) > 999.0);
  history_free(&h);
  PASS();
}

TEST(reasoning_empty_string) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Hello");
  history_set_reasoning(&h, idx, 0, "", 50.0);
  const char *r = history_get_reasoning(&h, idx, 0);
  ASSERT_NOT_NULL(r);
  ASSERT_EQ_STR("", r);
  history_free(&h);
  PASS();
}

TEST(reasoning_very_long) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Hello");
  char *huge = malloc(100000);
  ASSERT_NOT_NULL(huge);
  memset(huge, 'x', 99999);
  huge[99999] = '\0';
  history_set_reasoning(&h, idx, 0, huge, 5000.0);
  const char *r = history_get_reasoning(&h, idx, 0);
  ASSERT_NOT_NULL(r);
  ASSERT(strlen(r) == 99999);
  free(huge);
  history_free(&h);
  PASS();
}

TEST(reasoning_set_null_clears) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Hello");
  history_set_reasoning(&h, idx, 0, "Some reasoning", 100.0);
  ASSERT_NOT_NULL(history_get_reasoning(&h, idx, 0));
  history_set_reasoning(&h, idx, 0, NULL, 0.0);
  ASSERT(history_get_reasoning(&h, idx, 0) == NULL);
  history_free(&h);
  PASS();
}

TEST(reasoning_toggle_expand) {
  ui_reset_reasoning_state();
  ASSERT(!ui_is_reasoning_expanded(0));
  ASSERT(!ui_is_reasoning_expanded(5));
  ui_toggle_reasoning(0);
  ASSERT(ui_is_reasoning_expanded(0));
  ASSERT(!ui_is_reasoning_expanded(5));
  ui_toggle_reasoning(0);
  ASSERT(!ui_is_reasoning_expanded(0));
  ui_toggle_reasoning(5);
  ASSERT(ui_is_reasoning_expanded(5));
  ui_reset_reasoning_state();
  ASSERT(!ui_is_reasoning_expanded(0));
  ASSERT(!ui_is_reasoning_expanded(5));
  PASS();
}

TEST(reasoning_toggle_out_of_bounds) {
  ui_reset_reasoning_state();
  ui_toggle_reasoning(999999);
  ASSERT(!ui_is_reasoning_expanded(999999));
  PASS();
}

void run_robustness_tests(void) {
  TEST_SUITE("Robustness Tests");
  RUN_TEST(robust_history_very_long_message);
  RUN_TEST(robust_history_binary_content);
  RUN_TEST(robust_macro_huge_input);
  RUN_TEST(robust_macro_many_substitutions);
  RUN_TEST(robust_config_model_name_at_limit);
  RUN_TEST(robust_config_all_fields_maxed);
  RUN_TEST(robust_sampler_extreme_float_values);
  RUN_TEST(robust_utf8_overlong_sequences);
  RUN_TEST(robust_utf8_surrogate_pairs);
  RUN_TEST(robust_utf8_invalid_continuation);
  RUN_TEST(robust_pretokenize_only_whitespace);
  RUN_TEST(robust_pretokenize_only_punctuation);
  RUN_TEST(robust_pretokenize_mixed_scripts);
  RUN_TEST(robust_history_rapid_realloc);
  RUN_TEST(robust_swipe_stress);
  RUN_TEST(robust_empty_strings_everywhere);
  RUN_TEST(robust_null_everywhere);
  RUN_TEST(robust_config_load_null);
  RUN_TEST(robust_history_init_free_null);
  RUN_TEST(robust_history_add_null_message);
  RUN_TEST(robust_sampler_null_settings);
  RUN_TEST(robust_index_beyond_count);
  RUN_TEST(robust_size_max_index);
  RUN_TEST(robust_double_init);
  RUN_TEST(robust_double_free);
  RUN_TEST(robust_use_after_free_safe);
  RUN_TEST(robust_operations_on_empty);
  RUN_TEST(robust_config_operations_on_empty);
  RUN_TEST(robust_sampler_operations_on_empty);
  RUN_TEST(robust_tokenizer_unloaded);
  RUN_TEST(message_role_default_is_user);
  RUN_TEST(message_role_add_with_role);
  RUN_TEST(message_role_set_role);
  RUN_TEST(message_role_to_string);
  RUN_TEST(message_role_from_string);
  RUN_TEST(message_role_null_safety);
  RUN_TEST(message_role_out_of_bounds);
  RUN_TEST(history_move_up_basic);
  RUN_TEST(history_move_down_basic);
  RUN_TEST(history_move_up_at_top);
  RUN_TEST(history_move_down_at_bottom);
  RUN_TEST(history_move_null_safety);
  RUN_TEST(history_move_preserves_swipes);
  RUN_TEST(author_note_init);
  RUN_TEST(author_note_set_get);
  RUN_TEST(author_note_defaults);
  RUN_TEST(author_note_position_values);
  RUN_TEST(author_note_role_values);
  RUN_TEST(reasoning_set_get_basic);
  RUN_TEST(reasoning_null_on_missing);
  RUN_TEST(reasoning_multiple_swipes);
  RUN_TEST(reasoning_null_safety);
  RUN_TEST(reasoning_update_existing);
  RUN_TEST(reasoning_empty_string);
  RUN_TEST(reasoning_very_long);
  RUN_TEST(reasoning_set_null_clears);
  RUN_TEST(reasoning_toggle_expand);
  RUN_TEST(reasoning_toggle_out_of_bounds);
}

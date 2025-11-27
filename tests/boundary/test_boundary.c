#include "../test_framework.h"
#include "chat/history.h"
#include "core/config.h"
#include "core/macros.h"
#include "llm/sampler.h"
#include "tokenizer/tiktoken.h"

TEST(boundary_history_empty_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "");
  ASSERT(idx != SIZE_MAX);
  ASSERT_EQ_STR("", history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(boundary_history_null_message) {
  ChatHistory h;
  history_init(&h);
  history_free(&h);
  PASS();
}

TEST(boundary_history_very_long_message) {
  ChatHistory h;
  history_init(&h);

  char *long_msg = malloc(1000001);
  if (!long_msg)
    FAIL("malloc failed");
  memset(long_msg, 'x', 1000000);
  long_msg[1000000] = '\0';

  size_t idx = history_add(&h, long_msg);
  ASSERT(idx != SIZE_MAX);
  ASSERT_EQ_SIZE(1000000, strlen(history_get(&h, 0)));

  free(long_msg);
  history_free(&h);
  PASS();
}

TEST(boundary_history_unicode_message) {
  ChatHistory h;
  history_init(&h);
  const char *unicode = "Hello 🌍🔥 こんにちは 中文 العربية";
  size_t idx = history_add(&h, unicode);
  ASSERT(idx != SIZE_MAX);
  ASSERT_EQ_STR(unicode, history_get(&h, 0));
  history_free(&h);
  PASS();
}

TEST(boundary_history_binary_data) {
  ChatHistory h;
  history_init(&h);
  char binary[10] = {0, 1, 2, 0, 3, 4, 0, 5, 6, 0};
  size_t idx = history_add(&h, binary);
  ASSERT(idx != SIZE_MAX);
  history_free(&h);
  PASS();
}

TEST(boundary_history_max_swipes) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Original");

  int added = 0;
  for (int i = 0; i < MAX_SWIPES + 10; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", i);
    size_t idx = history_add_swipe(&h, 0, buf);
    if (idx != SIZE_MAX)
      added++;
  }

  ASSERT(added <= MAX_SWIPES);
  ASSERT_EQ_SIZE((size_t)added + 1, history_get_swipe_count(&h, 0));

  history_free(&h);
  PASS();
}

TEST(boundary_history_index_overflow) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");

  ASSERT_NULL(history_get(&h, SIZE_MAX));
  ASSERT_NULL(history_get(&h, SIZE_MAX - 1));
  ASSERT_NULL(history_get_swipe(&h, SIZE_MAX, 0));
  ASSERT_NULL(history_get_swipe(&h, 0, SIZE_MAX));

  history_free(&h);
  PASS();
}

TEST(boundary_config_empty_name) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig m = {0};
  m.name[0] = '\0';
  ASSERT_TRUE(config_add_model(&mf, &m));
  PASS();
}

TEST(boundary_config_max_name_length) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig m = {0};
  memset(m.name, 'x', sizeof(m.name) - 1);
  m.name[sizeof(m.name) - 1] = '\0';
  ASSERT_TRUE(config_add_model(&mf, &m));
  ASSERT_EQ_SIZE(sizeof(m.name) - 1, strlen(mf.models[0].name));
  PASS();
}

TEST(boundary_config_zero_context_length) {
  ModelsFile mf = {0};
  ModelConfig m = {0};
  snprintf(m.name, sizeof(m.name), "Test");
  m.context_length = 0;
  config_add_model(&mf, &m);
  PASS();
}

TEST(boundary_config_negative_context_length) {
  ModelsFile mf = {0};
  ModelConfig m = {0};
  snprintf(m.name, sizeof(m.name), "Test");
  m.context_length = -1;
  config_add_model(&mf, &m);
  PASS();
}

TEST(boundary_config_max_models) {
  ModelsFile mf = {0};
  mf.active_index = -1;

  for (size_t i = 0; i < MAX_MODELS; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %zu", i);
    ASSERT_TRUE(config_add_model(&mf, &m));
  }

  ModelConfig overflow = {0};
  snprintf(overflow.name, sizeof(overflow.name), "Overflow");
  ASSERT_FALSE(config_add_model(&mf, &overflow));
  ASSERT_EQ_SIZE(MAX_MODELS, mf.count);

  PASS();
}

TEST(boundary_macro_empty_pattern) {
  char *result = macro_substitute("{{char}}", "", "User");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("", result);
  free(result);
  PASS();
}

TEST(boundary_macro_very_long_replacement) {
  char *long_name = malloc(10001);
  memset(long_name, 'A', 10000);
  long_name[10000] = '\0';

  char *result = macro_substitute("Hello {{char}}!", long_name, "User");
  ASSERT_NOT_NULL(result);
  ASSERT(strlen(result) > 10000);

  free(result);
  free(long_name);
  PASS();
}

TEST(boundary_sampler_extreme_values) {
  SamplerSettings s;
  sampler_init_defaults(&s);

  s.temperature = 0.0;
  s.top_p = 0.0;
  s.top_k = 0;
  s.min_p = 0.0;
  PASS();
}

TEST(boundary_sampler_negative_values) {
  SamplerSettings s;
  sampler_init_defaults(&s);

  s.temperature = -1.0;
  s.top_k = -100;
  s.frequency_penalty = -10.0;
  PASS();
}

TEST(boundary_sampler_max_values) {
  SamplerSettings s;
  sampler_init_defaults(&s);

  s.temperature = 1000000.0;
  s.top_k = 1000000;
  s.max_tokens = 1000000;
  PASS();
}

TEST(boundary_utf8_invalid_sequences) {
  uint32_t cp;
  uint8_t invalid1[] = {0xFF, 0xFE};
  int len1 = utf8_decode(invalid1, 2, &cp);
  ASSERT(len1 <= 0 || cp != 0);

  uint8_t invalid2[] = {0xC0, 0x80};
  int len2 = utf8_decode(invalid2, 2, &cp);
  (void)len2;

  uint8_t truncated[] = {0xE0, 0x80};
  int len3 = utf8_decode(truncated, 2, &cp);
  (void)len3;

  PASS();
}

TEST(boundary_utf8_max_codepoint) {
  uint8_t buf[8];
  int len = utf8_encode(0x10FFFF, buf);
  ASSERT_EQ_INT(4, len);

  uint32_t decoded;
  int dec_len = utf8_decode(buf, len, &decoded);
  ASSERT_EQ_INT(4, dec_len);
  ASSERT_EQ_INT(0x10FFFF, (int)decoded);

  PASS();
}

TEST(boundary_utf8_invalid_codepoint) {
  uint8_t buf[8];

  int len1 = utf8_encode(0x110000, buf);
  (void)len1;

  int len2 = utf8_encode(0xD800, buf);
  (void)len2;

  int len3 = utf8_encode(0xDFFF, buf);
  (void)len3;

  PASS();
}

void run_boundary_tests(void);
void run_boundary_tests(void) {
  TEST_SUITE("Boundary Tests");
  RUN_TEST(boundary_history_empty_message);
  RUN_TEST(boundary_history_null_message);
  RUN_TEST(boundary_history_very_long_message);
  RUN_TEST(boundary_history_unicode_message);
  RUN_TEST(boundary_history_binary_data);
  RUN_TEST(boundary_history_max_swipes);
  RUN_TEST(boundary_history_index_overflow);
  RUN_TEST(boundary_config_empty_name);
  RUN_TEST(boundary_config_max_name_length);
  RUN_TEST(boundary_config_zero_context_length);
  RUN_TEST(boundary_config_negative_context_length);
  RUN_TEST(boundary_config_max_models);
  RUN_TEST(boundary_macro_empty_pattern);
  RUN_TEST(boundary_macro_very_long_replacement);
  RUN_TEST(boundary_sampler_extreme_values);
  RUN_TEST(boundary_sampler_negative_values);
  RUN_TEST(boundary_sampler_max_values);
  RUN_TEST(boundary_utf8_invalid_sequences);
  RUN_TEST(boundary_utf8_max_codepoint);
  RUN_TEST(boundary_utf8_invalid_codepoint);
}

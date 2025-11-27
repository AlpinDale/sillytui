#include "../test_framework.h"
#include "core/config.h"
#include "test_helper.h"

TEST(config_save_and_load_models) {
  setup_test_environment();

  ModelsFile mf1 = {0};
  mf1.active_index = -1;

  ModelConfig m1 = {0};
  snprintf(m1.name, sizeof(m1.name), "GPT-4");
  snprintf(m1.base_url, sizeof(m1.base_url), "https://api.openai.com/v1");
  snprintf(m1.api_key, sizeof(m1.api_key), "sk-test-key");
  snprintf(m1.model_id, sizeof(m1.model_id), "gpt-4-turbo");
  m1.context_length = 128000;
  m1.api_type = API_TYPE_OPENAI;

  ModelConfig m2 = {0};
  snprintf(m2.name, sizeof(m2.name), "Claude");
  snprintf(m2.base_url, sizeof(m2.base_url), "https://api.anthropic.com");
  snprintf(m2.api_key, sizeof(m2.api_key), "sk-ant-test");
  snprintf(m2.model_id, sizeof(m2.model_id), "claude-3-opus");
  m2.context_length = 200000;
  m2.api_type = API_TYPE_ANTHROPIC;

  config_add_model(&mf1, &m1);
  config_add_model(&mf1, &m2);
  config_set_active(&mf1, 1);

  bool saved = config_save_models(&mf1);
  ASSERT_TRUE(saved);

  ModelsFile mf2 = {0};
  bool loaded = config_load_models(&mf2);
  ASSERT_TRUE(loaded);

  ASSERT_EQ_SIZE(2, mf2.count);
  ASSERT_EQ_INT(1, mf2.active_index);

  ASSERT_EQ_STR("GPT-4", mf2.models[0].name);
  ASSERT_EQ_STR("https://api.openai.com/v1", mf2.models[0].base_url);
  ASSERT_EQ_STR("sk-test-key", mf2.models[0].api_key);
  ASSERT_EQ_STR("gpt-4-turbo", mf2.models[0].model_id);
  ASSERT_EQ_INT(128000, mf2.models[0].context_length);
  ASSERT_EQ(API_TYPE_OPENAI, mf2.models[0].api_type);

  ASSERT_EQ_STR("Claude", mf2.models[1].name);
  ASSERT_EQ(API_TYPE_ANTHROPIC, mf2.models[1].api_type);

  teardown_test_environment();
  PASS();
}

TEST(config_load_empty_returns_true) {
  setup_test_environment();

  ModelsFile mf = {0};
  bool loaded = config_load_models(&mf);
  ASSERT_TRUE(loaded);
  ASSERT_EQ_SIZE(0, mf.count);
  ASSERT_EQ_INT(-1, mf.active_index);

  teardown_test_environment();
  PASS();
}

TEST(config_ensure_dir_creates_directory) {
  setup_test_environment();

  bool created = config_ensure_dir();
  ASSERT_TRUE(created);

  char path[600];
  snprintf(path, sizeof(path), "%s/.config/sillytui", g_test_home);
  ASSERT_TRUE(file_exists(path));

  teardown_test_environment();
  PASS();
}

TEST(config_save_settings) {
  setup_test_environment();

  AppSettings s1 = {0};
  s1.skip_exit_confirm = true;

  bool saved = config_save_settings(&s1);
  ASSERT_TRUE(saved);

  AppSettings s2 = {0};
  bool loaded = config_load_settings(&s2);
  ASSERT_TRUE(loaded);
  ASSERT_TRUE(s2.skip_exit_confirm);

  teardown_test_environment();
  PASS();
}

TEST(config_special_characters_in_values) {
  setup_test_environment();

  ModelsFile mf1 = {0};
  mf1.active_index = 0;

  ModelConfig m = {0};
  snprintf(m.name, sizeof(m.name), "Test \"Model\" with 'quotes'");
  snprintf(m.base_url, sizeof(m.base_url), "https://example.com/path?a=1&b=2");
  snprintf(m.api_key, sizeof(m.api_key), "key-with-special-chars:!/");
  m.context_length = 8192;
  m.api_type = API_TYPE_APHRODITE;

  config_add_model(&mf1, &m);
  config_save_models(&mf1);

  ModelsFile mf2 = {0};
  config_load_models(&mf2);

  ASSERT_EQ_SIZE(1, mf2.count);
  ASSERT_EQ_STR("https://example.com/path?a=1&b=2", mf2.models[0].base_url);

  teardown_test_environment();
  PASS();
}

TEST(config_many_models) {
  setup_test_environment();

  ModelsFile mf1 = {0};
  mf1.active_index = -1;

  for (int i = 0; i < 10; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 5000 + i);
    snprintf(m.model_id, sizeof(m.model_id), "model-%d", i);
    m.context_length = 4096 * (i + 1);
    m.api_type = (ApiType)(i % API_TYPE_COUNT);
    config_add_model(&mf1, &m);
  }

  config_set_active(&mf1, 5);
  config_save_models(&mf1);

  ModelsFile mf2 = {0};
  config_load_models(&mf2);

  ASSERT_EQ_SIZE(10, mf2.count);
  ASSERT_EQ_INT(5, mf2.active_index);

  for (int i = 0; i < 10; i++) {
    char expected_name[64];
    snprintf(expected_name, sizeof(expected_name), "Model %d", i);
    ASSERT_EQ_STR(expected_name, mf2.models[i].name);
  }

  teardown_test_environment();
  PASS();
}

void run_config_integration_tests(void) {
  TEST_SUITE("Config Integration");
  RUN_TEST(config_save_and_load_models);
  RUN_TEST(config_load_empty_returns_true);
  RUN_TEST(config_ensure_dir_creates_directory);
  RUN_TEST(config_save_settings);
  RUN_TEST(config_special_characters_in_values);
  RUN_TEST(config_many_models);
}

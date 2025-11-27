#include "core/config.h"
#include "test_framework.h"

TEST(api_type_name_valid) {
  ASSERT_EQ_STR("openai", api_type_name(API_TYPE_OPENAI));
  ASSERT_EQ_STR("aphrodite", api_type_name(API_TYPE_APHRODITE));
  ASSERT_EQ_STR("vllm", api_type_name(API_TYPE_VLLM));
  ASSERT_EQ_STR("llamacpp", api_type_name(API_TYPE_LLAMACPP));
  ASSERT_EQ_STR("koboldcpp", api_type_name(API_TYPE_KOBOLDCPP));
  ASSERT_EQ_STR("tabby", api_type_name(API_TYPE_TABBY));
  ASSERT_EQ_STR("anthropic", api_type_name(API_TYPE_ANTHROPIC));
  PASS();
}

TEST(api_type_name_invalid) {
  ASSERT_EQ_STR("openai", api_type_name((ApiType)100));
  ASSERT_EQ_STR("openai", api_type_name((ApiType)-1));
  PASS();
}

TEST(api_type_from_name_valid) {
  ASSERT_EQ(API_TYPE_OPENAI, api_type_from_name("openai"));
  ASSERT_EQ(API_TYPE_APHRODITE, api_type_from_name("aphrodite"));
  ASSERT_EQ(API_TYPE_VLLM, api_type_from_name("vllm"));
  ASSERT_EQ(API_TYPE_LLAMACPP, api_type_from_name("llamacpp"));
  ASSERT_EQ(API_TYPE_KOBOLDCPP, api_type_from_name("koboldcpp"));
  ASSERT_EQ(API_TYPE_TABBY, api_type_from_name("tabby"));
  ASSERT_EQ(API_TYPE_ANTHROPIC, api_type_from_name("anthropic"));
  PASS();
}

TEST(api_type_from_name_case_insensitive) {
  ASSERT_EQ(API_TYPE_OPENAI, api_type_from_name("OPENAI"));
  ASSERT_EQ(API_TYPE_OPENAI, api_type_from_name("OpenAI"));
  ASSERT_EQ(API_TYPE_APHRODITE, api_type_from_name("APHRODITE"));
  ASSERT_EQ(API_TYPE_ANTHROPIC, api_type_from_name("ANTHROPIC"));
  PASS();
}

TEST(api_type_from_name_invalid) {
  ASSERT_EQ(API_TYPE_APHRODITE, api_type_from_name("invalid"));
  ASSERT_EQ(API_TYPE_APHRODITE, api_type_from_name(""));
  ASSERT_EQ(API_TYPE_APHRODITE, api_type_from_name(NULL));
  PASS();
}

TEST(config_add_model_basic) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Test Model");
  snprintf(mc.base_url, sizeof(mc.base_url), "http://localhost:5000");
  snprintf(mc.api_key, sizeof(mc.api_key), "test-key");
  snprintf(mc.model_id, sizeof(mc.model_id), "gpt-4");
  mc.context_length = 8192;
  mc.api_type = API_TYPE_OPENAI;

  ASSERT_TRUE(config_add_model(&mf, &mc));
  ASSERT_EQ_SIZE(1, mf.count);
  ASSERT_EQ_STR("Test Model", mf.models[0].name);
  ASSERT_EQ_STR("http://localhost:5000", mf.models[0].base_url);
  ASSERT_EQ_STR("test-key", mf.models[0].api_key);
  ASSERT_EQ_STR("gpt-4", mf.models[0].model_id);
  ASSERT_EQ_INT(8192, mf.models[0].context_length);
  ASSERT_EQ(API_TYPE_OPENAI, mf.models[0].api_type);
  PASS();
}

TEST(config_add_model_max_capacity) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  for (size_t i = 0; i < MAX_MODELS; i++) {
    ASSERT_TRUE(config_add_model(&mf, &mc));
  }
  ASSERT_EQ_SIZE(MAX_MODELS, mf.count);
  ASSERT_FALSE(config_add_model(&mf, &mc));
  ASSERT_EQ_SIZE(MAX_MODELS, mf.count);
  PASS();
}

TEST(config_remove_model_basic) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig mc1 = {0}, mc2 = {0}, mc3 = {0};
  snprintf(mc1.name, sizeof(mc1.name), "Model 1");
  snprintf(mc2.name, sizeof(mc2.name), "Model 2");
  snprintf(mc3.name, sizeof(mc3.name), "Model 3");
  config_add_model(&mf, &mc1);
  config_add_model(&mf, &mc2);
  config_add_model(&mf, &mc3);

  ASSERT_TRUE(config_remove_model(&mf, 1));
  ASSERT_EQ_SIZE(2, mf.count);
  ASSERT_EQ_STR("Model 1", mf.models[0].name);
  ASSERT_EQ_STR("Model 3", mf.models[1].name);
  PASS();
}

TEST(config_remove_model_first) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig mc1 = {0}, mc2 = {0};
  snprintf(mc1.name, sizeof(mc1.name), "First");
  snprintf(mc2.name, sizeof(mc2.name), "Second");
  config_add_model(&mf, &mc1);
  config_add_model(&mf, &mc2);

  ASSERT_TRUE(config_remove_model(&mf, 0));
  ASSERT_EQ_SIZE(1, mf.count);
  ASSERT_EQ_STR("Second", mf.models[0].name);
  PASS();
}

TEST(config_remove_model_updates_active_index) {
  ModelsFile mf = {0};
  mf.active_index = 2;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  config_add_model(&mf, &mc);
  config_add_model(&mf, &mc);
  config_add_model(&mf, &mc);

  config_remove_model(&mf, 0);
  ASSERT_EQ_INT(1, mf.active_index);
  PASS();
}

TEST(config_remove_model_clears_active_if_removed) {
  ModelsFile mf = {0};
  mf.active_index = 1;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  config_add_model(&mf, &mc);
  config_add_model(&mf, &mc);
  config_add_model(&mf, &mc);

  config_remove_model(&mf, 1);
  ASSERT_EQ_INT(-1, mf.active_index);
  PASS();
}

TEST(config_remove_model_invalid_index) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  config_add_model(&mf, &mc);

  ASSERT_FALSE(config_remove_model(&mf, 5));
  ASSERT_EQ_SIZE(1, mf.count);
  PASS();
}

TEST(config_get_active_valid) {
  ModelsFile mf = {0};
  mf.active_index = 0;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Active Model");
  config_add_model(&mf, &mc);

  ModelConfig *active = config_get_active(&mf);
  ASSERT_NOT_NULL(active);
  ASSERT_EQ_STR("Active Model", active->name);
  PASS();
}

TEST(config_get_active_none_selected) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  config_add_model(&mf, &mc);

  ASSERT_NULL(config_get_active(&mf));
  PASS();
}

TEST(config_get_active_invalid_index) {
  ModelsFile mf = {0};
  mf.active_index = 10;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  config_add_model(&mf, &mc);

  ASSERT_NULL(config_get_active(&mf));
  PASS();
}

TEST(config_set_active) {
  ModelsFile mf = {0};
  mf.active_index = -1;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  config_add_model(&mf, &mc);
  config_add_model(&mf, &mc);

  config_set_active(&mf, 1);
  ASSERT_EQ_INT(1, mf.active_index);
  config_set_active(&mf, 0);
  ASSERT_EQ_INT(0, mf.active_index);
  PASS();
}

TEST(config_set_active_invalid_noop) {
  ModelsFile mf = {0};
  mf.active_index = 0;
  ModelConfig mc = {0};
  snprintf(mc.name, sizeof(mc.name), "Model");
  config_add_model(&mf, &mc);

  config_set_active(&mf, 10);
  ASSERT_EQ_INT(0, mf.active_index);
  PASS();
}

void run_config_tests(void) {
  TEST_SUITE("Configuration");
  RUN_TEST(api_type_name_valid);
  RUN_TEST(api_type_name_invalid);
  RUN_TEST(api_type_from_name_valid);
  RUN_TEST(api_type_from_name_case_insensitive);
  RUN_TEST(api_type_from_name_invalid);
  RUN_TEST(config_add_model_basic);
  RUN_TEST(config_add_model_max_capacity);
  RUN_TEST(config_remove_model_basic);
  RUN_TEST(config_remove_model_first);
  RUN_TEST(config_remove_model_updates_active_index);
  RUN_TEST(config_remove_model_clears_active_if_removed);
  RUN_TEST(config_remove_model_invalid_index);
  RUN_TEST(config_get_active_valid);
  RUN_TEST(config_get_active_none_selected);
  RUN_TEST(config_get_active_invalid_index);
  RUN_TEST(config_set_active);
  RUN_TEST(config_set_active_invalid_noop);
}

#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"core/config.h\"\n");
  printf("#include <stdlib.h>\n");
  printf("#include <string.h>\n\n");

  for (int n = 1; n <= 32; n++) {
    printf("TEST(config_add_%d_models) {\n", n);
    printf("  ModelsFile mf = {0};\n");
    printf("  for (int i = 0; i < %d; i++) {\n", n);
    printf("    ModelConfig m = {0};\n");
    printf("    snprintf(m.name, sizeof(m.name), \"Model %%d\", i);\n");
    printf("    snprintf(m.base_url, sizeof(m.base_url), "
           "\"http://localhost:%%d\", 8000 + i);\n");
    printf("    m.context_length = 4096 + i;\n");
    printf("    m.api_type = i %% 7;\n");
    printf("    ASSERT_TRUE(config_add_model(&mf, &m));\n");
    printf("  }\n");
    printf("  ASSERT_EQ_SIZE(%d, mf.count);\n", n);
    printf("  for (int i = 0; i < %d; i++) {\n", n);
    printf("    char expected[64];\n");
    printf("    snprintf(expected, sizeof(expected), \"Model %%d\", i);\n");
    printf("    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));\n");
    printf("  }\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int n = 1; n <= 20; n++) {
    printf("TEST(config_remove_all_%d_models) {\n", n);
    printf("  ModelsFile mf = {0};\n");
    printf("  for (int i = 0; i < %d; i++) {\n", n);
    printf("    ModelConfig m = {0};\n");
    printf("    snprintf(m.name, sizeof(m.name), \"Model %%d\", i);\n");
    printf("    config_add_model(&mf, &m);\n");
    printf("  }\n");
    printf("  for (int i = 0; i < %d; i++) {\n", n);
    printf("    ASSERT_TRUE(config_remove_model(&mf, 0));\n");
    printf("  }\n");
    printf("  ASSERT_EQ_SIZE(0, mf.count);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(config_api_type_roundtrip) {\n");
  printf("  for (int t = 0; t < API_TYPE_COUNT; t++) {\n");
  printf("    const char *name = api_type_name((ApiType)t);\n");
  printf("    ASSERT_NOT_NULL(name);\n");
  printf("    ApiType parsed = api_type_from_name(name);\n");
  printf("    ASSERT_EQ_INT(t, (int)parsed);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(config_active_index_tracking) {\n");
  printf("  ModelsFile mf = {0};\n");
  printf("  for (int i = 0; i < 10; i++) {\n");
  printf("    ModelConfig m = {0};\n");
  printf("    snprintf(m.name, sizeof(m.name), \"Model %%d\", i);\n");
  printf("    config_add_model(&mf, &m);\n");
  printf("  }\n");
  printf("  for (size_t i = 0; i < 10; i++) {\n");
  printf("    config_set_active(&mf, i);\n");
  printf("    ASSERT_EQ_SIZE(i, mf.active_index);\n");
  printf("    ModelConfig *active = config_get_active(&mf);\n");
  printf("    ASSERT_NOT_NULL(active);\n");
  printf("    char expected[64];\n");
  printf("    snprintf(expected, sizeof(expected), \"Model %%zu\", i);\n");
  printf("    ASSERT_EQ_INT(0, strcmp(active->name, expected));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(config_remove_updates_active) {\n");
  printf("  ModelsFile mf = {0};\n");
  printf("  for (int i = 0; i < 5; i++) {\n");
  printf("    ModelConfig m = {0};\n");
  printf("    snprintf(m.name, sizeof(m.name), \"Model %%d\", i);\n");
  printf("    config_add_model(&mf, &m);\n");
  printf("  }\n");
  printf("  config_set_active(&mf, 2);\n");
  printf("  config_remove_model(&mf, 0);\n");
  printf("  ASSERT_EQ_SIZE(1, mf.active_index);\n");
  printf("  config_remove_model(&mf, 0);\n");
  printf("  ASSERT_EQ_SIZE(0, mf.active_index);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(config_context_length_values) {\n");
  printf("  int values[] = {1, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, "
         "32768, 65536, 131072};\n");
  printf("  ModelsFile mf = {0};\n");
  printf("  for (size_t i = 0; i < 12; i++) {\n");
  printf("    ModelConfig m = {0};\n");
  printf("    m.context_length = values[i];\n");
  printf("    snprintf(m.name, sizeof(m.name), \"Ctx%%d\", values[i]);\n");
  printf("    config_add_model(&mf, &m);\n");
  printf("    ASSERT_EQ_INT(values[i], mf.models[i].context_length);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_config_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"Config Generated Tests\");\n");
  for (int n = 1; n <= 32; n++) {
    printf("  RUN_TEST(config_add_%d_models);\n", n);
  }
  for (int n = 1; n <= 20; n++) {
    printf("  RUN_TEST(config_remove_all_%d_models);\n", n);
  }
  printf("  RUN_TEST(config_api_type_roundtrip);\n");
  printf("  RUN_TEST(config_active_index_tracking);\n");
  printf("  RUN_TEST(config_remove_updates_active);\n");
  printf("  RUN_TEST(config_context_length_values);\n");
  printf("}\n");

  return 0;
}

#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"tokenizer/tiktoken.h\"\n");
  printf("#include \"chat/history.h\"\n");
  printf("#include \"core/config.h\"\n");
  printf("#include <stdlib.h>\n");
  printf("#include <string.h>\n");
  printf("#include <limits.h>\n\n");

  for (int len = 0; len <= 64; len++) {
    printf("TEST(edge_utf8_sequence_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len > 0 ? len : 1);
    printf("  for (int i = 0; i < %d; i++) buf[i] = 'x';\n", len);
    printf("  uint32_t cp;\n");
    printf("  int result = utf8_decode(buf, %d, &cp);\n", len);
    if (len == 0) {
      printf("  ASSERT_EQ_INT(0, result);\n");
    } else {
      printf("  ASSERT(result >= 1);\n");
    }
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int first = 0xC0; first <= 0xFF; first++) {
    printf("TEST(edge_utf8_leading_byte_0x%02X) {\n", first);
    printf("  uint8_t buf[4] = {0x%02X, 0x80, 0x80, 0x80};\n", first);
    printf("  uint32_t cp;\n");
    printf("  int result = utf8_decode(buf, 4, &cp);\n");
    printf("  ASSERT(result >= 1);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(edge_history_100_messages) {\n");
  printf("  ChatHistory h;\n");
  printf("  history_init(&h);\n");
  printf("  for (int i = 0; i < 100; i++) {\n");
  printf("    char buf[32];\n");
  printf("    snprintf(buf, sizeof(buf), \"Msg %%d\", i);\n");
  printf("    history_add(&h, buf);\n");
  printf("  }\n");
  printf("  ASSERT_EQ_SIZE(100, h.count);\n");
  printf("  for (int i = 99; i >= 0; i--) {\n");
  printf("    const char *msg = history_get(&h, (size_t)i);\n");
  printf("    ASSERT_NOT_NULL(msg);\n");
  printf("  }\n");
  printf("  history_free(&h);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(edge_history_swipe_stress) {\n");
  printf("  ChatHistory h;\n");
  printf("  history_init(&h);\n");
  printf("  size_t idx = history_add(&h, \"Original\");\n");
  printf("  for (int i = 0; i < 32; i++) {\n");
  printf("    char buf[32];\n");
  printf("    snprintf(buf, sizeof(buf), \"Swipe %%d\", i);\n");
  printf("    history_add_swipe(&h, idx, buf);\n");
  printf("  }\n");
  printf("  for (int i = 0; i < 33; i++) {\n");
  printf("    ASSERT_TRUE(history_set_active_swipe(&h, idx, (size_t)i));\n");
  printf("    const char *swipe = history_get_swipe(&h, idx, (size_t)i);\n");
  printf("    ASSERT_NOT_NULL(swipe);\n");
  printf("  }\n");
  printf("  history_free(&h);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(edge_history_interleaved_ops) {\n");
  printf("  ChatHistory h;\n");
  printf("  history_init(&h);\n");
  printf("  for (int iter = 0; iter < 50; iter++) {\n");
  printf("    history_add(&h, \"A\");\n");
  printf("    history_add(&h, \"B\");\n");
  printf("    history_delete(&h, 0);\n");
  printf("    history_update(&h, 0, \"Updated\");\n");
  printf("  }\n");
  printf("  history_free(&h);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(edge_config_max_models) {\n");
  printf("  ModelsFile mf = {0};\n");
  printf("  for (int i = 0; i < 32; i++) {\n");
  printf("    ModelConfig m = {0};\n");
  printf("    snprintf(m.name, sizeof(m.name), \"M%%d\", i);\n");
  printf("    ASSERT_TRUE(config_add_model(&mf, &m));\n");
  printf("  }\n");
  printf("  ModelConfig extra = {0};\n");
  printf("  snprintf(extra.name, sizeof(extra.name), \"Extra\");\n");
  printf("  ASSERT_FALSE(config_add_model(&mf, &extra));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(edge_config_set_active_boundary) {\n");
  printf("  ModelsFile mf = {0};\n");
  printf("  for (int i = 0; i < 5; i++) {\n");
  printf("    ModelConfig m = {0};\n");
  printf("    snprintf(m.name, sizeof(m.name), \"M%%d\", i);\n");
  printf("    config_add_model(&mf, &m);\n");
  printf("  }\n");
  printf("  config_set_active(&mf, 0);\n");
  printf("  ASSERT_EQ_INT(0, mf.active_index);\n");
  printf("  config_set_active(&mf, 4);\n");
  printf("  ASSERT_EQ_INT(4, mf.active_index);\n");
  printf("  config_set_active(&mf, 100);\n");
  printf("  ASSERT_EQ_INT(4, mf.active_index);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(edge_pretokenize_unicode_mix) {\n");
  printf("  SpanList spans = {0};\n");
  printf("  const char *inputs[] = {\n");
  printf("    \"Hello日本語\",\n");
  printf("    \"日本語Hello\",\n");
  printf("    \"Hello 日本語 World\",\n");
  printf("    \"123日本456\",\n");
  printf("    \"日本123語\",\n");
  printf("    \" 日本 \",\n");
  printf("    \"日\",\n");
  printf("    \"日本\",\n");
  printf("  };\n");
  printf("  for (size_t i = 0; i < 8; i++) {\n");
  printf("    spans.count = 0;\n");
  printf("    int result = pretokenize_cl100k(inputs[i], &spans);\n");
  printf("    ASSERT(result >= 0);\n");
  printf("    size_t covered = 0;\n");
  printf("    for (size_t j = 0; j < spans.count; j++) {\n");
  printf("      covered += spans.spans[j].end - spans.spans[j].start;\n");
  printf("    }\n");
  printf("    ASSERT_EQ_SIZE(strlen(inputs[i]), covered);\n");
  printf("    free(spans.spans);\n");
  printf("    spans.spans = NULL;\n");
  printf("    spans.cap = 0;\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(edge_pretokenize_special_chars) {\n");
  printf("  SpanList spans = {0};\n");
  printf("  const char *inputs[] = {\n");
  printf("    \"\\n\\n\\n\",\n");
  printf("    \"\\r\\n\\r\\n\",\n");
  printf("    \"\\t\\t\\t\",\n");
  printf("    \"   \\n   \",\n");
  printf("    \"a\\nb\\nc\",\n");
  printf("    \"line1\\r\\nline2\",\n");
  printf("  };\n");
  printf("  for (size_t i = 0; i < 6; i++) {\n");
  printf("    spans.count = 0;\n");
  printf("    int result = pretokenize_cl100k(inputs[i], &spans);\n");
  printf("    ASSERT(result >= 0);\n");
  printf("    free(spans.spans);\n");
  printf("    spans.spans = NULL;\n");
  printf("    spans.cap = 0;\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  for (int len = 1; len <= 100; len += 10) {
    printf("TEST(edge_pretokenize_spaces_%d) {\n", len);
    printf("  char buf[%d];\n", len + 1);
    printf("  for (int i = 0; i < %d; i++) buf[i] = ' ';\n", len);
    printf("  buf[%d] = '\\0';\n", len);
    printf("  SpanList spans = {0};\n");
    printf("  int result = pretokenize_cl100k(buf, &spans);\n");
    printf("  ASSERT(result >= 0);\n");
    printf("  free(spans.spans);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int len = 1; len <= 100; len += 10) {
    printf("TEST(edge_pretokenize_digits_%d) {\n", len);
    printf("  char buf[%d];\n", len + 1);
    printf("  for (int i = 0; i < %d; i++) buf[i] = '0' + (i %% 10);\n", len);
    printf("  buf[%d] = '\\0';\n", len);
    printf("  SpanList spans = {0};\n");
    printf("  int result = pretokenize_cl100k(buf, &spans);\n");
    printf("  ASSERT(result >= 0);\n");
    printf("  free(spans.spans);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(edge_api_type_all_values) {\n");
  printf("  for (int t = 0; t < API_TYPE_COUNT; t++) {\n");
  printf("    const char *name = api_type_name((ApiType)t);\n");
  printf("    ASSERT_NOT_NULL(name);\n");
  printf("    ASSERT(strlen(name) > 0);\n");
  printf("    ApiType parsed = api_type_from_name(name);\n");
  printf("    ASSERT_EQ_INT(t, (int)parsed);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(edge_api_type_invalid_names) {\n");
  printf("  const char *invalid[] = {\"foo\", \"bar\", \"baz\", \"unknown\", "
         "\"test\", NULL};\n");
  printf("  for (int i = 0; invalid[i] != NULL; i++) {\n");
  printf("    ApiType t = api_type_from_name(invalid[i]);\n");
  printf("    ASSERT_EQ_INT(API_TYPE_APHRODITE, (int)t);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_edge_case_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"Edge Case Generated Tests\");\n");
  for (int len = 0; len <= 64; len++) {
    printf("  RUN_TEST(edge_utf8_sequence_len_%d);\n", len);
  }
  for (int first = 0xC0; first <= 0xFF; first++) {
    printf("  RUN_TEST(edge_utf8_leading_byte_0x%02X);\n", first);
  }
  printf("  RUN_TEST(edge_history_100_messages);\n");
  printf("  RUN_TEST(edge_history_swipe_stress);\n");
  printf("  RUN_TEST(edge_history_interleaved_ops);\n");
  printf("  RUN_TEST(edge_config_max_models);\n");
  printf("  RUN_TEST(edge_config_set_active_boundary);\n");
  printf("  RUN_TEST(edge_pretokenize_unicode_mix);\n");
  printf("  RUN_TEST(edge_pretokenize_special_chars);\n");
  for (int len = 1; len <= 100; len += 10) {
    printf("  RUN_TEST(edge_pretokenize_spaces_%d);\n", len);
    printf("  RUN_TEST(edge_pretokenize_digits_%d);\n", len);
  }
  printf("  RUN_TEST(edge_api_type_all_values);\n");
  printf("  RUN_TEST(edge_api_type_invalid_names);\n");
  printf("}\n");

  return 0;
}

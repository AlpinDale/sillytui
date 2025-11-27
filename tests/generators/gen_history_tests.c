#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"chat/history.h\"\n");
  printf("#include <stdlib.h>\n");
  printf("#include <string.h>\n\n");

  for (int n = 1; n <= 100; n++) {
    printf("TEST(history_add_%d_messages) {\n", n);
    printf("  ChatHistory h;\n");
    printf("  history_init(&h);\n");
    printf("  for (int i = 0; i < %d; i++) {\n", n);
    printf("    char buf[32];\n");
    printf("    snprintf(buf, sizeof(buf), \"Message %%d\", i);\n");
    printf("    size_t idx = history_add(&h, buf);\n");
    printf("    ASSERT_EQ_SIZE((size_t)i, idx);\n");
    printf("  }\n");
    printf("  ASSERT_EQ_SIZE(%d, h.count);\n", n);
    printf("  history_free(&h);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int n = 1; n <= 20; n++) {
    printf("TEST(history_delete_from_%d_messages) {\n", n);
    printf("  ChatHistory h;\n");
    printf("  history_init(&h);\n");
    printf("  for (int i = 0; i < %d; i++) {\n", n);
    printf("    char buf[32];\n");
    printf("    snprintf(buf, sizeof(buf), \"Message %%d\", i);\n");
    printf("    history_add(&h, buf);\n");
    printf("  }\n");
    printf("  ASSERT_EQ_SIZE(%d, h.count);\n", n);
    printf("  for (size_t i = 0; i < %d; i++) {\n", n);
    printf("    ASSERT_TRUE(history_delete(&h, 0));\n");
    printf("    ASSERT_EQ_SIZE(%d - i - 1, h.count);\n", n);
    printf("  }\n");
    printf("  ASSERT_EQ_SIZE(0, h.count);\n");
    printf("  history_free(&h);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int swipes = 1; swipes <= 16; swipes++) {
    printf("TEST(history_swipes_%d_per_message) {\n", swipes);
    printf("  ChatHistory h;\n");
    printf("  history_init(&h);\n");
    printf("  size_t idx = history_add(&h, \"Original\");\n");
    printf("  for (int s = 0; s < %d; s++) {\n", swipes);
    printf("    char buf[32];\n");
    printf("    snprintf(buf, sizeof(buf), \"Swipe %%d\", s);\n");
    printf("    history_add_swipe(&h, idx, buf);\n");
    printf("  }\n");
    printf("  ASSERT_EQ_SIZE(%d, history_get_swipe_count(&h, idx));\n",
           swipes + 1);
    printf("  for (int s = 0; s <= %d; s++) {\n", swipes);
    printf("    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));\n");
    printf(
        "    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));\n");
    printf("  }\n");
    printf("  history_free(&h);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(history_update_all_messages) {\n");
  printf("  ChatHistory h;\n");
  printf("  history_init(&h);\n");
  printf("  for (int i = 0; i < 50; i++) {\n");
  printf("    char buf[32];\n");
  printf("    snprintf(buf, sizeof(buf), \"Original %%d\", i);\n");
  printf("    history_add(&h, buf);\n");
  printf("  }\n");
  printf("  for (int i = 0; i < 50; i++) {\n");
  printf("    char buf[32];\n");
  printf("    snprintf(buf, sizeof(buf), \"Updated %%d\", i);\n");
  printf("    history_update(&h, (size_t)i, buf);\n");
  printf("  }\n");
  printf("  for (int i = 0; i < 50; i++) {\n");
  printf("    char buf[32];\n");
  printf("    snprintf(buf, sizeof(buf), \"Updated %%d\", i);\n");
  printf("    const char *msg = history_get(&h, (size_t)i);\n");
  printf("    ASSERT_NOT_NULL(msg);\n");
  printf("    ASSERT_EQ_INT(0, strcmp(msg, buf));\n");
  printf("  }\n");
  printf("  history_free(&h);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(history_mixed_operations) {\n");
  printf("  ChatHistory h;\n");
  printf("  history_init(&h);\n");
  printf("  for (int iter = 0; iter < 100; iter++) {\n");
  printf("    size_t idx = history_add(&h, \"New\");\n");
  printf("    history_update(&h, idx, \"Updated\");\n");
  printf("    if (idx > 0) {\n");
  printf("      history_add_swipe(&h, 0, \"Swipe\");\n");
  printf("    }\n");
  printf("    if (h.count > 5) {\n");
  printf("      history_delete(&h, h.count / 2);\n");
  printf("    }\n");
  printf("  }\n");
  printf("  history_free(&h);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_history_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"History Generated Tests\");\n");
  for (int n = 1; n <= 100; n++) {
    printf("  RUN_TEST(history_add_%d_messages);\n", n);
  }
  for (int n = 1; n <= 20; n++) {
    printf("  RUN_TEST(history_delete_from_%d_messages);\n", n);
  }
  for (int swipes = 1; swipes <= 16; swipes++) {
    printf("  RUN_TEST(history_swipes_%d_per_message);\n", swipes);
  }
  printf("  RUN_TEST(history_update_all_messages);\n");
  printf("  RUN_TEST(history_mixed_operations);\n");
  printf("}\n");

  return 0;
}

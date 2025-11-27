#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"core/macros.h\"\n");
  printf("#include <stdlib.h>\n");
  printf("#include <string.h>\n\n");

  const char *macros[] = {"{{char}}", "{{Char}}", "{{CHAR}}", "{{user}}",
                          "{{User}}", "{{USER}}", "{{name}}", "{{bot}}"};
  int num_macros = 8;

  for (int i = 0; i < num_macros; i++) {
    printf("TEST(macro_gen_single_%d) {\n", i);
    printf("  char *result = macro_substitute(\"%s\", \"Alice\", \"Bob\");\n",
           macros[i]);
    printf("  ASSERT_NOT_NULL(result);\n");
    printf("  free(result);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int i = 0; i < num_macros; i++) {
    for (int j = 0; j < num_macros; j++) {
      printf("TEST(macro_gen_pair_%d_%d) {\n", i, j);
      printf(
          "  char *result = macro_substitute(\"%s %s\", \"Alice\", \"Bob\");\n",
          macros[i], macros[j]);
      printf("  ASSERT_NOT_NULL(result);\n");
      printf("  free(result);\n");
      printf("  PASS();\n");
      printf("}\n\n");
    }
  }

  for (int n = 1; n <= 10; n++) {
    printf("TEST(macro_gen_repeated_%d) {\n", n);
    printf("  char input[512] = \"\";\n");
    printf(
        "  for (int i = 0; i < %d; i++) strcat(input, \"Hello {{char}}! \");\n",
        n);
    printf("  char *result = macro_substitute(input, \"Alice\", \"Bob\");\n");
    printf("  ASSERT_NOT_NULL(result);\n");
    printf("  int count = 0;\n");
    printf("  const char *p = result;\n");
    printf("  while ((p = strstr(p, \"Alice\")) != NULL) { count++; p++; }\n");
    printf("  ASSERT_EQ_INT(%d, count);\n", n);
    printf("  free(result);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  const char *names[] = {"",      "A",           "Ab",        "Bob",
                         "Alice", "Christopher", "ABCDEFGHIJ"};
  int num_names = 7;

  for (int i = 0; i < num_names; i++) {
    printf("TEST(macro_gen_char_name_%d) {\n", i);
    printf("  char *result = macro_substitute(\"Hello {{char}}\", \"%s\", "
           "\"User\");\n",
           names[i]);
    printf("  ASSERT_NOT_NULL(result);\n");
    if (strlen(names[i]) > 0) {
      printf("  ASSERT(strstr(result, \"%s\") != NULL);\n", names[i]);
    }
    printf("  free(result);\n");
    printf("  PASS();\n");
    printf("}\n\n");

    printf("TEST(macro_gen_user_name_%d) {\n", i);
    printf("  char *result = macro_substitute(\"Hello {{user}}\", \"Char\", "
           "\"%s\");\n",
           names[i]);
    printf("  ASSERT_NOT_NULL(result);\n");
    if (strlen(names[i]) > 0) {
      printf("  ASSERT(strstr(result, \"%s\") != NULL);\n", names[i]);
    }
    printf("  free(result);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(macro_gen_no_macros_long) {\n");
  printf("  const char *input = \"The quick brown fox jumps over the lazy dog. "
         "\";\n");
  printf("  char long_input[2048] = \"\";\n");
  printf("  for (int i = 0; i < 40; i++) strcat(long_input, input);\n");
  printf(
      "  char *result = macro_substitute(long_input, \"Alice\", \"Bob\");\n");
  printf("  ASSERT_NOT_NULL(result);\n");
  printf("  ASSERT_EQ_INT(0, strcmp(result, long_input));\n");
  printf("  free(result);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(macro_gen_mixed_text_macros) {\n");
  printf("  const char *input = \"Hello {{char}}, this is {{user}}. How are "
         "you {{Char}}?\";\n");
  printf("  char *result = macro_substitute(input, \"Alice\", \"Bob\");\n");
  printf("  ASSERT_NOT_NULL(result);\n");
  printf("  ASSERT(strstr(result, \"Hello Alice\") != NULL);\n");
  printf("  ASSERT(strstr(result, \"this is Bob\") != NULL);\n");
  printf("  free(result);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(macro_gen_null_input) {\n");
  printf("  char *result = macro_substitute(NULL, \"Alice\", \"Bob\");\n");
  printf("  ASSERT_NULL(result);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(macro_gen_null_char_name) {\n");
  printf("  char *result = macro_substitute(\"Hello {{char}}\", NULL, "
         "\"Bob\");\n");
  printf("  ASSERT_NOT_NULL(result);\n");
  printf("  free(result);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(macro_gen_null_user_name) {\n");
  printf("  char *result = macro_substitute(\"Hello {{user}}\", \"Alice\", "
         "NULL);\n");
  printf("  ASSERT_NOT_NULL(result);\n");
  printf("  free(result);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(macro_gen_empty_input) {\n");
  printf("  char *result = macro_substitute(\"\", \"Alice\", \"Bob\");\n");
  printf("  ASSERT_NOT_NULL(result);\n");
  printf("  ASSERT_EQ_INT(0, (int)strlen(result));\n");
  printf("  free(result);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_macro_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"Macro Generated Tests\");\n");
  for (int i = 0; i < num_macros; i++) {
    printf("  RUN_TEST(macro_gen_single_%d);\n", i);
  }
  for (int i = 0; i < num_macros; i++) {
    for (int j = 0; j < num_macros; j++) {
      printf("  RUN_TEST(macro_gen_pair_%d_%d);\n", i, j);
    }
  }
  for (int n = 1; n <= 10; n++) {
    printf("  RUN_TEST(macro_gen_repeated_%d);\n", n);
  }
  for (int i = 0; i < num_names; i++) {
    printf("  RUN_TEST(macro_gen_char_name_%d);\n", i);
    printf("  RUN_TEST(macro_gen_user_name_%d);\n", i);
  }
  printf("  RUN_TEST(macro_gen_no_macros_long);\n");
  printf("  RUN_TEST(macro_gen_mixed_text_macros);\n");
  printf("  RUN_TEST(macro_gen_null_input);\n");
  printf("  RUN_TEST(macro_gen_null_char_name);\n");
  printf("  RUN_TEST(macro_gen_null_user_name);\n");
  printf("  RUN_TEST(macro_gen_empty_input);\n");
  printf("}\n");

  return 0;
}

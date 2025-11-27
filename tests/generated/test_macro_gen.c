#include "core/macros.h"
#include "test_framework.h"
#include <stdlib.h>
#include <string.h>

TEST(macro_gen_single_0) {
  char *result = macro_substitute("{{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_single_1) {
  char *result = macro_substitute("{{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_single_2) {
  char *result = macro_substitute("{{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_single_3) {
  char *result = macro_substitute("{{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_single_4) {
  char *result = macro_substitute("{{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_single_5) {
  char *result = macro_substitute("{{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_single_6) {
  char *result = macro_substitute("{{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_single_7) {
  char *result = macro_substitute("{{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_0) {
  char *result = macro_substitute("{{char}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_1) {
  char *result = macro_substitute("{{char}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_2) {
  char *result = macro_substitute("{{char}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_3) {
  char *result = macro_substitute("{{char}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_4) {
  char *result = macro_substitute("{{char}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_5) {
  char *result = macro_substitute("{{char}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_6) {
  char *result = macro_substitute("{{char}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_0_7) {
  char *result = macro_substitute("{{char}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_0) {
  char *result = macro_substitute("{{Char}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_1) {
  char *result = macro_substitute("{{Char}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_2) {
  char *result = macro_substitute("{{Char}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_3) {
  char *result = macro_substitute("{{Char}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_4) {
  char *result = macro_substitute("{{Char}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_5) {
  char *result = macro_substitute("{{Char}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_6) {
  char *result = macro_substitute("{{Char}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_1_7) {
  char *result = macro_substitute("{{Char}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_0) {
  char *result = macro_substitute("{{CHAR}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_1) {
  char *result = macro_substitute("{{CHAR}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_2) {
  char *result = macro_substitute("{{CHAR}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_3) {
  char *result = macro_substitute("{{CHAR}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_4) {
  char *result = macro_substitute("{{CHAR}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_5) {
  char *result = macro_substitute("{{CHAR}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_6) {
  char *result = macro_substitute("{{CHAR}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_2_7) {
  char *result = macro_substitute("{{CHAR}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_0) {
  char *result = macro_substitute("{{user}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_1) {
  char *result = macro_substitute("{{user}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_2) {
  char *result = macro_substitute("{{user}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_3) {
  char *result = macro_substitute("{{user}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_4) {
  char *result = macro_substitute("{{user}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_5) {
  char *result = macro_substitute("{{user}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_6) {
  char *result = macro_substitute("{{user}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_3_7) {
  char *result = macro_substitute("{{user}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_0) {
  char *result = macro_substitute("{{User}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_1) {
  char *result = macro_substitute("{{User}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_2) {
  char *result = macro_substitute("{{User}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_3) {
  char *result = macro_substitute("{{User}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_4) {
  char *result = macro_substitute("{{User}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_5) {
  char *result = macro_substitute("{{User}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_6) {
  char *result = macro_substitute("{{User}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_4_7) {
  char *result = macro_substitute("{{User}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_0) {
  char *result = macro_substitute("{{USER}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_1) {
  char *result = macro_substitute("{{USER}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_2) {
  char *result = macro_substitute("{{USER}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_3) {
  char *result = macro_substitute("{{USER}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_4) {
  char *result = macro_substitute("{{USER}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_5) {
  char *result = macro_substitute("{{USER}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_6) {
  char *result = macro_substitute("{{USER}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_5_7) {
  char *result = macro_substitute("{{USER}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_0) {
  char *result = macro_substitute("{{name}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_1) {
  char *result = macro_substitute("{{name}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_2) {
  char *result = macro_substitute("{{name}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_3) {
  char *result = macro_substitute("{{name}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_4) {
  char *result = macro_substitute("{{name}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_5) {
  char *result = macro_substitute("{{name}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_6) {
  char *result = macro_substitute("{{name}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_6_7) {
  char *result = macro_substitute("{{name}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_0) {
  char *result = macro_substitute("{{bot}} {{char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_1) {
  char *result = macro_substitute("{{bot}} {{Char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_2) {
  char *result = macro_substitute("{{bot}} {{CHAR}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_3) {
  char *result = macro_substitute("{{bot}} {{user}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_4) {
  char *result = macro_substitute("{{bot}} {{User}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_5) {
  char *result = macro_substitute("{{bot}} {{USER}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_6) {
  char *result = macro_substitute("{{bot}} {{name}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_pair_7_7) {
  char *result = macro_substitute("{{bot}} {{bot}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_1) {
  char input[512] = "";
  for (int i = 0; i < 1; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(1, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_2) {
  char input[512] = "";
  for (int i = 0; i < 2; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(2, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_3) {
  char input[512] = "";
  for (int i = 0; i < 3; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(3, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_4) {
  char input[512] = "";
  for (int i = 0; i < 4; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(4, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_5) {
  char input[512] = "";
  for (int i = 0; i < 5; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(5, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_6) {
  char input[512] = "";
  for (int i = 0; i < 6; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(6, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_7) {
  char input[512] = "";
  for (int i = 0; i < 7; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(7, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_8) {
  char input[512] = "";
  for (int i = 0; i < 8; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(8, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_9) {
  char input[512] = "";
  for (int i = 0; i < 9; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(9, count);
  free(result);
  PASS();
}

TEST(macro_gen_repeated_10) {
  char input[512] = "";
  for (int i = 0; i < 10; i++)
    strcat(input, "Hello {{char}}! ");
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  int count = 0;
  const char *p = result;
  while ((p = strstr(p, "Alice")) != NULL) {
    count++;
    p++;
  }
  ASSERT_EQ_INT(10, count);
  free(result);
  PASS();
}

TEST(macro_gen_char_name_0) {
  char *result = macro_substitute("Hello {{char}}", "", "User");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_user_name_0) {
  char *result = macro_substitute("Hello {{user}}", "Char", "");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_char_name_1) {
  char *result = macro_substitute("Hello {{char}}", "A", "User");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "A") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_user_name_1) {
  char *result = macro_substitute("Hello {{user}}", "Char", "A");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "A") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_char_name_2) {
  char *result = macro_substitute("Hello {{char}}", "Ab", "User");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Ab") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_user_name_2) {
  char *result = macro_substitute("Hello {{user}}", "Char", "Ab");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Ab") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_char_name_3) {
  char *result = macro_substitute("Hello {{char}}", "Bob", "User");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Bob") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_user_name_3) {
  char *result = macro_substitute("Hello {{user}}", "Char", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Bob") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_char_name_4) {
  char *result = macro_substitute("Hello {{char}}", "Alice", "User");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Alice") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_user_name_4) {
  char *result = macro_substitute("Hello {{user}}", "Char", "Alice");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Alice") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_char_name_5) {
  char *result = macro_substitute("Hello {{char}}", "Christopher", "User");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Christopher") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_user_name_5) {
  char *result = macro_substitute("Hello {{user}}", "Char", "Christopher");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Christopher") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_char_name_6) {
  char *result = macro_substitute("Hello {{char}}", "ABCDEFGHIJ", "User");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "ABCDEFGHIJ") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_user_name_6) {
  char *result = macro_substitute("Hello {{user}}", "Char", "ABCDEFGHIJ");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "ABCDEFGHIJ") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_no_macros_long) {
  const char *input = "The quick brown fox jumps over the lazy dog. ";
  char long_input[2048] = "";
  for (int i = 0; i < 40; i++)
    strcat(long_input, input);
  char *result = macro_substitute(long_input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_INT(0, strcmp(result, long_input));
  free(result);
  PASS();
}

TEST(macro_gen_mixed_text_macros) {
  const char *input = "Hello {{char}}, this is {{user}}. How are you {{Char}}?";
  char *result = macro_substitute(input, "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT(strstr(result, "Hello Alice") != NULL);
  ASSERT(strstr(result, "this is Bob") != NULL);
  free(result);
  PASS();
}

TEST(macro_gen_null_input) {
  char *result = macro_substitute(NULL, "Alice", "Bob");
  ASSERT_NULL(result);
  PASS();
}

TEST(macro_gen_null_char_name) {
  char *result = macro_substitute("Hello {{char}}", NULL, "Bob");
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_null_user_name) {
  char *result = macro_substitute("Hello {{user}}", "Alice", NULL);
  ASSERT_NOT_NULL(result);
  free(result);
  PASS();
}

TEST(macro_gen_empty_input) {
  char *result = macro_substitute("", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_INT(0, (int)strlen(result));
  free(result);
  PASS();
}

void run_macro_generated_tests(void) {
  TEST_SUITE("Macro Generated Tests");
  RUN_TEST(macro_gen_single_0);
  RUN_TEST(macro_gen_single_1);
  RUN_TEST(macro_gen_single_2);
  RUN_TEST(macro_gen_single_3);
  RUN_TEST(macro_gen_single_4);
  RUN_TEST(macro_gen_single_5);
  RUN_TEST(macro_gen_single_6);
  RUN_TEST(macro_gen_single_7);
  RUN_TEST(macro_gen_pair_0_0);
  RUN_TEST(macro_gen_pair_0_1);
  RUN_TEST(macro_gen_pair_0_2);
  RUN_TEST(macro_gen_pair_0_3);
  RUN_TEST(macro_gen_pair_0_4);
  RUN_TEST(macro_gen_pair_0_5);
  RUN_TEST(macro_gen_pair_0_6);
  RUN_TEST(macro_gen_pair_0_7);
  RUN_TEST(macro_gen_pair_1_0);
  RUN_TEST(macro_gen_pair_1_1);
  RUN_TEST(macro_gen_pair_1_2);
  RUN_TEST(macro_gen_pair_1_3);
  RUN_TEST(macro_gen_pair_1_4);
  RUN_TEST(macro_gen_pair_1_5);
  RUN_TEST(macro_gen_pair_1_6);
  RUN_TEST(macro_gen_pair_1_7);
  RUN_TEST(macro_gen_pair_2_0);
  RUN_TEST(macro_gen_pair_2_1);
  RUN_TEST(macro_gen_pair_2_2);
  RUN_TEST(macro_gen_pair_2_3);
  RUN_TEST(macro_gen_pair_2_4);
  RUN_TEST(macro_gen_pair_2_5);
  RUN_TEST(macro_gen_pair_2_6);
  RUN_TEST(macro_gen_pair_2_7);
  RUN_TEST(macro_gen_pair_3_0);
  RUN_TEST(macro_gen_pair_3_1);
  RUN_TEST(macro_gen_pair_3_2);
  RUN_TEST(macro_gen_pair_3_3);
  RUN_TEST(macro_gen_pair_3_4);
  RUN_TEST(macro_gen_pair_3_5);
  RUN_TEST(macro_gen_pair_3_6);
  RUN_TEST(macro_gen_pair_3_7);
  RUN_TEST(macro_gen_pair_4_0);
  RUN_TEST(macro_gen_pair_4_1);
  RUN_TEST(macro_gen_pair_4_2);
  RUN_TEST(macro_gen_pair_4_3);
  RUN_TEST(macro_gen_pair_4_4);
  RUN_TEST(macro_gen_pair_4_5);
  RUN_TEST(macro_gen_pair_4_6);
  RUN_TEST(macro_gen_pair_4_7);
  RUN_TEST(macro_gen_pair_5_0);
  RUN_TEST(macro_gen_pair_5_1);
  RUN_TEST(macro_gen_pair_5_2);
  RUN_TEST(macro_gen_pair_5_3);
  RUN_TEST(macro_gen_pair_5_4);
  RUN_TEST(macro_gen_pair_5_5);
  RUN_TEST(macro_gen_pair_5_6);
  RUN_TEST(macro_gen_pair_5_7);
  RUN_TEST(macro_gen_pair_6_0);
  RUN_TEST(macro_gen_pair_6_1);
  RUN_TEST(macro_gen_pair_6_2);
  RUN_TEST(macro_gen_pair_6_3);
  RUN_TEST(macro_gen_pair_6_4);
  RUN_TEST(macro_gen_pair_6_5);
  RUN_TEST(macro_gen_pair_6_6);
  RUN_TEST(macro_gen_pair_6_7);
  RUN_TEST(macro_gen_pair_7_0);
  RUN_TEST(macro_gen_pair_7_1);
  RUN_TEST(macro_gen_pair_7_2);
  RUN_TEST(macro_gen_pair_7_3);
  RUN_TEST(macro_gen_pair_7_4);
  RUN_TEST(macro_gen_pair_7_5);
  RUN_TEST(macro_gen_pair_7_6);
  RUN_TEST(macro_gen_pair_7_7);
  RUN_TEST(macro_gen_repeated_1);
  RUN_TEST(macro_gen_repeated_2);
  RUN_TEST(macro_gen_repeated_3);
  RUN_TEST(macro_gen_repeated_4);
  RUN_TEST(macro_gen_repeated_5);
  RUN_TEST(macro_gen_repeated_6);
  RUN_TEST(macro_gen_repeated_7);
  RUN_TEST(macro_gen_repeated_8);
  RUN_TEST(macro_gen_repeated_9);
  RUN_TEST(macro_gen_repeated_10);
  RUN_TEST(macro_gen_char_name_0);
  RUN_TEST(macro_gen_user_name_0);
  RUN_TEST(macro_gen_char_name_1);
  RUN_TEST(macro_gen_user_name_1);
  RUN_TEST(macro_gen_char_name_2);
  RUN_TEST(macro_gen_user_name_2);
  RUN_TEST(macro_gen_char_name_3);
  RUN_TEST(macro_gen_user_name_3);
  RUN_TEST(macro_gen_char_name_4);
  RUN_TEST(macro_gen_user_name_4);
  RUN_TEST(macro_gen_char_name_5);
  RUN_TEST(macro_gen_user_name_5);
  RUN_TEST(macro_gen_char_name_6);
  RUN_TEST(macro_gen_user_name_6);
  RUN_TEST(macro_gen_no_macros_long);
  RUN_TEST(macro_gen_mixed_text_macros);
  RUN_TEST(macro_gen_null_input);
  RUN_TEST(macro_gen_null_char_name);
  RUN_TEST(macro_gen_null_user_name);
  RUN_TEST(macro_gen_empty_input);
}

#include "core/macros.h"
#include "test_framework.h"

TEST(macro_null_input) {
  char *result = macro_substitute(NULL, "Bot", "User");
  ASSERT_NULL(result);
  PASS();
}

TEST(macro_no_macros) {
  char *result = macro_substitute("Hello world!", "Bot", "User");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello world!", result);
  free(result);
  PASS();
}

TEST(macro_char_lowercase) {
  char *result = macro_substitute("Hello {{char}}!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Alice!", result);
  free(result);
  PASS();
}

TEST(macro_char_titlecase) {
  char *result = macro_substitute("Hello {{Char}}!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Alice!", result);
  free(result);
  PASS();
}

TEST(macro_char_uppercase) {
  char *result = macro_substitute("Hello {{CHAR}}!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Alice!", result);
  free(result);
  PASS();
}

TEST(macro_user_lowercase) {
  char *result = macro_substitute("Hello {{user}}!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Bob!", result);
  free(result);
  PASS();
}

TEST(macro_user_titlecase) {
  char *result = macro_substitute("Hello {{User}}!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Bob!", result);
  free(result);
  PASS();
}

TEST(macro_user_uppercase) {
  char *result = macro_substitute("Hello {{USER}}!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Bob!", result);
  free(result);
  PASS();
}

TEST(macro_legacy_user) {
  char *result = macro_substitute("Hello <USER>!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Bob!", result);
  free(result);
  PASS();
}

TEST(macro_legacy_bot) {
  char *result = macro_substitute("Hello <BOT>!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Alice!", result);
  free(result);
  PASS();
}

TEST(macro_legacy_char) {
  char *result = macro_substitute("Hello <CHAR>!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Alice!", result);
  free(result);
  PASS();
}

TEST(macro_charIfNotGroup) {
  char *result = macro_substitute("Hello {{charIfNotGroup}}!", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello Alice!", result);
  free(result);
  PASS();
}

TEST(macro_multiple_replacements) {
  char *result = macro_substitute(
      "{{char}} says hello to {{user}}. {{user}} replies to {{char}}.", "Alice",
      "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Alice says hello to Bob. Bob replies to Alice.", result);
  free(result);
  PASS();
}

TEST(macro_null_char_name) {
  char *result = macro_substitute("Hello {{char}}!", NULL, "User");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello !", result);
  free(result);
  PASS();
}

TEST(macro_null_user_name) {
  char *result = macro_substitute("Hello {{user}}!", "Bot", NULL);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello User!", result);
  free(result);
  PASS();
}

TEST(macro_empty_char_name) {
  char *result = macro_substitute("Hello {{char}}!", "", "User");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Hello !", result);
  free(result);
  PASS();
}

TEST(macro_adjacent_macros) {
  char *result = macro_substitute("{{char}}{{user}}", "A", "B");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("AB", result);
  free(result);
  PASS();
}

TEST(macro_partial_macro_not_replaced) {
  char *result = macro_substitute("{{cha}} and {char}}", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("{{cha}} and {char}}", result);
  free(result);
  PASS();
}

TEST(macro_long_names) {
  char *result = macro_substitute("{{char}} greets {{user}}",
                                  "VeryLongCharacterNameThatIsQuiteExtended",
                                  "AnotherVeryLongUserNameForTesting");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("VeryLongCharacterNameThatIsQuiteExtended greets "
                "AnotherVeryLongUserNameForTesting",
                result);
  free(result);
  PASS();
}

TEST(macro_empty_input) {
  char *result = macro_substitute("", "Alice", "Bob");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("", result);
  free(result);
  PASS();
}

TEST(macro_only_macro) {
  char *result = macro_substitute("{{char}}", "Test", "User");
  ASSERT_NOT_NULL(result);
  ASSERT_EQ_STR("Test", result);
  free(result);
  PASS();
}

void run_macros_tests(void) {
  TEST_SUITE("Macro Substitution");
  RUN_TEST(macro_null_input);
  RUN_TEST(macro_no_macros);
  RUN_TEST(macro_char_lowercase);
  RUN_TEST(macro_char_titlecase);
  RUN_TEST(macro_char_uppercase);
  RUN_TEST(macro_user_lowercase);
  RUN_TEST(macro_user_titlecase);
  RUN_TEST(macro_user_uppercase);
  RUN_TEST(macro_legacy_user);
  RUN_TEST(macro_legacy_bot);
  RUN_TEST(macro_legacy_char);
  RUN_TEST(macro_charIfNotGroup);
  RUN_TEST(macro_multiple_replacements);
  RUN_TEST(macro_null_char_name);
  RUN_TEST(macro_null_user_name);
  RUN_TEST(macro_empty_char_name);
  RUN_TEST(macro_adjacent_macros);
  RUN_TEST(macro_partial_macro_not_replaced);
  RUN_TEST(macro_long_names);
  RUN_TEST(macro_empty_input);
  RUN_TEST(macro_only_macro);
}

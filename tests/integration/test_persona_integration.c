#include "../test_framework.h"
#include "character/persona.h"
#include "test_helper.h"

TEST(persona_save_and_load) {
  setup_test_environment();

  Persona p1;
  memset(&p1, 0, sizeof(p1));
  persona_set_name(&p1, "Alice");
  persona_set_description(&p1, "A friendly AI researcher who loves cats.");

  bool saved = persona_save(&p1);
  ASSERT_TRUE(saved);

  Persona p2;
  memset(&p2, 0, sizeof(p2));
  bool loaded = persona_load(&p2);
  ASSERT_TRUE(loaded);

  ASSERT_EQ_STR("Alice", persona_get_name(&p2));
  ASSERT_EQ_STR("A friendly AI researcher who loves cats.",
                persona_get_description(&p2));

  teardown_test_environment();
  PASS();
}

TEST(persona_load_default_when_missing) {
  setup_test_environment();

  Persona p;
  memset(&p, 0, sizeof(p));
  persona_load(&p);

  ASSERT_EQ_STR("User", persona_get_name(&p));

  teardown_test_environment();
  PASS();
}

TEST(persona_set_name_truncates) {
  Persona p;
  memset(&p, 0, sizeof(p));

  char long_name[200];
  memset(long_name, 'A', 199);
  long_name[199] = '\0';

  persona_set_name(&p, long_name);

  ASSERT(strlen(persona_get_name(&p)) < PERSONA_NAME_MAX);
  PASS();
}

TEST(persona_set_description_long) {
  setup_test_environment();

  Persona p1;
  memset(&p1, 0, sizeof(p1));
  persona_set_name(&p1, "Test");

  char long_desc[10000];
  for (int i = 0; i < 9999; i++) {
    long_desc[i] = 'a' + (i % 26);
  }
  long_desc[9999] = '\0';

  persona_set_description(&p1, long_desc);
  persona_save(&p1);

  Persona p2;
  memset(&p2, 0, sizeof(p2));
  persona_load(&p2);

  ASSERT(strlen(persona_get_description(&p2)) >= 9999);

  teardown_test_environment();
  PASS();
}

TEST(persona_special_characters) {
  setup_test_environment();

  Persona p1;
  memset(&p1, 0, sizeof(p1));
  persona_set_name(&p1, "\"Alice\"");
  persona_set_description(&p1, "Contains \"quotes\" and\nnewlines\tand\ttabs");

  persona_save(&p1);

  Persona p2;
  memset(&p2, 0, sizeof(p2));
  persona_load(&p2);

  ASSERT(strstr(persona_get_description(&p2), "newlines") != NULL);

  teardown_test_environment();
  PASS();
}

TEST(persona_null_safety) {
  ASSERT_EQ_STR("User", persona_get_name(NULL));
  ASSERT_EQ_STR("", persona_get_description(NULL));
  persona_set_name(NULL, "Test");
  persona_set_description(NULL, "Test");
  PASS();
}

TEST(persona_empty_values) {
  setup_test_environment();

  Persona p1;
  memset(&p1, 0, sizeof(p1));
  persona_set_name(&p1, "");
  persona_set_description(&p1, "");
  persona_save(&p1);

  Persona p2;
  memset(&p2, 0, sizeof(p2));
  persona_load(&p2);

  ASSERT_EQ_STR("User", persona_get_name(&p2));

  teardown_test_environment();
  PASS();
}

void run_persona_integration_tests(void) {
  TEST_SUITE("Persona Integration");
  RUN_TEST(persona_save_and_load);
  RUN_TEST(persona_load_default_when_missing);
  RUN_TEST(persona_set_name_truncates);
  RUN_TEST(persona_set_description_long);
  RUN_TEST(persona_special_characters);
  RUN_TEST(persona_null_safety);
  RUN_TEST(persona_empty_values);
}

#include "../test_framework.h"

int g_tests_run = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;
const char *g_current_suite = NULL;
bool g_current_test_passed = true;

extern void run_chat_integration_tests(void);
extern void run_config_integration_tests(void);
extern void run_persona_integration_tests(void);
extern void run_tokenizer_integration_tests(void);

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf(
      "\n\033[1;"
      "35m╔══════════════════════════════════════════════════════╗\033[0m\n");
  printf("\033[1;35m║       SillyTUI Integration Test Suite                "
         "║\033[0m\n");
  printf(
      "\033[1;35m╚══════════════════════════════════════════════════════╝\033["
      "0m\n");

  run_chat_integration_tests();
  run_config_integration_tests();
  run_persona_integration_tests();
  run_tokenizer_integration_tests();

  print_test_summary();

  return get_test_result();
}

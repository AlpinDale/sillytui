#include "test_framework.h"
#include <time.h>

int g_tests_run = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;
const char *g_current_suite = NULL;
bool g_current_test_passed = true;

extern void run_history_tests(void);
extern void run_macros_tests(void);
extern void run_config_tests(void);
extern void run_sampler_tests(void);
extern void run_simd_tests(void);
extern void run_tokenizer_tests(void);
extern void run_modal_tests(void);
extern void run_struct_size_tests(void);
extern void run_robustness_tests(void);
extern void run_lorebook_tests(void);
extern void run_tokenizer_selector_tests(void);

extern void run_boundary_tests(void);
extern void run_stress_tests(void);

extern void run_unicode_generated_tests(void);
extern void run_utf8_generated_tests(void);
extern void run_pretokenize_generated_tests(void);
extern void run_history_generated_tests(void);
extern void run_config_generated_tests(void);
extern void run_sampler_generated_tests(void);
extern void run_simd_generated_tests(void);
extern void run_macro_generated_tests(void);
extern void run_edge_case_generated_tests(void);

extern void run_property_tests(void);

extern void run_chat_integration_tests(void);
extern void run_config_integration_tests(void);
extern void run_persona_integration_tests(void);
extern void run_tokenizer_integration_tests(void);
extern void run_safetensors_tests(void);

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  clock_t start = clock();

  printf(
      "\n\033[1;"
      "35m╔══════════════════════════════════════════════════════╗\033[0m\n");
  printf("\033[1;35m║    SillyTUI Comprehensive Test Suite  "
         "║\033[0m\n");
  printf(
      "\033[1;35m╚══════════════════════════════════════════════════════╝\033["
      "0m\n");

  printf("\n\033[1;33m>>> Struct Size Safety <<<\033[0m\n");
  run_struct_size_tests();

  printf("\n\033[1;33m>>> Robustness Tests <<<\033[0m\n");
  run_robustness_tests();

  printf("\n\033[1;33m>>> Unit Tests <<<\033[0m\n");
  run_history_tests();
  run_macros_tests();
  run_config_tests();
  run_sampler_tests();
  run_simd_tests();
  run_tokenizer_tests();
  run_modal_tests();
  run_lorebook_tests();
  run_tokenizer_selector_tests();

  printf("\n\033[1;33m>>> Generated Tests <<<\033[0m\n");
  run_unicode_generated_tests();
  run_utf8_generated_tests();
  run_pretokenize_generated_tests();
  run_history_generated_tests();
  run_config_generated_tests();
  run_sampler_generated_tests();
  run_simd_generated_tests();
  run_macro_generated_tests();
  run_edge_case_generated_tests();

  printf("\n\033[1;33m>>> Property-Based Tests <<<\033[0m\n");
  run_property_tests();

  printf("\n\033[1;33m>>> Boundary Tests <<<\033[0m\n");
  run_boundary_tests();

  printf("\n\033[1;33m>>> Stress Tests <<<\033[0m\n");
  run_stress_tests();

  printf("\n\033[1;33m>>> Integration Tests <<<\033[0m\n");
  run_chat_integration_tests();
  run_config_integration_tests();
  run_persona_integration_tests();
  run_tokenizer_integration_tests();
  run_safetensors_tests();

  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

  print_test_summary();
  printf("\n  Time: %.3f seconds\n", elapsed);

  return get_test_result();
}

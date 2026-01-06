#include "test_framework.h"

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
extern void run_attachment_tests(void);
extern void run_safetensors_tests(void);
extern void run_gemm_tests(void);
extern void run_gemm_pytorch_accuracy_tests(void);
extern void run_layernorm_tests(void);

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf(
      "\n\033[1;"
      "35m╔══════════════════════════════════════════════════════╗\033[0m\n");
  printf("\033[1;35m║           SillyTUI Test Suite                        "
         "║\033[0m\n");
  printf(
      "\033[1;35m╚══════════════════════════════════════════════════════╝\033["
      "0m\n");

  run_history_tests();
  run_macros_tests();
  run_config_tests();
  run_sampler_tests();
  run_simd_tests();
  run_tokenizer_tests();
  run_modal_tests();
  run_attachment_tests();
  run_safetensors_tests();
  run_gemm_tests();
  run_gemm_pytorch_accuracy_tests();
  run_layernorm_tests();

  print_test_summary();

  return get_test_result();
}

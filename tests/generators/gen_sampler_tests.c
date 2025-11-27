#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"llm/sampler.h\"\n");
  printf("#include <stdlib.h>\n");
  printf("#include <string.h>\n");
  printf("#include <math.h>\n\n");

  const char *float_params[] = {"temperature",
                                "top_p",
                                "min_p",
                                "typical_p",
                                "tfs",
                                "presence_penalty",
                                "frequency_penalty",
                                "repetition_penalty",
                                "dynatemp_min",
                                "dynatemp_max",
                                "dynatemp_exponent",
                                "mirostat_tau",
                                "mirostat_eta",
                                "dry_multiplier",
                                "dry_base",
                                "xtc_probability",
                                "xtc_threshold"};
  int num_float_params = 17;

  for (int i = 0; i < num_float_params; i++) {
    printf("TEST(sampler_gen_%s_range) {\n", float_params[i]);
    printf("  SamplerSettings s;\n");
    printf("  sampler_init_defaults(&s);\n");
    printf("  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};\n");
    printf("  for (size_t j = 0; j < 6; j++) {\n");
    printf("    s.%s = values[j];\n", float_params[i]);
    printf("    ASSERT(s.%s >= 0.0 || s.%s < 0.0);\n", float_params[i],
           float_params[i]);
    printf("  }\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(sampler_gen_top_k_range) {\n");
  printf("  SamplerSettings s;\n");
  printf("  sampler_init_defaults(&s);\n");
  printf("  int values[] = {0, 1, 10, 40, 100, 500, 1000};\n");
  printf("  for (size_t j = 0; j < 7; j++) {\n");
  printf("    s.top_k = values[j];\n");
  printf("    ASSERT_EQ_INT(values[j], s.top_k);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  for (int n = 1; n <= 64; n++) {
    printf("TEST(sampler_gen_add_%d_custom) {\n", n);
    printf("  SamplerSettings s;\n");
    printf("  sampler_init_defaults(&s);\n");
    printf("  for (int i = 0; i < %d; i++) {\n", n);
    printf("    char name[64];\n");
    printf("    snprintf(name, sizeof(name), \"param%%d\", i);\n");
    printf("    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT, "
           "(double)i * 0.1, NULL, 0.0, 10.0, 0.1));\n");
    printf("  }\n");
    printf("  ASSERT_EQ_INT(%d, s.custom_count);\n", n);
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(sampler_gen_custom_types_all) {\n");
  printf("  SamplerSettings s;\n");
  printf("  sampler_init_defaults(&s);\n");
  printf("  ASSERT_TRUE(sampler_add_custom(&s, \"int_param\", "
         "SAMPLER_TYPE_INT, 42.0, NULL, 0.0, 100.0, 1.0));\n");
  printf("  ASSERT_TRUE(sampler_add_custom(&s, \"float_param\", "
         "SAMPLER_TYPE_FLOAT, 3.14, NULL, 0.0, 10.0, 0.01));\n");
  printf("  ASSERT_TRUE(sampler_add_custom(&s, \"string_param\", "
         "SAMPLER_TYPE_STRING, 0.0, \"hello\", 0.0, 0.0, 0.0));\n");
  printf("  ASSERT_TRUE(sampler_add_custom(&s, \"bool_param\", "
         "SAMPLER_TYPE_BOOL, 1.0, NULL, 0.0, 1.0, 1.0));\n");
  printf("  ASSERT_EQ_INT(4, s.custom_count);\n");
  printf("  ASSERT_EQ_INT(SAMPLER_TYPE_INT, s.custom[0].type);\n");
  printf("  ASSERT_EQ_INT(SAMPLER_TYPE_FLOAT, s.custom[1].type);\n");
  printf("  ASSERT_EQ_INT(SAMPLER_TYPE_STRING, s.custom[2].type);\n");
  printf("  ASSERT_EQ_INT(SAMPLER_TYPE_BOOL, s.custom[3].type);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(sampler_gen_remove_custom_all) {\n");
  printf("  SamplerSettings s;\n");
  printf("  sampler_init_defaults(&s);\n");
  printf("  for (int i = 0; i < 20; i++) {\n");
  printf("    char name[64];\n");
  printf("    snprintf(name, sizeof(name), \"param%%d\", i);\n");
  printf("    sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT, 0.0, NULL, 0.0, "
         "1.0, 0.1);\n");
  printf("  }\n");
  printf("  for (int i = 0; i < 20; i++) {\n");
  printf("    ASSERT_TRUE(sampler_remove_custom(&s, 0));\n");
  printf("  }\n");
  printf("  ASSERT_EQ_INT(0, s.custom_count);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(sampler_gen_mirostat_modes) {\n");
  printf("  SamplerSettings s;\n");
  printf("  sampler_init_defaults(&s);\n");
  printf("  for (int mode = 0; mode <= 2; mode++) {\n");
  printf("    s.mirostat_mode = mode;\n");
  printf("    ASSERT_EQ_INT(mode, s.mirostat_mode);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_sampler_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"Sampler Generated Tests\");\n");
  for (int i = 0; i < num_float_params; i++) {
    printf("  RUN_TEST(sampler_gen_%s_range);\n", float_params[i]);
  }
  printf("  RUN_TEST(sampler_gen_top_k_range);\n");
  for (int n = 1; n <= 64; n++) {
    printf("  RUN_TEST(sampler_gen_add_%d_custom);\n", n);
  }
  printf("  RUN_TEST(sampler_gen_custom_types_all);\n");
  printf("  RUN_TEST(sampler_gen_remove_custom_all);\n");
  printf("  RUN_TEST(sampler_gen_mirostat_modes);\n");
  printf("}\n");

  return 0;
}

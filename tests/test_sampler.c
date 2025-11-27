#include "llm/sampler.h"
#include "test_framework.h"

TEST(sampler_init_defaults_temperature) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(1.0, s.temperature, 0.001);
  PASS();
}

TEST(sampler_init_defaults_top_p) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(1.0, s.top_p, 0.001);
  PASS();
}

TEST(sampler_init_defaults_top_k) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_EQ_INT(-1, s.top_k);
  PASS();
}

TEST(sampler_init_defaults_min_p) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(0.0, s.min_p, 0.001);
  PASS();
}

TEST(sampler_init_defaults_penalties) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(0.0, s.frequency_penalty, 0.001);
  ASSERT_NEAR(0.0, s.presence_penalty, 0.001);
  ASSERT_NEAR(1.0, s.repetition_penalty, 0.001);
  PASS();
}

TEST(sampler_init_defaults_typical_tfs) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(1.0, s.typical_p, 0.001);
  ASSERT_NEAR(1.0, s.tfs, 0.001);
  PASS();
}

TEST(sampler_init_defaults_tokens) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_EQ_INT(0, s.min_tokens);
  ASSERT_EQ_INT(512, s.max_tokens);
  PASS();
}

TEST(sampler_init_defaults_dynatemp) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(0.0, s.dynatemp_min, 0.001);
  ASSERT_NEAR(0.0, s.dynatemp_max, 0.001);
  ASSERT_NEAR(1.0, s.dynatemp_exponent, 0.001);
  PASS();
}

TEST(sampler_init_defaults_mirostat) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_EQ_INT(0, s.mirostat_mode);
  ASSERT_NEAR(0.0, s.mirostat_tau, 0.001);
  ASSERT_NEAR(0.0, s.mirostat_eta, 0.001);
  PASS();
}

TEST(sampler_init_defaults_dry) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(0.0, s.dry_multiplier, 0.001);
  ASSERT_NEAR(1.75, s.dry_base, 0.001);
  ASSERT_EQ_INT(2, s.dry_allowed_length);
  ASSERT_EQ_INT(0, s.dry_range);
  PASS();
}

TEST(sampler_init_defaults_xtc) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_NEAR(0.1, s.xtc_threshold, 0.001);
  ASSERT_NEAR(0.0, s.xtc_probability, 0.001);
  PASS();
}

TEST(sampler_init_defaults_custom_count) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_EQ_INT(0, s.custom_count);
  PASS();
}

TEST(sampler_add_custom_basic) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "test_param", SAMPLER_TYPE_FLOAT, 0.5,
                                 NULL, 0.0, 1.0, 0.1));
  ASSERT_EQ_INT(1, s.custom_count);
  ASSERT_EQ_STR("test_param", s.custom[0].name);
  ASSERT_EQ(SAMPLER_TYPE_FLOAT, s.custom[0].type);
  ASSERT_NEAR(0.5, s.custom[0].value, 0.001);
  PASS();
}

TEST(sampler_add_custom_int) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "int_param", SAMPLER_TYPE_INT, 42.0, NULL,
                                 0.0, 100.0, 1.0));
  ASSERT_EQ(SAMPLER_TYPE_INT, s.custom[0].type);
  ASSERT_NEAR(42.0, s.custom[0].value, 0.001);
  ASSERT_NEAR(1.0, s.custom[0].step, 0.001);
  PASS();
}

TEST(sampler_add_custom_string) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "str_param", SAMPLER_TYPE_STRING, 0.0,
                                 "test_value", 0.0, 0.0, 0.0));
  ASSERT_EQ(SAMPLER_TYPE_STRING, s.custom[0].type);
  ASSERT_EQ_STR("test_value", s.custom[0].str_value);
  PASS();
}

TEST(sampler_add_custom_bool) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "bool_param", SAMPLER_TYPE_BOOL, 1.0, NULL,
                                 0.0, 0.0, 0.0));
  ASSERT_EQ(SAMPLER_TYPE_BOOL, s.custom[0].type);
  ASSERT_NEAR(1.0, s.custom[0].value, 0.001);
  PASS();
}

TEST(sampler_add_custom_multiple) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "param1", SAMPLER_TYPE_FLOAT, 0.1, NULL, 0,
                                 1, 0.1));
  ASSERT_TRUE(
      sampler_add_custom(&s, "param2", SAMPLER_TYPE_INT, 50, NULL, 0, 100, 1));
  ASSERT_TRUE(sampler_add_custom(&s, "param3", SAMPLER_TYPE_STRING, 0, "hello",
                                 0, 0, 0));
  ASSERT_EQ_INT(3, s.custom_count);
  ASSERT_EQ_STR("param1", s.custom[0].name);
  ASSERT_EQ_STR("param2", s.custom[1].name);
  ASSERT_EQ_STR("param3", s.custom[2].name);
  PASS();
}

TEST(sampler_add_custom_max_capacity) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < MAX_CUSTOM_SAMPLERS; i++) {
    char name[32];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(
        sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT, 0.0, NULL, 0, 1, 0.1));
  }
  ASSERT_EQ_INT(MAX_CUSTOM_SAMPLERS, s.custom_count);
  ASSERT_FALSE(sampler_add_custom(&s, "overflow", SAMPLER_TYPE_FLOAT, 0.0, NULL,
                                  0, 1, 0.1));
  ASSERT_EQ_INT(MAX_CUSTOM_SAMPLERS, s.custom_count);
  PASS();
}

TEST(sampler_remove_custom_basic) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  sampler_add_custom(&s, "param1", SAMPLER_TYPE_FLOAT, 0.1, NULL, 0, 1, 0.1);
  sampler_add_custom(&s, "param2", SAMPLER_TYPE_FLOAT, 0.2, NULL, 0, 1, 0.1);
  sampler_add_custom(&s, "param3", SAMPLER_TYPE_FLOAT, 0.3, NULL, 0, 1, 0.1);

  ASSERT_TRUE(sampler_remove_custom(&s, 1));
  ASSERT_EQ_INT(2, s.custom_count);
  ASSERT_EQ_STR("param1", s.custom[0].name);
  ASSERT_EQ_STR("param3", s.custom[1].name);
  PASS();
}

TEST(sampler_remove_custom_first) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  sampler_add_custom(&s, "first", SAMPLER_TYPE_FLOAT, 0.1, NULL, 0, 1, 0.1);
  sampler_add_custom(&s, "second", SAMPLER_TYPE_FLOAT, 0.2, NULL, 0, 1, 0.1);

  ASSERT_TRUE(sampler_remove_custom(&s, 0));
  ASSERT_EQ_INT(1, s.custom_count);
  ASSERT_EQ_STR("second", s.custom[0].name);
  PASS();
}

TEST(sampler_remove_custom_last) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  sampler_add_custom(&s, "first", SAMPLER_TYPE_FLOAT, 0.1, NULL, 0, 1, 0.1);
  sampler_add_custom(&s, "last", SAMPLER_TYPE_FLOAT, 0.2, NULL, 0, 1, 0.1);

  ASSERT_TRUE(sampler_remove_custom(&s, 1));
  ASSERT_EQ_INT(1, s.custom_count);
  ASSERT_EQ_STR("first", s.custom[0].name);
  PASS();
}

TEST(sampler_remove_custom_invalid) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  sampler_add_custom(&s, "param", SAMPLER_TYPE_FLOAT, 0.1, NULL, 0, 1, 0.1);

  ASSERT_FALSE(sampler_remove_custom(&s, 5));
  ASSERT_FALSE(sampler_remove_custom(&s, -1));
  ASSERT_EQ_INT(1, s.custom_count);
  PASS();
}

TEST(sampler_add_custom_default_step_float) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "param", SAMPLER_TYPE_FLOAT, 0.5, NULL,
                                 0.0, 1.0, 0.0));
  ASSERT_NEAR(0.1, s.custom[0].step, 0.001);
  PASS();
}

TEST(sampler_add_custom_default_step_int) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "param", SAMPLER_TYPE_INT, 50.0, NULL, 0.0,
                                 100.0, 0.0));
  ASSERT_NEAR(1.0, s.custom[0].step, 0.001);
  PASS();
}

void run_sampler_tests(void) {
  TEST_SUITE("Sampler Settings");
  RUN_TEST(sampler_init_defaults_temperature);
  RUN_TEST(sampler_init_defaults_top_p);
  RUN_TEST(sampler_init_defaults_top_k);
  RUN_TEST(sampler_init_defaults_min_p);
  RUN_TEST(sampler_init_defaults_penalties);
  RUN_TEST(sampler_init_defaults_typical_tfs);
  RUN_TEST(sampler_init_defaults_tokens);
  RUN_TEST(sampler_init_defaults_dynatemp);
  RUN_TEST(sampler_init_defaults_mirostat);
  RUN_TEST(sampler_init_defaults_dry);
  RUN_TEST(sampler_init_defaults_xtc);
  RUN_TEST(sampler_init_defaults_custom_count);
  RUN_TEST(sampler_add_custom_basic);
  RUN_TEST(sampler_add_custom_int);
  RUN_TEST(sampler_add_custom_string);
  RUN_TEST(sampler_add_custom_bool);
  RUN_TEST(sampler_add_custom_multiple);
  RUN_TEST(sampler_add_custom_max_capacity);
  RUN_TEST(sampler_remove_custom_basic);
  RUN_TEST(sampler_remove_custom_first);
  RUN_TEST(sampler_remove_custom_last);
  RUN_TEST(sampler_remove_custom_invalid);
  RUN_TEST(sampler_add_custom_default_step_float);
  RUN_TEST(sampler_add_custom_default_step_int);
}

#include "llm/sampler.h"
#include "test_framework.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

TEST(sampler_gen_temperature_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.temperature = values[j];
    ASSERT(s.temperature >= 0.0 || s.temperature < 0.0);
  }
  PASS();
}

TEST(sampler_gen_top_p_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.top_p = values[j];
    ASSERT(s.top_p >= 0.0 || s.top_p < 0.0);
  }
  PASS();
}

TEST(sampler_gen_min_p_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.min_p = values[j];
    ASSERT(s.min_p >= 0.0 || s.min_p < 0.0);
  }
  PASS();
}

TEST(sampler_gen_typical_p_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.typical_p = values[j];
    ASSERT(s.typical_p >= 0.0 || s.typical_p < 0.0);
  }
  PASS();
}

TEST(sampler_gen_tfs_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.tfs = values[j];
    ASSERT(s.tfs >= 0.0 || s.tfs < 0.0);
  }
  PASS();
}

TEST(sampler_gen_presence_penalty_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.presence_penalty = values[j];
    ASSERT(s.presence_penalty >= 0.0 || s.presence_penalty < 0.0);
  }
  PASS();
}

TEST(sampler_gen_frequency_penalty_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.frequency_penalty = values[j];
    ASSERT(s.frequency_penalty >= 0.0 || s.frequency_penalty < 0.0);
  }
  PASS();
}

TEST(sampler_gen_repetition_penalty_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.repetition_penalty = values[j];
    ASSERT(s.repetition_penalty >= 0.0 || s.repetition_penalty < 0.0);
  }
  PASS();
}

TEST(sampler_gen_dynatemp_min_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.dynatemp_min = values[j];
    ASSERT(s.dynatemp_min >= 0.0 || s.dynatemp_min < 0.0);
  }
  PASS();
}

TEST(sampler_gen_dynatemp_max_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.dynatemp_max = values[j];
    ASSERT(s.dynatemp_max >= 0.0 || s.dynatemp_max < 0.0);
  }
  PASS();
}

TEST(sampler_gen_dynatemp_exponent_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.dynatemp_exponent = values[j];
    ASSERT(s.dynatemp_exponent >= 0.0 || s.dynatemp_exponent < 0.0);
  }
  PASS();
}

TEST(sampler_gen_mirostat_tau_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.mirostat_tau = values[j];
    ASSERT(s.mirostat_tau >= 0.0 || s.mirostat_tau < 0.0);
  }
  PASS();
}

TEST(sampler_gen_mirostat_eta_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.mirostat_eta = values[j];
    ASSERT(s.mirostat_eta >= 0.0 || s.mirostat_eta < 0.0);
  }
  PASS();
}

TEST(sampler_gen_dry_multiplier_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.dry_multiplier = values[j];
    ASSERT(s.dry_multiplier >= 0.0 || s.dry_multiplier < 0.0);
  }
  PASS();
}

TEST(sampler_gen_dry_base_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.dry_base = values[j];
    ASSERT(s.dry_base >= 0.0 || s.dry_base < 0.0);
  }
  PASS();
}

TEST(sampler_gen_xtc_probability_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.xtc_probability = values[j];
    ASSERT(s.xtc_probability >= 0.0 || s.xtc_probability < 0.0);
  }
  PASS();
}

TEST(sampler_gen_xtc_threshold_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  double values[] = {0.0, 0.1, 0.5, 1.0, 2.0, 10.0};
  for (size_t j = 0; j < 6; j++) {
    s.xtc_threshold = values[j];
    ASSERT(s.xtc_threshold >= 0.0 || s.xtc_threshold < 0.0);
  }
  PASS();
}

TEST(sampler_gen_top_k_range) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  int values[] = {0, 1, 10, 40, 100, 500, 1000};
  for (size_t j = 0; j < 7; j++) {
    s.top_k = values[j];
    ASSERT_EQ_INT(values[j], s.top_k);
  }
  PASS();
}

TEST(sampler_gen_add_1_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 1; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(1, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_2_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 2; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(2, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_3_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 3; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(3, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_4_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 4; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(4, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_5_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 5; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(5, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_6_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 6; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(6, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_7_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 7; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(7, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_8_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 8; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(8, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_9_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 9; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(9, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_10_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 10; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(10, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_11_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 11; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(11, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_12_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 12; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(12, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_13_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 13; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(13, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_14_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 14; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(14, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_15_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 15; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(15, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_16_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 16; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(16, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_17_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 17; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(17, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_18_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 18; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(18, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_19_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 19; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(19, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_20_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 20; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(20, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_21_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 21; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(21, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_22_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 22; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(22, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_23_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 23; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(23, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_24_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 24; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(24, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_25_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 25; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(25, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_26_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 26; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(26, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_27_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 27; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(27, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_28_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 28; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(28, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_29_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 29; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(29, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_30_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 30; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(30, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_31_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 31; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(31, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_32_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 32; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(32, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_33_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 33; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(33, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_34_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 34; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(34, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_35_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 35; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(35, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_36_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 36; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(36, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_37_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 37; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(37, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_38_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 38; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(38, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_39_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 39; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(39, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_40_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 40; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(40, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_41_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 41; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(41, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_42_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 42; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(42, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_43_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 43; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(43, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_44_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 44; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(44, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_45_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 45; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(45, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_46_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 46; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(46, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_47_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 47; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(47, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_48_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 48; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(48, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_49_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 49; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(49, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_50_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 50; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(50, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_51_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 51; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(51, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_52_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 52; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(52, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_53_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 53; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(53, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_54_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 54; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(54, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_55_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 55; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(55, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_56_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 56; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(56, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_57_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 57; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(57, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_58_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 58; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(58, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_59_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 59; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(59, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_60_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 60; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(60, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_61_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 61; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(61, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_62_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 62; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(62, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_63_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 63; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(63, s.custom_count);
  PASS();
}

TEST(sampler_gen_add_64_custom) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 64; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    ASSERT_TRUE(sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT,
                                   (double)i * 0.1, NULL, 0.0, 10.0, 0.1));
  }
  ASSERT_EQ_INT(64, s.custom_count);
  PASS();
}

TEST(sampler_gen_custom_types_all) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  ASSERT_TRUE(sampler_add_custom(&s, "int_param", SAMPLER_TYPE_INT, 42.0, NULL,
                                 0.0, 100.0, 1.0));
  ASSERT_TRUE(sampler_add_custom(&s, "float_param", SAMPLER_TYPE_FLOAT, 3.14,
                                 NULL, 0.0, 10.0, 0.01));
  ASSERT_TRUE(sampler_add_custom(&s, "string_param", SAMPLER_TYPE_STRING, 0.0,
                                 "hello", 0.0, 0.0, 0.0));
  ASSERT_TRUE(sampler_add_custom(&s, "bool_param", SAMPLER_TYPE_BOOL, 1.0, NULL,
                                 0.0, 1.0, 1.0));
  ASSERT_EQ_INT(4, s.custom_count);
  ASSERT_EQ_INT(SAMPLER_TYPE_INT, s.custom[0].type);
  ASSERT_EQ_INT(SAMPLER_TYPE_FLOAT, s.custom[1].type);
  ASSERT_EQ_INT(SAMPLER_TYPE_STRING, s.custom[2].type);
  ASSERT_EQ_INT(SAMPLER_TYPE_BOOL, s.custom[3].type);
  PASS();
}

TEST(sampler_gen_remove_custom_all) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int i = 0; i < 20; i++) {
    char name[64];
    snprintf(name, sizeof(name), "param%d", i);
    sampler_add_custom(&s, name, SAMPLER_TYPE_FLOAT, 0.0, NULL, 0.0, 1.0, 0.1);
  }
  for (int i = 0; i < 20; i++) {
    ASSERT_TRUE(sampler_remove_custom(&s, 0));
  }
  ASSERT_EQ_INT(0, s.custom_count);
  PASS();
}

TEST(sampler_gen_mirostat_modes) {
  SamplerSettings s;
  sampler_init_defaults(&s);
  for (int mode = 0; mode <= 2; mode++) {
    s.mirostat_mode = mode;
    ASSERT_EQ_INT(mode, s.mirostat_mode);
  }
  PASS();
}

void run_sampler_generated_tests(void) {
  TEST_SUITE("Sampler Generated Tests");
  RUN_TEST(sampler_gen_temperature_range);
  RUN_TEST(sampler_gen_top_p_range);
  RUN_TEST(sampler_gen_min_p_range);
  RUN_TEST(sampler_gen_typical_p_range);
  RUN_TEST(sampler_gen_tfs_range);
  RUN_TEST(sampler_gen_presence_penalty_range);
  RUN_TEST(sampler_gen_frequency_penalty_range);
  RUN_TEST(sampler_gen_repetition_penalty_range);
  RUN_TEST(sampler_gen_dynatemp_min_range);
  RUN_TEST(sampler_gen_dynatemp_max_range);
  RUN_TEST(sampler_gen_dynatemp_exponent_range);
  RUN_TEST(sampler_gen_mirostat_tau_range);
  RUN_TEST(sampler_gen_mirostat_eta_range);
  RUN_TEST(sampler_gen_dry_multiplier_range);
  RUN_TEST(sampler_gen_dry_base_range);
  RUN_TEST(sampler_gen_xtc_probability_range);
  RUN_TEST(sampler_gen_xtc_threshold_range);
  RUN_TEST(sampler_gen_top_k_range);
  RUN_TEST(sampler_gen_add_1_custom);
  RUN_TEST(sampler_gen_add_2_custom);
  RUN_TEST(sampler_gen_add_3_custom);
  RUN_TEST(sampler_gen_add_4_custom);
  RUN_TEST(sampler_gen_add_5_custom);
  RUN_TEST(sampler_gen_add_6_custom);
  RUN_TEST(sampler_gen_add_7_custom);
  RUN_TEST(sampler_gen_add_8_custom);
  RUN_TEST(sampler_gen_add_9_custom);
  RUN_TEST(sampler_gen_add_10_custom);
  RUN_TEST(sampler_gen_add_11_custom);
  RUN_TEST(sampler_gen_add_12_custom);
  RUN_TEST(sampler_gen_add_13_custom);
  RUN_TEST(sampler_gen_add_14_custom);
  RUN_TEST(sampler_gen_add_15_custom);
  RUN_TEST(sampler_gen_add_16_custom);
  RUN_TEST(sampler_gen_add_17_custom);
  RUN_TEST(sampler_gen_add_18_custom);
  RUN_TEST(sampler_gen_add_19_custom);
  RUN_TEST(sampler_gen_add_20_custom);
  RUN_TEST(sampler_gen_add_21_custom);
  RUN_TEST(sampler_gen_add_22_custom);
  RUN_TEST(sampler_gen_add_23_custom);
  RUN_TEST(sampler_gen_add_24_custom);
  RUN_TEST(sampler_gen_add_25_custom);
  RUN_TEST(sampler_gen_add_26_custom);
  RUN_TEST(sampler_gen_add_27_custom);
  RUN_TEST(sampler_gen_add_28_custom);
  RUN_TEST(sampler_gen_add_29_custom);
  RUN_TEST(sampler_gen_add_30_custom);
  RUN_TEST(sampler_gen_add_31_custom);
  RUN_TEST(sampler_gen_add_32_custom);
  RUN_TEST(sampler_gen_add_33_custom);
  RUN_TEST(sampler_gen_add_34_custom);
  RUN_TEST(sampler_gen_add_35_custom);
  RUN_TEST(sampler_gen_add_36_custom);
  RUN_TEST(sampler_gen_add_37_custom);
  RUN_TEST(sampler_gen_add_38_custom);
  RUN_TEST(sampler_gen_add_39_custom);
  RUN_TEST(sampler_gen_add_40_custom);
  RUN_TEST(sampler_gen_add_41_custom);
  RUN_TEST(sampler_gen_add_42_custom);
  RUN_TEST(sampler_gen_add_43_custom);
  RUN_TEST(sampler_gen_add_44_custom);
  RUN_TEST(sampler_gen_add_45_custom);
  RUN_TEST(sampler_gen_add_46_custom);
  RUN_TEST(sampler_gen_add_47_custom);
  RUN_TEST(sampler_gen_add_48_custom);
  RUN_TEST(sampler_gen_add_49_custom);
  RUN_TEST(sampler_gen_add_50_custom);
  RUN_TEST(sampler_gen_add_51_custom);
  RUN_TEST(sampler_gen_add_52_custom);
  RUN_TEST(sampler_gen_add_53_custom);
  RUN_TEST(sampler_gen_add_54_custom);
  RUN_TEST(sampler_gen_add_55_custom);
  RUN_TEST(sampler_gen_add_56_custom);
  RUN_TEST(sampler_gen_add_57_custom);
  RUN_TEST(sampler_gen_add_58_custom);
  RUN_TEST(sampler_gen_add_59_custom);
  RUN_TEST(sampler_gen_add_60_custom);
  RUN_TEST(sampler_gen_add_61_custom);
  RUN_TEST(sampler_gen_add_62_custom);
  RUN_TEST(sampler_gen_add_63_custom);
  RUN_TEST(sampler_gen_add_64_custom);
  RUN_TEST(sampler_gen_custom_types_all);
  RUN_TEST(sampler_gen_remove_custom_all);
  RUN_TEST(sampler_gen_mirostat_modes);
}

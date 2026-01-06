/*
 * Token Sampling Unit Tests
 */

#include "test_framework.h"

extern "C" {
#include "inference/kernels/sampling/sampling.h"
}

#include <cmath>
#include <cstdlib>
#include <cstring>

TEST(sampling_greedy_argmax) {
  const int vocab_size = 10;
  float logits[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                    6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled =
      sampling_sample_f32(logits, vocab_size, 0.0f, -1, 1.0f, 0.0f, &rng);
  ASSERT_EQ_INT(9, sampled);
}

TEST(sampling_temperature_scaling) {
  const int vocab_size = 5;
  float logits[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled =
      sampling_sample_f32(logits, vocab_size, 1.0f, -1, 1.0f, 0.0f, &rng);
  ASSERT_TRUE(sampled >= 0 && sampled < vocab_size);
}

TEST(sampling_top_k) {
  const int vocab_size = 10;
  float logits[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                    6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled =
      sampling_sample_f32(logits, vocab_size, 1.0f, 3, 1.0f, 0.0f, &rng);
  ASSERT_TRUE(sampled >= 7 && sampled < vocab_size);
}

TEST(sampling_top_p) {
  const int vocab_size = 10;
  float logits[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                    6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled =
      sampling_sample_f32(logits, vocab_size, 1.0f, -1, 0.5f, 0.0f, &rng);
  ASSERT_TRUE(sampled >= 0 && sampled < vocab_size);
}

TEST(sampling_min_p) {
  const int vocab_size = 10;
  float logits[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                    6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled =
      sampling_sample_f32(logits, vocab_size, 1.0f, -1, 1.0f, 0.1f, &rng);
  ASSERT_TRUE(sampled >= 0 && sampled < vocab_size);
}

TEST(sampling_prob_computation) {
  const int vocab_size = 5;
  float logits[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};

  float prob = sampling_prob_f32(logits, vocab_size, 4);
  ASSERT_TRUE(prob > 0.0f && prob <= 1.0f);

  float sum = 0.0f;
  for (int i = 0; i < vocab_size; i++) {
    sum += sampling_prob_f32(logits, vocab_size, i);
  }
  ASSERT_NEAR(1.0, sum, 1e-5);
}

TEST(sampling_rng_consistency) {
  sampling_rng_t rng1, rng2;
  sampling_rng_init(&rng1, 42);
  sampling_rng_init(&rng2, 42);

  float val1 = sampling_rng_f32(&rng1);
  float val2 = sampling_rng_f32(&rng2);
  ASSERT_NEAR(val1, val2, 1e-6);

  val1 = sampling_rng_f32(&rng1);
  val2 = sampling_rng_f32(&rng2);
  ASSERT_NEAR(val1, val2, 1e-6);
}

TEST(sampling_combined_filters) {
  const int vocab_size = 10;
  float logits[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                    6.0f, 7.0f, 8.0f, 9.0f, 10.0f};

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled =
      sampling_sample_f32(logits, vocab_size, 0.8f, 5, 0.9f, 0.05f, &rng);
  ASSERT_TRUE(sampled >= 0 && sampled < vocab_size);
}

extern "C" void run_sampling_tests(void) {
  TEST_SUITE("Token Sampling");
  RUN_TEST(sampling_greedy_argmax);
  RUN_TEST(sampling_temperature_scaling);
  RUN_TEST(sampling_top_k);
  RUN_TEST(sampling_top_p);
  RUN_TEST(sampling_min_p);
  RUN_TEST(sampling_prob_computation);
  RUN_TEST(sampling_rng_consistency);
  RUN_TEST(sampling_combined_filters);
}

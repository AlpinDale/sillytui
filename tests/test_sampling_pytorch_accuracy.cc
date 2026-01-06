/*
 * Sampling PyTorch Accuracy Tests
 */

#include "test_framework.h"

extern "C" {
#include "inference/kernels/sampling/sampling.h"
}

#include "inference/model_loader/safetensors.hh"
#include <cmath>
#include <cstring>
#include <string>

static safetensors::safetensors_t g_st;
static bool g_loaded = false;

static bool load_reference_data() {
  if (g_loaded)
    return true;

  std::string warn, err;
  bool ok = safetensors::mmap_from_file("tests/sampling_reference.safetensors",
                                        &g_st, &warn, &err);
  if (!ok) {
    printf("Failed to load sampling reference data: %s\n", err.c_str());
    return false;
  }

  g_loaded = true;
  return true;
}

static const float *get_f32_tensor(const std::string &name, size_t *count) {
  const auto &keys = g_st.tensors.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      safetensors::tensor_t tensor;
      g_st.tensors.at(i, &tensor);
      size_t data_len = tensor.data_offsets[1] - tensor.data_offsets[0];
      *count = data_len / sizeof(float);
      return reinterpret_cast<const float *>(g_st.databuffer_addr +
                                             tensor.data_offsets[0]);
    }
  }
  return nullptr;
}

static int get_metadata_int(const std::string &name) {
  const auto &keys = g_st.metadata.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      std::string val;
      g_st.metadata.at(i, &val);
      return std::stoi(val);
    }
  }
  return -1;
}

static float get_metadata_float(const std::string &name) {
  const auto &keys = g_st.metadata.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      std::string val;
      g_st.metadata.at(i, &val);
      return std::stof(val);
    }
  }
  return -1.0f;
}

static float compute_max_rel_error_prob(const float *expected,
                                        const float *actual, size_t count) {
  float max_err = 0.0f;
  for (size_t i = 0; i < count; i++) {
    float err = fabsf(expected[i] - actual[i]);
    float rel_err = (expected[i] != 0.0f) ? err / fabsf(expected[i]) : err;
    if (rel_err > max_err) {
      max_err = rel_err;
    }
  }
  return max_err;
}

static void run_sampling_prob_test(const char *case_name) {
  if (!load_reference_data()) {
    ASSERT(false);
    return;
  }

  std::string prefix = std::string("f32_") + case_name;
  size_t logits_count, probs_count;

  const float *logits = get_f32_tensor(prefix + "_logits", &logits_count);
  const float *expected_probs =
      get_f32_tensor(prefix + "_probs_greedy", &probs_count);

  if (!logits || !expected_probs) {
    printf("Missing tensors for %s\n", case_name);
    ASSERT(false);
    return;
  }

  int vocab_size = get_metadata_int(prefix + "_vocab_size");

  float *actual_probs = (float *)malloc(vocab_size * sizeof(float));
  for (int i = 0; i < vocab_size; i++) {
    actual_probs[i] = sampling_prob_f32(logits, vocab_size, i);
  }

  float max_err =
      compute_max_rel_error_prob(expected_probs, actual_probs, vocab_size);
  ASSERT_LT(max_err, 1e-4f);

  free(actual_probs);
}

static void run_sampling_greedy_test(const char *case_name) {
  if (!load_reference_data()) {
    ASSERT(false);
    return;
  }

  std::string prefix = std::string("f32_") + case_name;
  size_t logits_count;

  const float *logits = get_f32_tensor(prefix + "_logits", &logits_count);

  if (!logits) {
    printf("Missing tensors for %s\n", case_name);
    ASSERT(false);
    return;
  }

  int vocab_size = get_metadata_int(prefix + "_vocab_size");
  int expected_sampled = get_metadata_int(prefix + "_sampled_greedy");

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled =
      sampling_sample_f32(logits, vocab_size, 0.0f, -1, 1.0f, 0.0f, &rng);
  ASSERT_EQ_INT(expected_sampled, sampled);
}

static void run_sampling_temperature_test(const char *case_name) {
  if (!load_reference_data()) {
    ASSERT(false);
    return;
  }

  std::string prefix = std::string("f32_") + case_name;
  size_t logits_count;

  const float *logits = get_f32_tensor(prefix + "_logits", &logits_count);

  if (!logits) {
    printf("Missing tensors for %s\n", case_name);
    ASSERT(false);
    return;
  }

  int vocab_size = get_metadata_int(prefix + "_vocab_size");
  float temperature = get_metadata_float(prefix + "_temperature");

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int sampled = sampling_sample_f32(logits, vocab_size, temperature, -1, 1.0f,
                                    0.0f, &rng);
  ASSERT_TRUE(sampled >= 0 && sampled < vocab_size);
}

TEST(sampling_pytorch_prob_small) { run_sampling_prob_test("small"); }
TEST(sampling_pytorch_prob_medium) { run_sampling_prob_test("medium"); }
TEST(sampling_pytorch_prob_large) { run_sampling_prob_test("large"); }

TEST(sampling_pytorch_greedy_small) { run_sampling_greedy_test("small"); }
TEST(sampling_pytorch_greedy_medium) { run_sampling_greedy_test("medium"); }
TEST(sampling_pytorch_greedy_large) { run_sampling_greedy_test("large"); }

TEST(sampling_pytorch_temperature_small) {
  run_sampling_temperature_test("small");
}
TEST(sampling_pytorch_temperature_medium) {
  run_sampling_temperature_test("medium");
}
TEST(sampling_pytorch_temperature_large) {
  run_sampling_temperature_test("large");
}

extern "C" void run_sampling_pytorch_tests(void) {
  TEST_SUITE("Sampling PyTorch Accuracy");
  RUN_TEST(sampling_pytorch_prob_small);
  RUN_TEST(sampling_pytorch_prob_medium);
  RUN_TEST(sampling_pytorch_prob_large);
  RUN_TEST(sampling_pytorch_greedy_small);
  RUN_TEST(sampling_pytorch_greedy_medium);
  RUN_TEST(sampling_pytorch_greedy_large);
  RUN_TEST(sampling_pytorch_temperature_small);
  RUN_TEST(sampling_pytorch_temperature_medium);
  RUN_TEST(sampling_pytorch_temperature_large);
}

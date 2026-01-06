/*
 * Softmax PyTorch Accuracy Tests
 */

extern "C" {
#include "inference/kernels/softmax/softmax.h"
#include "test_framework.h"
}

#include "inference/model_loader/safetensors.hh"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

static safetensors::safetensors_t g_st;
static bool g_loaded = false;

static float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static uint16_t float_to_bf16(float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));
  return (uint16_t)(bits >> 16);
}

static float fp16_to_float(uint16_t fp16) {
  uint32_t sign = (fp16 & 0x8000) << 16;
  uint32_t exp = (fp16 >> 10) & 0x1F;
  uint32_t mant = fp16 & 0x3FF;
  uint32_t f32_bits;

  if (exp == 0) {
    f32_bits = (mant == 0) ? sign : (sign | (127 - 14) << 23 | mant << 13);
  } else if (exp == 31) {
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    f32_bits = sign | (((uint32_t)(exp - 15 + 127)) << 23) | (mant << 13);
  }

  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

static uint16_t float_to_fp16(float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));
  uint32_t sign = (bits >> 16) & 0x8000;
  int32_t exp = ((bits >> 23) & 0xFF) - 127 + 15;
  uint32_t mant = (bits >> 13) & 0x3FF;

  if (exp <= 0)
    return (uint16_t)sign;
  if (exp >= 31)
    return (uint16_t)(sign | 0x7C00);
  return (uint16_t)(sign | (exp << 10) | mant);
}

static bool load_reference_data() {
  if (g_loaded)
    return true;

  std::string warn, err;
  bool ok = safetensors::mmap_from_file("tests/softmax_reference.safetensors",
                                        &g_st, &warn, &err);
  if (!ok) {
    printf("Failed to load softmax reference data: %s\n", err.c_str());
    return false;
  }

  g_loaded = true;
  return true;
}

static const float *get_f32_tensor(const std::string &name, int *num_rows,
                                   int *row_size) {
  safetensors::tensor_t tensor;
  bool found = false;

  for (size_t i = 0; i < g_st.tensors.size(); i++) {
    if (g_st.tensors.keys()[i] == name) {
      g_st.tensors.at(i, &tensor);
      found = true;
      break;
    }
  }

  if (!found)
    return nullptr;
  if (tensor.dtype != safetensors::kFLOAT32)
    return nullptr;

  *num_rows = (tensor.shape.size() >= 1) ? tensor.shape[0] : 1;
  *row_size = (tensor.shape.size() >= 2) ? tensor.shape[1] : tensor.shape[0];

  return reinterpret_cast<const float *>(g_st.databuffer_addr +
                                         tensor.data_offsets[0]);
}

static const uint16_t *get_bf16_tensor(const std::string &name, int *num_rows,
                                       int *row_size) {
  safetensors::tensor_t tensor;
  bool found = false;

  for (size_t i = 0; i < g_st.tensors.size(); i++) {
    if (g_st.tensors.keys()[i] == name) {
      g_st.tensors.at(i, &tensor);
      found = true;
      break;
    }
  }

  if (!found)
    return nullptr;
  if (tensor.dtype != safetensors::kBFLOAT16)
    return nullptr;

  *num_rows = (tensor.shape.size() >= 1) ? tensor.shape[0] : 1;
  *row_size = (tensor.shape.size() >= 2) ? tensor.shape[1] : tensor.shape[0];

  return reinterpret_cast<const uint16_t *>(g_st.databuffer_addr +
                                            tensor.data_offsets[0]);
}

static const uint16_t *get_f16_tensor(const std::string &name, int *num_rows,
                                      int *row_size) {
  safetensors::tensor_t tensor;
  bool found = false;

  for (size_t i = 0; i < g_st.tensors.size(); i++) {
    if (g_st.tensors.keys()[i] == name) {
      g_st.tensors.at(i, &tensor);
      found = true;
      break;
    }
  }

  if (!found)
    return nullptr;
  if (tensor.dtype != safetensors::kFLOAT16)
    return nullptr;

  *num_rows = (tensor.shape.size() >= 1) ? tensor.shape[0] : 1;
  *row_size = (tensor.shape.size() >= 2) ? tensor.shape[1] : tensor.shape[0];

  return reinterpret_cast<const uint16_t *>(g_st.databuffer_addr +
                                            tensor.data_offsets[0]);
}

static double compute_max_rel_error(const float *computed,
                                    const float *expected, int n) {
  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    double diff = fabs((double)computed[i] - (double)expected[i]);
    double mag = fabs((double)expected[i]);
    double rel_err = diff / (mag + 1e-10);
    if (rel_err > max_err)
      max_err = rel_err;
  }
  return max_err;
}

static void run_softmax_f32_test(const char *test_name, const char *case_name) {
  if (!load_reference_data()) {
    ASSERT(false);
    return;
  }

  std::string input_key = std::string("f32_") + case_name + "_input";
  std::string output_key = std::string("f32_") + case_name + "_output";

  int num_rows, row_size;
  const float *input = get_f32_tensor(input_key, &num_rows, &row_size);
  const float *expected = get_f32_tensor(output_key, &num_rows, &row_size);

  if (!input || !expected) {
    printf("Missing tensors for %s\n", case_name);
    ASSERT(false);
    return;
  }

  int total = num_rows * row_size;
  std::vector<float> output(total);

  softmax_f32(output.data(), input, num_rows, row_size);

  double max_err = compute_max_rel_error(output.data(), expected, total);
  ASSERT_LT(max_err, 1e-5);
}

static void run_softmax_bf16_test(const char *case_name) {
  if (!load_reference_data()) {
    ASSERT(false);
    return;
  }

  std::string input_key = std::string("bf16_") + case_name + "_input";
  std::string output_key = std::string("bf16_") + case_name + "_output";

  int num_rows, row_size;
  const uint16_t *input = get_bf16_tensor(input_key, &num_rows, &row_size);
  const uint16_t *expected = get_bf16_tensor(output_key, &num_rows, &row_size);

  if (!input || !expected) {
    printf("Missing tensors for %s\n", case_name);
    ASSERT(false);
    return;
  }

  int total = num_rows * row_size;
  std::vector<uint16_t> output(total);

  softmax_bf16(output.data(), input, num_rows, row_size);

  double max_err = 0.0;
  for (int i = 0; i < total; i++) {
    double computed = bf16_to_float(output[i]);
    double exp_val = bf16_to_float(expected[i]);
    double diff = fabs(computed - exp_val);
    double mag = fabs(exp_val);
    double rel_err = diff / (mag + 1e-6);
    if (rel_err > max_err)
      max_err = rel_err;
  }

  ASSERT_LT(max_err, 0.02);
}

static void run_softmax_f16_test(const char *case_name) {
  if (!load_reference_data()) {
    ASSERT(false);
    return;
  }

  std::string input_key = std::string("f16_") + case_name + "_input";
  std::string output_key = std::string("f16_") + case_name + "_output";

  int num_rows, row_size;
  const uint16_t *input = get_f16_tensor(input_key, &num_rows, &row_size);
  const uint16_t *expected = get_f16_tensor(output_key, &num_rows, &row_size);

  if (!input || !expected) {
    printf("Missing tensors for %s\n", case_name);
    ASSERT(false);
    return;
  }

  int total = num_rows * row_size;
  std::vector<uint16_t> output(total);

  softmax_f16(output.data(), input, num_rows, row_size);

  double max_err = 0.0;
  for (int i = 0; i < total; i++) {
    double computed = fp16_to_float(output[i]);
    double exp_val = fp16_to_float(expected[i]);
    double diff = fabs(computed - exp_val);
    double mag = fabs(exp_val);
    double rel_err = diff / (mag + 1e-6);
    if (rel_err > max_err)
      max_err = rel_err;
  }

  ASSERT_LT(max_err, 0.01);
}

TEST(softmax_pytorch_f32_small) { run_softmax_f32_test("small", "small"); }
TEST(softmax_pytorch_f32_medium) { run_softmax_f32_test("medium", "medium"); }
TEST(softmax_pytorch_f32_large) { run_softmax_f32_test("large", "large"); }
TEST(softmax_pytorch_f32_attention_128) {
  run_softmax_f32_test("attention_128", "attention_128");
}
TEST(softmax_pytorch_f32_attention_512) {
  run_softmax_f32_test("attention_512", "attention_512");
}
TEST(softmax_pytorch_f32_attention_2048) {
  run_softmax_f32_test("attention_2048", "attention_2048");
}

TEST(softmax_pytorch_f32_scaled) {
  if (!load_reference_data()) {
    ASSERT(false);
    return;
  }

  int num_rows, row_size;
  const float *input = get_f32_tensor("f32_scaled_input", &num_rows, &row_size);
  const float *expected =
      get_f32_tensor("f32_scaled_output", &num_rows, &row_size);

  int scale_rows, scale_size;
  const float *scale_ptr =
      get_f32_tensor("f32_scaled_scale", &scale_rows, &scale_size);
  float scale = scale_ptr ? scale_ptr[0] : 0.125f;

  if (!input || !expected) {
    ASSERT(false);
    return;
  }

  int total = num_rows * row_size;
  std::vector<float> output(total);

  softmax_f32_scaled(output.data(), input, num_rows, row_size, scale);

  double max_err = compute_max_rel_error(output.data(), expected, total);
  ASSERT_LT(max_err, 1e-5);
}

TEST(softmax_pytorch_bf16_small) { run_softmax_bf16_test("small"); }
TEST(softmax_pytorch_bf16_medium) { run_softmax_bf16_test("medium"); }
TEST(softmax_pytorch_bf16_large) { run_softmax_bf16_test("large"); }

TEST(softmax_pytorch_f16_small) { run_softmax_f16_test("small"); }
TEST(softmax_pytorch_f16_medium) { run_softmax_f16_test("medium"); }
TEST(softmax_pytorch_f16_large) { run_softmax_f16_test("large"); }

extern "C" void run_softmax_pytorch_tests(void) {
  TEST_SUITE("Softmax PyTorch Accuracy");
  RUN_TEST(softmax_pytorch_f32_small);
  RUN_TEST(softmax_pytorch_f32_medium);
  RUN_TEST(softmax_pytorch_f32_large);
  RUN_TEST(softmax_pytorch_f32_attention_128);
  RUN_TEST(softmax_pytorch_f32_attention_512);
  RUN_TEST(softmax_pytorch_f32_attention_2048);
  RUN_TEST(softmax_pytorch_f32_scaled);
  RUN_TEST(softmax_pytorch_bf16_small);
  RUN_TEST(softmax_pytorch_bf16_medium);
  RUN_TEST(softmax_pytorch_bf16_large);
  RUN_TEST(softmax_pytorch_f16_small);
  RUN_TEST(softmax_pytorch_f16_medium);
  RUN_TEST(softmax_pytorch_f16_large);
}

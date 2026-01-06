/*
 * Activation PyTorch Accuracy Tests
 */

#include "inference/model_loader/safetensors.hh"
#include "test_framework.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include "inference/kernels/activation/activation.h"
}

static safetensors::safetensors_t g_reference_data;
static bool g_reference_loaded = false;

static bool load_reference_data() {
  if (g_reference_loaded)
    return true;

  std::string warn, err;
  bool ret = safetensors::mmap_from_file(
      "tests/activation_reference.safetensors", &g_reference_data, &warn, &err);
  if (!ret) {
    fprintf(stderr, "Failed to load activation reference: %s\n", err.c_str());
    return false;
  }

  g_reference_loaded = true;
  return true;
}

static float *load_tensor(const char *name, size_t *out_size) {
  if (!load_reference_data())
    return nullptr;

  safetensors::tensor_t tensor;
  bool found = false;

  for (size_t i = 0; i < g_reference_data.tensors.size(); i++) {
    std::string key = g_reference_data.tensors.keys()[i];
    if (key == name) {
      g_reference_data.tensors.at(i, &tensor);
      found = true;
      break;
    }
  }

  if (!found)
    return nullptr;

  if (tensor.dtype != safetensors::dtype::kFLOAT32)
    return nullptr;

  size_t total = safetensors::get_shape_size(tensor);
  float *data = (float *)malloc(total * sizeof(float));
  if (!data)
    return nullptr;

  const uint8_t *src =
      g_reference_data.databuffer_addr + tensor.data_offsets[0];
  memcpy(data, src, total * sizeof(float));

  *out_size = total;
  return data;
}

static uint16_t *load_tensor_bf16(const char *name, size_t *out_size) {
  if (!load_reference_data())
    return nullptr;

  safetensors::tensor_t tensor;
  bool found = false;

  for (size_t i = 0; i < g_reference_data.tensors.size(); i++) {
    std::string key = g_reference_data.tensors.keys()[i];
    if (key == name) {
      g_reference_data.tensors.at(i, &tensor);
      found = true;
      break;
    }
  }

  if (!found)
    return nullptr;

  if (tensor.dtype != safetensors::dtype::kBFLOAT16)
    return nullptr;

  size_t total = safetensors::get_shape_size(tensor);
  uint16_t *data = (uint16_t *)malloc(total * sizeof(uint16_t));
  if (!data)
    return nullptr;

  const uint8_t *src =
      g_reference_data.databuffer_addr + tensor.data_offsets[0];
  memcpy(data, src, total * sizeof(uint16_t));

  *out_size = total;
  return data;
}

static uint16_t *load_tensor_f16(const char *name, size_t *out_size) {
  if (!load_reference_data())
    return nullptr;

  safetensors::tensor_t tensor;
  bool found = false;

  for (size_t i = 0; i < g_reference_data.tensors.size(); i++) {
    std::string key = g_reference_data.tensors.keys()[i];
    if (key == name) {
      g_reference_data.tensors.at(i, &tensor);
      found = true;
      break;
    }
  }

  if (!found)
    return nullptr;

  if (tensor.dtype != safetensors::dtype::kFLOAT16)
    return nullptr;

  size_t total = safetensors::get_shape_size(tensor);
  uint16_t *data = (uint16_t *)malloc(total * sizeof(uint16_t));
  if (!data)
    return nullptr;

  const uint8_t *src =
      g_reference_data.databuffer_addr + tensor.data_offsets[0];
  memcpy(data, src, total * sizeof(uint16_t));

  *out_size = total;
  return data;
}

static float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
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

static double compute_max_relative_error_bf16(const uint16_t *expected,
                                              const uint16_t *actual, size_t n,
                                              double atol = 1e-3) {
  double max_err = 0.0;
  for (size_t i = 0; i < n; i++) {
    double exp_f = bf16_to_float(expected[i]);
    double act_f = bf16_to_float(actual[i]);
    double diff = fabs(exp_f - act_f);
    double mag = fmax(fabs(exp_f), atol);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

static double compute_max_relative_error_f16(const uint16_t *expected,
                                             const uint16_t *actual, size_t n,
                                             double atol = 1e-3) {
  double max_err = 0.0;
  for (size_t i = 0; i < n; i++) {
    double exp_f = fp16_to_float(expected[i]);
    double act_f = fp16_to_float(actual[i]);
    double diff = fabs(exp_f - act_f);
    double mag = fmax(fabs(exp_f), atol);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

static double compute_max_relative_error(const float *expected,
                                         const float *actual, size_t n,
                                         double atol = 1e-6) {
  double max_err = 0.0;
  for (size_t i = 0; i < n; i++) {
    double diff = fabs((double)expected[i] - (double)actual[i]);
    double mag = fmax(fabs((double)expected[i]), atol);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

/* ============ SiLU Tests ============ */

TEST(pytorch_silu_small_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("silu_small_input", &in_size);
  float *expected = load_tensor("silu_small_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  silu_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_silu_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("silu_decode_input", &in_size);
  float *expected = load_tensor("silu_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  silu_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_silu_prefill_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("silu_prefill_input", &in_size);
  float *expected = load_tensor("silu_prefill_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  silu_f32(output, input, 512, 4096);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

/* ============ SwiGLU Tests ============ */

TEST(pytorch_swiglu_small_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("swiglu_small_input", &in_size);
  float *expected = load_tensor("swiglu_small_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  silu_and_mul_f32(output, input, 1, (int)out_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_swiglu_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("swiglu_decode_input", &in_size);
  float *expected = load_tensor("swiglu_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  silu_and_mul_f32(output, input, 1, (int)out_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_swiglu_prefill_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("swiglu_prefill_input", &in_size);
  float *expected = load_tensor("swiglu_prefill_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  silu_and_mul_f32(output, input, 512, 4096);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

/* ============ GELU Tests ============ */

TEST(pytorch_gelu_small_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_small_input", &in_size);
  float *expected = load_tensor("gelu_small_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 0.01) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.01);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_gelu_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_decode_input", &in_size);
  float *expected = load_tensor("gelu_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 0.01) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.01);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_geglu_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("geglu_decode_input", &in_size);
  float *expected = load_tensor("geglu_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_and_mul_f32(output, input, 1, (int)out_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 0.01) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.01);

  free(input);
  free(expected);
  free(output);
}

/* ============ GELU Tanh Tests ============ */

TEST(pytorch_gelu_tanh_small_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_tanh_small_input", &in_size);
  float *expected = load_tensor("gelu_tanh_small_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_tanh_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 0.01) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.01);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_gelu_tanh_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_tanh_decode_input", &in_size);
  float *expected = load_tensor("gelu_tanh_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_tanh_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 0.01) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.01);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_gelu_tanh_mul_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_tanh_mul_decode_input", &in_size);
  float *expected = load_tensor("gelu_tanh_mul_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_tanh_and_mul_f32(output, input, 1, (int)out_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 0.01) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.01);

  free(input);
  free(expected);
  free(output);
}

/* ============ GELU Quick Tests ============ */

TEST(pytorch_gelu_quick_small_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_quick_small_input", &in_size);
  float *expected = load_tensor("gelu_quick_small_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_quick_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_gelu_quick_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_quick_decode_input", &in_size);
  float *expected = load_tensor("gelu_quick_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_quick_f32(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_gelu_quick_mul_decode_f32) {
  size_t in_size, out_size;
  float *input = load_tensor("gelu_quick_mul_decode_input", &in_size);
  float *expected = load_tensor("gelu_quick_mul_decode_output", &out_size);
  ASSERT(input && expected);

  float *output = (float *)malloc(out_size * sizeof(float));
  gelu_quick_and_mul_f32(output, input, 1, (int)out_size);

  double max_err = compute_max_relative_error(expected, output, out_size);
  if (max_err >= 1e-4) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(expected);
  free(output);
}

/* ============ BF16 Tests ============ */

TEST(pytorch_silu_decode_bf16) {
  size_t in_size, out_size;
  uint16_t *input = load_tensor_bf16("silu_decode_bf16_input", &in_size);
  uint16_t *expected = load_tensor_bf16("silu_decode_bf16_output", &out_size);
  ASSERT(input && expected);

  uint16_t *output = (uint16_t *)malloc(out_size * sizeof(uint16_t));
  silu_bf16(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error_bf16(expected, output, out_size);
  if (max_err >= 0.02) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.02);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_swiglu_decode_bf16) {
  size_t in_size, out_size;
  uint16_t *input = load_tensor_bf16("swiglu_decode_bf16_input", &in_size);
  uint16_t *expected = load_tensor_bf16("swiglu_decode_bf16_output", &out_size);
  ASSERT(input && expected);

  uint16_t *output = (uint16_t *)malloc(out_size * sizeof(uint16_t));
  silu_and_mul_bf16(output, input, 1, (int)out_size);

  double max_err = compute_max_relative_error_bf16(expected, output, out_size);
  if (max_err >= 0.02) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.02);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_gelu_decode_bf16) {
  size_t in_size, out_size;
  uint16_t *input = load_tensor_bf16("gelu_decode_bf16_input", &in_size);
  uint16_t *expected = load_tensor_bf16("gelu_decode_bf16_output", &out_size);
  ASSERT(input && expected);

  uint16_t *output = (uint16_t *)malloc(out_size * sizeof(uint16_t));
  gelu_bf16(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error_bf16(expected, output, out_size);
  if (max_err >= 0.05) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.05);

  free(input);
  free(expected);
  free(output);
}

/* ============ FP16 Tests ============ */

TEST(pytorch_silu_decode_f16) {
  size_t in_size, out_size;
  uint16_t *input = load_tensor_f16("silu_decode_f16_input", &in_size);
  uint16_t *expected = load_tensor_f16("silu_decode_f16_output", &out_size);
  ASSERT(input && expected);

  uint16_t *output = (uint16_t *)malloc(out_size * sizeof(uint16_t));
  silu_f16(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error_f16(expected, output, out_size);
  if (max_err >= 0.02) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.02);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_swiglu_decode_f16) {
  size_t in_size, out_size;
  uint16_t *input = load_tensor_f16("swiglu_decode_f16_input", &in_size);
  uint16_t *expected = load_tensor_f16("swiglu_decode_f16_output", &out_size);
  ASSERT(input && expected);

  uint16_t *output = (uint16_t *)malloc(out_size * sizeof(uint16_t));
  silu_and_mul_f16(output, input, 1, (int)out_size);

  double max_err = compute_max_relative_error_f16(expected, output, out_size);
  if (max_err >= 0.02) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.02);

  free(input);
  free(expected);
  free(output);
}

TEST(pytorch_gelu_decode_f16) {
  size_t in_size, out_size;
  uint16_t *input = load_tensor_f16("gelu_decode_f16_input", &in_size);
  uint16_t *expected = load_tensor_f16("gelu_decode_f16_output", &out_size);
  ASSERT(input && expected);

  uint16_t *output = (uint16_t *)malloc(out_size * sizeof(uint16_t));
  gelu_f16(output, input, 1, (int)in_size);

  double max_err = compute_max_relative_error_f16(expected, output, out_size);
  if (max_err >= 0.05) {
    printf("max_err = %.6e ", max_err);
  }
  ASSERT_LT(max_err, 0.05);

  free(input);
  free(expected);
  free(output);
}

/* ============ Test Registration ============ */

extern "C" void run_activation_pytorch_tests(void) {
  TEST_SUITE("Activation (FP32/FP16/BF16) - PyTorch Accuracy");
  RUN_TEST(pytorch_silu_small_f32);
  RUN_TEST(pytorch_silu_decode_f32);
  RUN_TEST(pytorch_silu_prefill_f32);
  RUN_TEST(pytorch_swiglu_small_f32);
  RUN_TEST(pytorch_swiglu_decode_f32);
  RUN_TEST(pytorch_swiglu_prefill_f32);
  RUN_TEST(pytorch_gelu_small_f32);
  RUN_TEST(pytorch_gelu_decode_f32);
  RUN_TEST(pytorch_geglu_decode_f32);
  RUN_TEST(pytorch_gelu_tanh_small_f32);
  RUN_TEST(pytorch_gelu_tanh_decode_f32);
  RUN_TEST(pytorch_gelu_tanh_mul_decode_f32);
  RUN_TEST(pytorch_gelu_quick_small_f32);
  RUN_TEST(pytorch_gelu_quick_decode_f32);
  RUN_TEST(pytorch_gelu_quick_mul_decode_f32);
  RUN_TEST(pytorch_silu_decode_bf16);
  RUN_TEST(pytorch_swiglu_decode_bf16);
  RUN_TEST(pytorch_gelu_decode_bf16);
  RUN_TEST(pytorch_silu_decode_f16);
  RUN_TEST(pytorch_swiglu_decode_f16);
  RUN_TEST(pytorch_gelu_decode_f16);
}

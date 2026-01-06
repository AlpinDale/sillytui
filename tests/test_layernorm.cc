/*
 * LayerNorm Tests - PyTorch Accuracy Validation
 */

#include "inference/model_loader/safetensors.hh"
#include "test_framework.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/norm/layernorm.h"
}

static safetensors::safetensors_t g_reference_data;
static bool g_reference_loaded = false;

static bool load_reference_data() {
  if (g_reference_loaded)
    return true;

  std::string warn, err;
  bool ret = safetensors::mmap_from_file(
      "tests/layernorm_reference.safetensors", &g_reference_data, &warn, &err);
  if (!ret) {
    fprintf(stderr, "Failed to load layernorm reference: %s\n", err.c_str());
    return false;
  }

  g_reference_loaded = true;
  return true;
}

static float *load_tensor(const char *name, int *rows, int *cols) {
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

  if (tensor.shape.size() == 2) {
    *rows = tensor.shape[0];
    *cols = tensor.shape[1];
  } else if (tensor.shape.size() == 1) {
    *rows = 1;
    *cols = tensor.shape[0];
  } else {
    free(data);
    return nullptr;
  }

  return data;
}

static int get_metadata_int(const char *key) {
  for (size_t i = 0; i < g_reference_data.metadata.size(); i++) {
    std::string k = g_reference_data.metadata.keys()[i];
    if (k == key) {
      std::string val;
      g_reference_data.metadata.at(i, &val);
      return std::stoi(val);
    }
  }
  return -1;
}

static double compute_max_error(const float *a, const float *b, int n) {
  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    double err = fabs((double)a[i] - (double)b[i]);
    double mag = fmax(fabs((double)a[i]), fabs((double)b[i]));
    double threshold = 0.05 + 0.05 * mag;
    double normalized_err = err / fmax(threshold, 1e-6);
    if (normalized_err > max_err)
      max_err = normalized_err;
  }
  return max_err;
}

/* ============ RMSNorm Tests ============ */

TEST(layernorm_rms_f32_decode) {
  int rows, cols;
  float *input = load_tensor("rms_f32_decode.input", &rows, &cols);
  float *weight = load_tensor("rms_f32_decode.weight", &rows, &cols);
  float *expected = load_tensor("rms_f32_decode.out", &rows, &cols);
  ASSERT(input && weight && expected);

  int num_tokens = get_metadata_int("rms_f32_decode.num_tokens");
  int hidden_size = get_metadata_int("rms_f32_decode.hidden_size");
  ASSERT(num_tokens > 0 && hidden_size > 0);

  float *out = (float *)malloc(num_tokens * hidden_size * sizeof(float));
  ASSERT(out);

  rms_norm_f32(out, input, weight, 1e-6f, num_tokens, hidden_size);

  double err = compute_max_error(out, expected, num_tokens * hidden_size);
  ASSERT(err < 1.0);

  free(input);
  free(weight);
  free(expected);
  free(out);
  PASS();
}

TEST(layernorm_rms_bf16_decode) {
  int rows, cols;
  float *input_f32 = load_tensor("rms_bf16_decode.input", &rows, &cols);
  float *weight_f32 = load_tensor("rms_bf16_decode.weight", &rows, &cols);
  float *expected_f32 = load_tensor("rms_bf16_decode.out", &rows, &cols);
  ASSERT(input_f32 && weight_f32 && expected_f32);

  int num_tokens = get_metadata_int("rms_bf16_decode.num_tokens");
  int hidden_size = get_metadata_int("rms_bf16_decode.hidden_size");
  ASSERT(num_tokens > 0 && hidden_size > 0);

  uint16_t *input =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  uint16_t *weight = (uint16_t *)malloc(hidden_size * sizeof(uint16_t));
  uint16_t *out =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  ASSERT(input && weight && out);

  f32_array_to_bf16(input_f32, input, num_tokens * hidden_size);
  f32_array_to_bf16(weight_f32, weight, hidden_size);

  rms_norm_bf16(out, input, weight, 1e-6f, num_tokens, hidden_size);

  float *out_f32 = (float *)malloc(num_tokens * hidden_size * sizeof(float));
  ASSERT(out_f32);
  bf16_array_to_f32(out, out_f32, num_tokens * hidden_size);

  double err =
      compute_max_error(out_f32, expected_f32, num_tokens * hidden_size);
  ASSERT(err < 1.0);

  free(input_f32);
  free(weight_f32);
  free(expected_f32);
  free(input);
  free(weight);
  free(out);
  free(out_f32);
  PASS();
}

TEST(layernorm_rms_f16_decode) {
  int rows, cols;
  float *input_f32 = load_tensor("rms_f16_decode.input", &rows, &cols);
  float *weight_f32 = load_tensor("rms_f16_decode.weight", &rows, &cols);
  float *expected_f32 = load_tensor("rms_f16_decode.out", &rows, &cols);
  ASSERT(input_f32 && weight_f32 && expected_f32);

  int num_tokens = get_metadata_int("rms_f16_decode.num_tokens");
  int hidden_size = get_metadata_int("rms_f16_decode.hidden_size");
  ASSERT(num_tokens > 0 && hidden_size > 0);

  uint16_t *input =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  uint16_t *weight = (uint16_t *)malloc(hidden_size * sizeof(uint16_t));
  uint16_t *out =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  ASSERT(input && weight && out);

  f32_array_to_f16(input_f32, input, num_tokens * hidden_size);
  f32_array_to_f16(weight_f32, weight, hidden_size);

  rms_norm_f16(out, input, weight, 1e-6f, num_tokens, hidden_size);

  float *out_f32 = (float *)malloc(num_tokens * hidden_size * sizeof(float));
  ASSERT(out_f32);
  f16_array_to_f32(out, out_f32, num_tokens * hidden_size);

  double err =
      compute_max_error(out_f32, expected_f32, num_tokens * hidden_size);
  ASSERT(err < 1.0);

  free(input_f32);
  free(weight_f32);
  free(expected_f32);
  free(input);
  free(weight);
  free(out);
  free(out_f32);
  PASS();
}

/* ============ Fused Add + RMSNorm Tests ============ */

TEST(layernorm_fused_f32_decode) {
  int rows, cols;
  float *input = load_tensor("fused_f32_decode.input", &rows, &cols);
  float *weight = load_tensor("fused_f32_decode.weight", &rows, &cols);
  float *residual_in =
      load_tensor("fused_f32_decode.residual_in", &rows, &cols);
  float *expected_out = load_tensor("fused_f32_decode.out", &rows, &cols);
  float *expected_res =
      load_tensor("fused_f32_decode.residual_out", &rows, &cols);
  ASSERT(input && weight && residual_in && expected_out && expected_res);

  int num_tokens = get_metadata_int("fused_f32_decode.num_tokens");
  int hidden_size = get_metadata_int("fused_f32_decode.hidden_size");
  ASSERT(num_tokens > 0 && hidden_size > 0);

  float *residual = (float *)malloc(num_tokens * hidden_size * sizeof(float));
  float *out = (float *)malloc(num_tokens * hidden_size * sizeof(float));
  ASSERT(residual && out);

  memcpy(residual, residual_in, num_tokens * hidden_size * sizeof(float));

  fused_add_rms_norm_f32(out, input, residual, weight, 1e-6f, num_tokens,
                         hidden_size);

  double err_out =
      compute_max_error(out, expected_out, num_tokens * hidden_size);
  double err_res =
      compute_max_error(residual, expected_res, num_tokens * hidden_size);

  ASSERT(err_out < 1.0);
  ASSERT(err_res < 1.0);

  free(input);
  free(weight);
  free(residual_in);
  free(expected_out);
  free(expected_res);
  free(residual);
  free(out);
  PASS();
}

TEST(layernorm_fused_bf16_prefill) {
  int rows, cols;
  float *input_f32 = load_tensor("fused_bf16_prefill.input", &rows, &cols);
  float *weight_f32 = load_tensor("fused_bf16_prefill.weight", &rows, &cols);
  float *residual_in_f32 =
      load_tensor("fused_bf16_prefill.residual_in", &rows, &cols);
  float *expected_out_f32 = load_tensor("fused_bf16_prefill.out", &rows, &cols);
  float *expected_res_f32 =
      load_tensor("fused_bf16_prefill.residual_out", &rows, &cols);
  ASSERT(input_f32 && weight_f32 && residual_in_f32 && expected_out_f32 &&
         expected_res_f32);

  int num_tokens = get_metadata_int("fused_bf16_prefill.num_tokens");
  int hidden_size = get_metadata_int("fused_bf16_prefill.hidden_size");
  ASSERT(num_tokens > 0 && hidden_size > 0);

  uint16_t *input =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  uint16_t *weight = (uint16_t *)malloc(hidden_size * sizeof(uint16_t));
  uint16_t *residual =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  uint16_t *out =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  ASSERT(input && weight && residual && out);

  f32_array_to_bf16(input_f32, input, num_tokens * hidden_size);
  f32_array_to_bf16(weight_f32, weight, hidden_size);
  f32_array_to_bf16(residual_in_f32, residual, num_tokens * hidden_size);

  fused_add_rms_norm_bf16(out, input, residual, weight, 1e-6f, num_tokens,
                          hidden_size);

  float *out_f32 = (float *)malloc(num_tokens * hidden_size * sizeof(float));
  float *residual_f32 =
      (float *)malloc(num_tokens * hidden_size * sizeof(float));
  ASSERT(out_f32 && residual_f32);

  bf16_array_to_f32(out, out_f32, num_tokens * hidden_size);
  bf16_array_to_f32(residual, residual_f32, num_tokens * hidden_size);

  double err_out =
      compute_max_error(out_f32, expected_out_f32, num_tokens * hidden_size);
  double err_res = compute_max_error(residual_f32, expected_res_f32,
                                     num_tokens * hidden_size);

  ASSERT(err_out < 1.0);
  ASSERT(err_res < 1.0);

  free(input_f32);
  free(weight_f32);
  free(residual_in_f32);
  free(expected_out_f32);
  free(expected_res_f32);
  free(input);
  free(weight);
  free(residual);
  free(out);
  free(out_f32);
  free(residual_f32);
  PASS();
}

TEST(layernorm_fused_f16_prefill) {
  int rows, cols;
  float *input_f32 = load_tensor("fused_f16_prefill.input", &rows, &cols);
  float *weight_f32 = load_tensor("fused_f16_prefill.weight", &rows, &cols);
  float *residual_in_f32 =
      load_tensor("fused_f16_prefill.residual_in", &rows, &cols);
  float *expected_out_f32 = load_tensor("fused_f16_prefill.out", &rows, &cols);
  float *expected_res_f32 =
      load_tensor("fused_f16_prefill.residual_out", &rows, &cols);
  ASSERT(input_f32 && weight_f32 && residual_in_f32 && expected_out_f32 &&
         expected_res_f32);

  int num_tokens = get_metadata_int("fused_f16_prefill.num_tokens");
  int hidden_size = get_metadata_int("fused_f16_prefill.hidden_size");
  ASSERT(num_tokens > 0 && hidden_size > 0);

  uint16_t *input =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  uint16_t *weight = (uint16_t *)malloc(hidden_size * sizeof(uint16_t));
  uint16_t *residual =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  uint16_t *out =
      (uint16_t *)malloc(num_tokens * hidden_size * sizeof(uint16_t));
  ASSERT(input && weight && residual && out);

  f32_array_to_f16(input_f32, input, num_tokens * hidden_size);
  f32_array_to_f16(weight_f32, weight, hidden_size);
  f32_array_to_f16(residual_in_f32, residual, num_tokens * hidden_size);

  fused_add_rms_norm_f16(out, input, residual, weight, 1e-6f, num_tokens,
                         hidden_size);

  float *out_f32 = (float *)malloc(num_tokens * hidden_size * sizeof(float));
  float *residual_f32 =
      (float *)malloc(num_tokens * hidden_size * sizeof(float));
  ASSERT(out_f32 && residual_f32);

  f16_array_to_f32(out, out_f32, num_tokens * hidden_size);
  f16_array_to_f32(residual, residual_f32, num_tokens * hidden_size);

  double err_out =
      compute_max_error(out_f32, expected_out_f32, num_tokens * hidden_size);
  double err_res = compute_max_error(residual_f32, expected_res_f32,
                                     num_tokens * hidden_size);

  ASSERT(err_out < 1.0);
  ASSERT(err_res < 1.0);

  free(input_f32);
  free(weight_f32);
  free(residual_in_f32);
  free(expected_out_f32);
  free(expected_res_f32);
  free(input);
  free(weight);
  free(residual);
  free(out);
  free(out_f32);
  free(residual_f32);
  PASS();
}

extern "C" void run_layernorm_tests(void) {
  TEST_SUITE("LayerNorm (FP32/FP16/BF16)");
  RUN_TEST(layernorm_rms_f32_decode);
  RUN_TEST(layernorm_rms_bf16_decode);
  RUN_TEST(layernorm_rms_f16_decode);
  RUN_TEST(layernorm_fused_f32_decode);
  RUN_TEST(layernorm_fused_bf16_prefill);
  RUN_TEST(layernorm_fused_f16_prefill);
}

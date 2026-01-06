/*
 * Activation Function Tests
 */

extern "C" {
#include "inference/kernels/activation/activation.h"
#include "test_framework.h"
}

#include <cmath>
#include <cstdlib>
#include <cstring>

static inline float scalar_silu(float x) { return x / (1.0f + expf(-x)); }

static inline float scalar_gelu(float x) {
  return x * 0.5f * (1.0f + erff(x * 0.70710678118f));
}

static inline float scalar_gelu_tanh(float x) {
  float x3 = x * x * x;
  float inner = 0.7978845608f * (x + 0.044715f * x3);
  return x * 0.5f * (1.0f + tanhf(inner));
}

static inline float scalar_gelu_quick(float x) {
  return x / (1.0f + expf(-1.702f * x));
}

static inline uint16_t float_to_bf16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t lsb = (bits >> 16) & 1;
  bits += 0x7fff + lsb;
  return (uint16_t)(bits >> 16);
}

static inline float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_fp16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t sign = bits & 0x80000000;
  uint32_t exp = (bits >> 23) & 0xFF;
  uint32_t mant = bits & 0x7FFFFF;
  if (exp == 0xFF)
    return (uint16_t)((sign >> 16) | 0x7C00);
  if (exp == 0 && mant == 0)
    return (uint16_t)(sign >> 16);
  int32_t new_exp = (int32_t)exp - 127 + 15;
  if (new_exp <= 0)
    return (uint16_t)(sign >> 16);
  if (new_exp >= 31)
    return (uint16_t)((sign >> 16) | 0x7C00);
  return (uint16_t)((sign >> 16) | (new_exp << 10) | (mant >> 13));
}

static inline float fp16_to_float(uint16_t fp16) {
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

static double compute_max_relative_error(const float *expected,
                                         const float *actual, int n,
                                         double atol) {
  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    double diff = fabs((double)expected[i] - (double)actual[i]);
    double mag = fmax(fabs((double)expected[i]), atol);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

/* ============ SiLU Tests ============ */

TEST(silu_f32_basic) {
  const int n = 128;
  float input[n], output[n], expected[n];

  for (int i = 0; i < n; i++) {
    input[i] = ((float)i / n) * 4.0f - 2.0f;
    expected[i] = scalar_silu(input[i]);
  }

  silu_f32(output, input, 1, n);

  double max_err = compute_max_relative_error(expected, output, n, 1e-6);
  ASSERT_LT(max_err, 1e-4);
}

TEST(silu_f32_batch) {
  const int num_tokens = 4;
  const int d = 64;
  float input[num_tokens * d], output[num_tokens * d], expected[num_tokens * d];

  for (int i = 0; i < num_tokens * d; i++) {
    input[i] = ((float)(i % 100) / 50.0f) - 1.0f;
    expected[i] = scalar_silu(input[i]);
  }

  silu_f32(output, input, num_tokens, d);

  double max_err =
      compute_max_relative_error(expected, output, num_tokens * d, 1e-6);
  ASSERT_LT(max_err, 1e-4);
}

TEST(silu_bf16_basic) {
  const int n = 128;
  uint16_t input[n], output[n];
  float expected[n];

  for (int i = 0; i < n; i++) {
    float val = ((float)i / n) * 4.0f - 2.0f;
    input[i] = float_to_bf16(val);
    expected[i] = scalar_silu(bf16_to_float(input[i]));
  }

  silu_bf16(output, input, 1, n);

  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    float actual = bf16_to_float(output[i]);
    double diff = fabs((double)expected[i] - (double)actual);
    double mag = fmax(fabs((double)expected[i]), 1e-3);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  ASSERT_LT(max_err, 0.02);
}

TEST(silu_f16_basic) {
  const int n = 128;
  uint16_t input[n], output[n];
  float expected[n];

  for (int i = 0; i < n; i++) {
    float val = ((float)i / n) * 4.0f - 2.0f;
    input[i] = float_to_fp16(val);
    expected[i] = scalar_silu(fp16_to_float(input[i]));
  }

  silu_f16(output, input, 1, n);

  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    float actual = fp16_to_float(output[i]);
    double diff = fabs((double)expected[i] - (double)actual);
    double mag = fmax(fabs((double)expected[i]), 1e-3);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  ASSERT_LT(max_err, 0.02);
}

/* ============ SwiGLU Tests ============ */

TEST(silu_and_mul_f32_basic) {
  const int d = 64;
  float input[2 * d], output[d], expected[d];

  for (int i = 0; i < d; i++) {
    input[i] = ((float)i / d) * 2.0f - 1.0f;
    input[d + i] = ((float)(d - i) / d) * 2.0f - 1.0f;
    expected[i] = scalar_silu(input[i]) * input[d + i];
  }

  silu_and_mul_f32(output, input, 1, d);

  double max_err = compute_max_relative_error(expected, output, d, 1e-6);
  ASSERT_LT(max_err, 1e-4);
}

TEST(silu_and_mul_f32_batch) {
  const int num_tokens = 4;
  const int d = 64;
  float input[num_tokens * 2 * d], output[num_tokens * d],
      expected[num_tokens * d];

  for (int t = 0; t < num_tokens; t++) {
    for (int i = 0; i < d; i++) {
      float x = ((float)(i + t) / (d + num_tokens)) * 2.0f - 1.0f;
      float gate = ((float)(d - i + t) / (d + num_tokens)) * 2.0f - 1.0f;
      input[t * 2 * d + i] = x;
      input[t * 2 * d + d + i] = gate;
      expected[t * d + i] = scalar_silu(x) * gate;
    }
  }

  silu_and_mul_f32(output, input, num_tokens, d);

  double max_err =
      compute_max_relative_error(expected, output, num_tokens * d, 1e-6);
  ASSERT_LT(max_err, 1e-4);
}

TEST(silu_and_mul_bf16_basic) {
  const int d = 64;
  uint16_t input[2 * d], output[d];
  float expected[d];

  for (int i = 0; i < d; i++) {
    float x = ((float)i / d) * 2.0f - 1.0f;
    float gate = ((float)(d - i) / d) * 2.0f - 1.0f;
    input[i] = float_to_bf16(x);
    input[d + i] = float_to_bf16(gate);
    expected[i] =
        scalar_silu(bf16_to_float(input[i])) * bf16_to_float(input[d + i]);
  }

  silu_and_mul_bf16(output, input, 1, d);

  double max_err = 0.0;
  for (int i = 0; i < d; i++) {
    float actual = bf16_to_float(output[i]);
    double diff = fabs((double)expected[i] - (double)actual);
    double mag = fmax(fabs((double)expected[i]), 1e-3);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  ASSERT_LT(max_err, 0.02);
}

TEST(silu_and_mul_f16_basic) {
  const int d = 64;
  uint16_t input[2 * d], output[d];
  float expected[d];

  for (int i = 0; i < d; i++) {
    float x = ((float)i / d) * 2.0f - 1.0f;
    float gate = ((float)(d - i) / d) * 2.0f - 1.0f;
    input[i] = float_to_fp16(x);
    input[d + i] = float_to_fp16(gate);
    expected[i] =
        scalar_silu(fp16_to_float(input[i])) * fp16_to_float(input[d + i]);
  }

  silu_and_mul_f16(output, input, 1, d);

  double max_err = 0.0;
  for (int i = 0; i < d; i++) {
    float actual = fp16_to_float(output[i]);
    double diff = fabs((double)expected[i] - (double)actual);
    double mag = fmax(fabs((double)expected[i]), 1e-3);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  ASSERT_LT(max_err, 0.02);
}

/* ============ GELU Tests ============ */

TEST(gelu_f32_basic) {
  const int n = 128;
  float input[n], output[n], expected[n];

  for (int i = 0; i < n; i++) {
    input[i] = ((float)i / n) * 4.0f - 2.0f;
    expected[i] = scalar_gelu(input[i]);
  }

  gelu_f32(output, input, 1, n);

  double max_err = compute_max_relative_error(expected, output, n, 1e-6);
  ASSERT_LT(max_err, 0.01);
}

TEST(gelu_bf16_basic) {
  const int n = 128;
  uint16_t input[n], output[n];
  float expected[n];

  for (int i = 0; i < n; i++) {
    float val = ((float)i / n) * 4.0f - 2.0f;
    input[i] = float_to_bf16(val);
    expected[i] = scalar_gelu(bf16_to_float(input[i]));
  }

  gelu_bf16(output, input, 1, n);

  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    float actual = bf16_to_float(output[i]);
    double diff = fabs((double)expected[i] - (double)actual);
    double mag = fmax(fabs((double)expected[i]), 1e-3);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  ASSERT_LT(max_err, 0.05);
}

TEST(gelu_and_mul_f32_basic) {
  const int d = 64;
  float input[2 * d], output[d], expected[d];

  for (int i = 0; i < d; i++) {
    input[i] = ((float)i / d) * 2.0f - 1.0f;
    input[d + i] = ((float)(d - i) / d) * 2.0f - 1.0f;
    expected[i] = scalar_gelu(input[i]) * input[d + i];
  }

  gelu_and_mul_f32(output, input, 1, d);

  double max_err = compute_max_relative_error(expected, output, d, 1e-6);
  ASSERT_LT(max_err, 0.01);
}

/* ============ GELU Tanh Tests ============ */

TEST(gelu_tanh_f32_basic) {
  const int n = 128;
  float input[n], output[n], expected[n];

  for (int i = 0; i < n; i++) {
    input[i] = ((float)i / n) * 4.0f - 2.0f;
    expected[i] = scalar_gelu_tanh(input[i]);
  }

  gelu_tanh_f32(output, input, 1, n);

  double max_err = compute_max_relative_error(expected, output, n, 1e-6);
  ASSERT_LT(max_err, 0.01);
}

TEST(gelu_tanh_and_mul_f32_basic) {
  const int d = 64;
  float input[2 * d], output[d], expected[d];

  for (int i = 0; i < d; i++) {
    input[i] = ((float)i / d) * 2.0f - 1.0f;
    input[d + i] = ((float)(d - i) / d) * 2.0f - 1.0f;
    expected[i] = scalar_gelu_tanh(input[i]) * input[d + i];
  }

  gelu_tanh_and_mul_f32(output, input, 1, d);

  double max_err = compute_max_relative_error(expected, output, d, 1e-6);
  ASSERT_LT(max_err, 0.01);
}

/* ============ GELU Quick Tests ============ */

TEST(gelu_quick_f32_basic) {
  const int n = 128;
  float input[n], output[n], expected[n];

  for (int i = 0; i < n; i++) {
    input[i] = ((float)i / n) * 4.0f - 2.0f;
    expected[i] = scalar_gelu_quick(input[i]);
  }

  gelu_quick_f32(output, input, 1, n);

  double max_err = compute_max_relative_error(expected, output, n, 1e-6);
  ASSERT_LT(max_err, 1e-4);
}

TEST(gelu_quick_and_mul_f32_basic) {
  const int d = 64;
  float input[2 * d], output[d], expected[d];

  for (int i = 0; i < d; i++) {
    input[i] = ((float)i / d) * 2.0f - 1.0f;
    input[d + i] = ((float)(d - i) / d) * 2.0f - 1.0f;
    expected[i] = scalar_gelu_quick(input[i]) * input[d + i];
  }

  gelu_quick_and_mul_f32(output, input, 1, d);

  double max_err = compute_max_relative_error(expected, output, d, 1e-6);
  ASSERT_LT(max_err, 1e-4);
}

/* ============ Edge Cases ============ */

TEST(silu_zero_input) {
  float input = 0.0f, output;
  silu_f32(&output, &input, 1, 1);
  ASSERT_EQ(output, 0.0f);
}

TEST(silu_large_positive) {
  float input = 10.0f, output;
  silu_f32(&output, &input, 1, 1);
  ASSERT_LT(fabs(output - 10.0f), 0.001f);
}

TEST(silu_large_negative) {
  float input = -10.0f, output;
  silu_f32(&output, &input, 1, 1);
  ASSERT_LT(fabs(output), 0.001f);
}

TEST(gelu_zero_input) {
  float input = 0.0f, output;
  gelu_f32(&output, &input, 1, 1);
  ASSERT_EQ(output, 0.0f);
}

TEST(gelu_large_positive) {
  float input = 10.0f, output;
  gelu_f32(&output, &input, 1, 1);
  ASSERT_LT(fabs(output - 10.0f), 0.001f);
}

TEST(gelu_large_negative) {
  float input = -10.0f, output;
  gelu_f32(&output, &input, 1, 1);
  ASSERT_LT(fabs(output), 0.001f);
}

/* ============ Alignment Tests ============ */

TEST(silu_unaligned_size) {
  const int n = 127;
  float *input = (float *)malloc(n * sizeof(float));
  float *output = (float *)malloc(n * sizeof(float));
  float *expected = (float *)malloc(n * sizeof(float));

  for (int i = 0; i < n; i++) {
    input[i] = ((float)i / n) * 4.0f - 2.0f;
    expected[i] = scalar_silu(input[i]);
  }

  silu_f32(output, input, 1, n);

  double max_err = compute_max_relative_error(expected, output, n, 1e-6);
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(output);
  free(expected);
}

TEST(silu_and_mul_unaligned_size) {
  const int d = 127;
  float *input = (float *)malloc(2 * d * sizeof(float));
  float *output = (float *)malloc(d * sizeof(float));
  float *expected = (float *)malloc(d * sizeof(float));

  for (int i = 0; i < d; i++) {
    input[i] = ((float)i / d) * 2.0f - 1.0f;
    input[d + i] = ((float)(d - i) / d) * 2.0f - 1.0f;
    expected[i] = scalar_silu(input[i]) * input[d + i];
  }

  silu_and_mul_f32(output, input, 1, d);

  double max_err = compute_max_relative_error(expected, output, d, 1e-6);
  ASSERT_LT(max_err, 1e-4);

  free(input);
  free(output);
  free(expected);
}

/* ============ Test Registration ============ */

extern "C" void run_activation_tests(void) {
  TEST_SUITE("Activation (FP32/FP16/BF16)");
  RUN_TEST(silu_f32_basic);
  RUN_TEST(silu_f32_batch);
  RUN_TEST(silu_bf16_basic);
  RUN_TEST(silu_f16_basic);
  RUN_TEST(silu_and_mul_f32_basic);
  RUN_TEST(silu_and_mul_f32_batch);
  RUN_TEST(silu_and_mul_bf16_basic);
  RUN_TEST(silu_and_mul_f16_basic);
  RUN_TEST(gelu_f32_basic);
  RUN_TEST(gelu_bf16_basic);
  RUN_TEST(gelu_and_mul_f32_basic);
  RUN_TEST(gelu_tanh_f32_basic);
  RUN_TEST(gelu_tanh_and_mul_f32_basic);
  RUN_TEST(gelu_quick_f32_basic);
  RUN_TEST(gelu_quick_and_mul_f32_basic);
  RUN_TEST(silu_zero_input);
  RUN_TEST(silu_large_positive);
  RUN_TEST(silu_large_negative);
  RUN_TEST(gelu_zero_input);
  RUN_TEST(gelu_large_positive);
  RUN_TEST(gelu_large_negative);
  RUN_TEST(silu_unaligned_size);
  RUN_TEST(silu_and_mul_unaligned_size);
}

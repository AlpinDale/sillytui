/*
 * Softmax Unit Tests
 */

extern "C" {
#include "inference/kernels/softmax/softmax.h"
#include "test_framework.h"
}

#include <cmath>
#include <cstdlib>
#include <cstring>

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

  if (exp <= 0) {
    return (uint16_t)sign;
  } else if (exp >= 31) {
    return (uint16_t)(sign | 0x7C00);
  }
  return (uint16_t)(sign | (exp << 10) | mant);
}

static void reference_softmax_f32(float *output, const float *input,
                                  int row_size) {
  float max_val = input[0];
  for (int i = 1; i < row_size; i++) {
    if (input[i] > max_val)
      max_val = input[i];
  }

  float sum = 0.0f;
  for (int i = 0; i < row_size; i++) {
    output[i] = expf(input[i] - max_val);
    sum += output[i];
  }

  for (int i = 0; i < row_size; i++) {
    output[i] /= sum;
  }
}

static double compute_max_error(const float *a, const float *b, int n) {
  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    double err = fabs((double)a[i] - (double)b[i]);
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

TEST(softmax_f32_basic) {
  float input[] = {1.0f, 2.0f, 3.0f, 4.0f};
  float output[4];
  float expected[4];

  reference_softmax_f32(expected, input, 4);
  softmax_f32(output, input, 1, 4);

  double max_err = compute_max_error(output, expected, 4);
  ASSERT_LT(max_err, 1e-5);
}

TEST(softmax_f32_sum_to_one) {
  float input[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
  float output[5];

  softmax_f32(output, input, 1, 5);

  float sum = 0.0f;
  for (int i = 0; i < 5; i++) {
    sum += output[i];
  }

  ASSERT_LT(fabs(sum - 1.0f), 1e-5);
}

TEST(softmax_f32_all_equal) {
  float input[] = {1.0f, 1.0f, 1.0f, 1.0f};
  float output[4];

  softmax_f32(output, input, 1, 4);

  for (int i = 0; i < 4; i++) {
    ASSERT_LT(fabs(output[i] - 0.25f), 1e-5);
  }
}

TEST(softmax_f32_large_values) {
  float input[] = {100.0f, 101.0f, 102.0f, 103.0f};
  float output[4];
  float expected[4];

  reference_softmax_f32(expected, input, 4);
  softmax_f32(output, input, 1, 4);

  double max_err = compute_max_error(output, expected, 4);
  ASSERT_LT(max_err, 1e-5);
}

TEST(softmax_f32_negative_values) {
  float input[] = {-1.0f, -2.0f, -3.0f, -4.0f};
  float output[4];
  float expected[4];

  reference_softmax_f32(expected, input, 4);
  softmax_f32(output, input, 1, 4);

  double max_err = compute_max_error(output, expected, 4);
  ASSERT_LT(max_err, 1e-5);
}

TEST(softmax_f32_batch) {
  float input[8] = {1.0f, 2.0f, 3.0f, 4.0f, 0.5f, 1.5f, 2.5f, 3.5f};
  float output[8];

  softmax_f32(output, input, 2, 4);

  float sum1 = output[0] + output[1] + output[2] + output[3];
  float sum2 = output[4] + output[5] + output[6] + output[7];

  ASSERT_LT(fabs(sum1 - 1.0f), 1e-5);
  ASSERT_LT(fabs(sum2 - 1.0f), 1e-5);
}

TEST(softmax_f32_scaled) {
  float input[] = {1.0f, 2.0f, 3.0f, 4.0f};
  float output[4];

  float scale = 0.5f;
  float scaled_input[] = {0.5f, 1.0f, 1.5f, 2.0f};
  float expected[4];
  reference_softmax_f32(expected, scaled_input, 4);

  softmax_f32_scaled(output, input, 1, 4, scale);

  double max_err = compute_max_error(output, expected, 4);
  ASSERT_LT(max_err, 1e-5);
}

TEST(softmax_bf16_basic) {
  uint16_t input[4];
  uint16_t output[4];
  float input_f[] = {1.0f, 2.0f, 3.0f, 4.0f};

  for (int i = 0; i < 4; i++) {
    input[i] = float_to_bf16(input_f[i]);
  }

  softmax_bf16(output, input, 1, 4);

  float sum = 0.0f;
  for (int i = 0; i < 4; i++) {
    sum += bf16_to_float(output[i]);
  }

  ASSERT_LT(fabs(sum - 1.0f), 0.01);
}

TEST(softmax_f16_basic) {
  uint16_t input[4];
  uint16_t output[4];
  float input_f[] = {1.0f, 2.0f, 3.0f, 4.0f};

  for (int i = 0; i < 4; i++) {
    input[i] = float_to_fp16(input_f[i]);
  }

  softmax_f16(output, input, 1, 4);

  float sum = 0.0f;
  for (int i = 0; i < 4; i++) {
    sum += fp16_to_float(output[i]);
  }

  ASSERT_LT(fabs(sum - 1.0f), 0.01);
}

TEST(softmax_f32_inplace) {
  float data[] = {1.0f, 2.0f, 3.0f, 4.0f};
  float expected[4];

  reference_softmax_f32(expected, data, 4);
  softmax_f32_inplace(data, 1, 4);

  double max_err = compute_max_error(data, expected, 4);
  ASSERT_LT(max_err, 1e-5);
}

TEST(softmax_f32_large_row) {
  const int size = 512;
  float *input = (float *)malloc(size * sizeof(float));
  float *output = (float *)malloc(size * sizeof(float));
  float *expected = (float *)malloc(size * sizeof(float));

  for (int i = 0; i < size; i++) {
    input[i] = (float)(i % 10) - 5.0f;
  }

  reference_softmax_f32(expected, input, size);
  softmax_f32(output, input, 1, size);

  double max_err = compute_max_error(output, expected, size);
  ASSERT_LT(max_err, 1e-4);

  float sum = 0.0f;
  for (int i = 0; i < size; i++) {
    sum += output[i];
  }
  ASSERT_LT(fabs(sum - 1.0f), 1e-4);

  free(input);
  free(output);
  free(expected);
}

TEST(softmax_f32_attention_dim) {
  const int size = 4096;
  float *input = (float *)malloc(size * sizeof(float));
  float *output = (float *)malloc(size * sizeof(float));

  for (int i = 0; i < size; i++) {
    input[i] = ((float)(i % 100) / 50.0f) - 1.0f;
  }

  softmax_f32(output, input, 1, size);

  float sum = 0.0f;
  for (int i = 0; i < size; i++) {
    sum += output[i];
    ASSERT(output[i] >= 0.0f && output[i] <= 1.0f);
  }
  ASSERT_LT(fabs(sum - 1.0f), 1e-4);

  free(input);
  free(output);
}

extern "C" void run_softmax_tests(void) {
  TEST_SUITE("Softmax (FP32/BF16/FP16)");
  RUN_TEST(softmax_f32_basic);
  RUN_TEST(softmax_f32_sum_to_one);
  RUN_TEST(softmax_f32_all_equal);
  RUN_TEST(softmax_f32_large_values);
  RUN_TEST(softmax_f32_negative_values);
  RUN_TEST(softmax_f32_batch);
  RUN_TEST(softmax_f32_scaled);
  RUN_TEST(softmax_bf16_basic);
  RUN_TEST(softmax_f16_basic);
  RUN_TEST(softmax_f32_inplace);
  RUN_TEST(softmax_f32_large_row);
  RUN_TEST(softmax_f32_attention_dim);
}

/*
 * Softmax - Dispatcher and Scalar Fallback Implementations
 *
 * Numerically stable softmax: softmax(x_i) = exp(x_i - max(x)) / sum(exp(x_j -
 * max(x)))
 */

#include "inference/kernels/softmax/softmax.h"
#include "inference/kernels/softmax/softmax_kernels.h"
#include <math.h>
#include <string.h>

static inline float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_bf16(float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));
  return (uint16_t)(bits >> 16);
}

static inline float fp16_to_float(uint16_t fp16) {
  uint32_t sign = (fp16 & 0x8000) << 16;
  uint32_t exp = (fp16 >> 10) & 0x1F;
  uint32_t mant = fp16 & 0x3FF;
  uint32_t f32_bits;

  if (exp == 0) {
    if (mant == 0) {
      f32_bits = sign;
    } else {
      exp = 1;
      while ((mant & 0x400) == 0) {
        mant <<= 1;
        exp--;
      }
      mant &= 0x3FF;
      f32_bits = sign | ((127 - 15 + exp) << 23) | (mant << 13);
    }
  } else if (exp == 31) {
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    f32_bits = sign | ((exp - 15 + 127) << 23) | (mant << 13);
  }

  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_fp16(float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));

  uint32_t sign = (bits >> 16) & 0x8000;
  int32_t exp = ((bits >> 23) & 0xFF) - 127 + 15;
  uint32_t mant = (bits >> 13) & 0x3FF;

  if (exp <= 0) {
    if (exp < -10) {
      return (uint16_t)sign;
    }
    mant = (mant | 0x400) >> (1 - exp);
    return (uint16_t)(sign | mant);
  } else if (exp >= 31) {
    if (exp == 128 && mant) {
      return (uint16_t)(sign | 0x7C00 | (mant >> 10));
    }
    return (uint16_t)(sign | 0x7C00);
  }

  return (uint16_t)(sign | (exp << 10) | mant);
}

static void softmax_f32_scalar(float *output, const float *input, int num_rows,
                               int row_size, float scale) {
  for (int row = 0; row < num_rows; row++) {
    const float *in_row = input + row * row_size;
    float *out_row = output + row * row_size;

    float max_val = in_row[0] * scale;
    for (int i = 1; i < row_size; i++) {
      float val = in_row[i] * scale;
      if (val > max_val)
        max_val = val;
    }

    float sum = 0.0f;
    for (int i = 0; i < row_size; i++) {
      float val = expf(in_row[i] * scale - max_val);
      out_row[i] = val;
      sum += val;
    }

    float inv_sum = 1.0f / sum;
    for (int i = 0; i < row_size; i++) {
      out_row[i] *= inv_sum;
    }
  }
}

static void softmax_bf16_scalar(uint16_t *output, const uint16_t *input,
                                int num_rows, int row_size, float scale) {
  for (int row = 0; row < num_rows; row++) {
    const uint16_t *in_row = input + row * row_size;
    uint16_t *out_row = output + row * row_size;

    float max_val = bf16_to_float(in_row[0]) * scale;
    for (int i = 1; i < row_size; i++) {
      float val = bf16_to_float(in_row[i]) * scale;
      if (val > max_val)
        max_val = val;
    }

    float sum = 0.0f;
    for (int i = 0; i < row_size; i++) {
      float val = expf(bf16_to_float(in_row[i]) * scale - max_val);
      out_row[i] = float_to_bf16(val);
      sum += val;
    }

    float inv_sum = 1.0f / sum;
    for (int i = 0; i < row_size; i++) {
      float val = bf16_to_float(out_row[i]) * inv_sum;
      out_row[i] = float_to_bf16(val);
    }
  }
}

static void softmax_f16_scalar(uint16_t *output, const uint16_t *input,
                               int num_rows, int row_size, float scale) {
  for (int row = 0; row < num_rows; row++) {
    const uint16_t *in_row = input + row * row_size;
    uint16_t *out_row = output + row * row_size;

    float max_val = fp16_to_float(in_row[0]) * scale;
    for (int i = 1; i < row_size; i++) {
      float val = fp16_to_float(in_row[i]) * scale;
      if (val > max_val)
        max_val = val;
    }

    float sum = 0.0f;
    for (int i = 0; i < row_size; i++) {
      float val = expf(fp16_to_float(in_row[i]) * scale - max_val);
      out_row[i] = float_to_fp16(val);
      sum += val;
    }

    float inv_sum = 1.0f / sum;
    for (int i = 0; i < row_size; i++) {
      float val = fp16_to_float(out_row[i]) * inv_sum;
      out_row[i] = float_to_fp16(val);
    }
  }
}

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define HAS_NEON_IMPL 1
#else
#define HAS_NEON_IMPL 0
#endif

void softmax_f32(float *output, const float *input, int num_rows,
                 int row_size) {
#if HAS_NEON_IMPL
  softmax_caps_t caps = softmax_get_capabilities();
  if (caps.has_neon) {
    softmax_f32_kernel(output, input, num_rows, row_size, 1.0f);
    return;
  }
#endif
  softmax_f32_scalar(output, input, num_rows, row_size, 1.0f);
}

void softmax_bf16(uint16_t *output, const uint16_t *input, int num_rows,
                  int row_size) {
#if HAS_NEON_IMPL
  softmax_caps_t caps = softmax_get_capabilities();
  if (caps.has_neon) {
    softmax_bf16_kernel(output, input, num_rows, row_size, 1.0f);
    return;
  }
#endif
  softmax_bf16_scalar(output, input, num_rows, row_size, 1.0f);
}

void softmax_f16(uint16_t *output, const uint16_t *input, int num_rows,
                 int row_size) {
#if HAS_NEON_IMPL
  softmax_caps_t caps = softmax_get_capabilities();
  if (caps.has_neon) {
    softmax_f16_kernel(output, input, num_rows, row_size, 1.0f);
    return;
  }
#endif
  softmax_f16_scalar(output, input, num_rows, row_size, 1.0f);
}

void softmax_f32_inplace(float *data, int num_rows, int row_size) {
  softmax_f32(data, data, num_rows, row_size);
}

void softmax_bf16_inplace(uint16_t *data, int num_rows, int row_size) {
  softmax_bf16(data, data, num_rows, row_size);
}

void softmax_f16_inplace(uint16_t *data, int num_rows, int row_size) {
  softmax_f16(data, data, num_rows, row_size);
}

void softmax_f32_scaled(float *output, const float *input, int num_rows,
                        int row_size, float scale) {
#if HAS_NEON_IMPL
  softmax_caps_t caps = softmax_get_capabilities();
  if (caps.has_neon) {
    softmax_f32_kernel(output, input, num_rows, row_size, scale);
    return;
  }
#endif
  softmax_f32_scalar(output, input, num_rows, row_size, scale);
}

void softmax_bf16_scaled(uint16_t *output, const uint16_t *input, int num_rows,
                         int row_size, float scale) {
#if HAS_NEON_IMPL
  softmax_caps_t caps = softmax_get_capabilities();
  if (caps.has_neon) {
    softmax_bf16_kernel(output, input, num_rows, row_size, scale);
    return;
  }
#endif
  softmax_bf16_scalar(output, input, num_rows, row_size, scale);
}

void softmax_f16_scaled(uint16_t *output, const uint16_t *input, int num_rows,
                        int row_size, float scale) {
#if HAS_NEON_IMPL
  softmax_caps_t caps = softmax_get_capabilities();
  if (caps.has_neon) {
    softmax_f16_kernel(output, input, num_rows, row_size, scale);
    return;
  }
#endif
  softmax_f16_scalar(output, input, num_rows, row_size, scale);
}

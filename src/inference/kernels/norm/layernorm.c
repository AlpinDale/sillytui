/*
 * Layer Normalization Operations - Reference Implementations
 */

#include "inference/kernels/norm/layernorm.h"
#include "inference/kernels/norm/layernorm_kernels.h"
#include <math.h>
#include <string.h>

/* ============ FP32 Implementation ============ */

void rms_norm_f32(float *out, const float *input, const float *weight,
                  float epsilon, int num_tokens, int hidden_size) {
  if (num_tokens <= 0 || hidden_size <= 0)
    return;

  norm_caps_t caps = norm_get_capabilities();

  if (caps.has_neon) {
    rms_norm_f32_kernel(out, input, weight, epsilon, num_tokens, hidden_size);
    return;
  }

  /* Scalar fallback */
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * hidden_size;
    float *out_row = out + i * hidden_size;

    /* Compute variance = mean(x^2) */
    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      variance += in_row[j] * in_row[j];
    }
    variance /= (float)hidden_size;

    /* Compute normalization factor: 1 / sqrt(variance + epsilon) */
    float scale = 1.0f / sqrtf(variance + epsilon);

    /* Normalize and apply weight */
    for (int j = 0; j < hidden_size; j++) {
      out_row[j] = in_row[j] * scale * weight[j];
    }
  }
}

void fused_add_rms_norm_f32(float *out, const float *input, float *residual,
                            const float *weight, float epsilon, int num_tokens,
                            int hidden_size) {
  if (num_tokens <= 0 || hidden_size <= 0)
    return;

  norm_caps_t caps = norm_get_capabilities();

  if (caps.has_neon) {
    fused_add_rms_norm_f32_kernel(out, input, residual, weight, epsilon,
                                  num_tokens, hidden_size);
    return;
  }

  /* Scalar fallback */
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * hidden_size;
    float *res_row = residual + i * hidden_size;
    float *out_row = out + i * hidden_size;

    /* Fuse: residual += input, compute variance */
    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      res_row[j] += in_row[j];
      variance += res_row[j] * res_row[j];
    }
    variance /= (float)hidden_size;

    /* Compute normalization factor */
    float scale = 1.0f / sqrtf(variance + epsilon);

    /* Normalize residual and apply weight */
    for (int j = 0; j < hidden_size; j++) {
      out_row[j] = res_row[j] * scale * weight[j];
    }
  }
}

/* ============ BF16 Implementation ============ */

static inline uint16_t float_to_bf16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t lsb = (bits >> 16) & 1;
  uint32_t rounding_bias = 0x7fff + lsb;
  bits += rounding_bias;
  return (uint16_t)(bits >> 16);
}

static inline float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

void rms_norm_bf16(uint16_t *out, const uint16_t *input, const uint16_t *weight,
                   float epsilon, int num_tokens, int hidden_size) {
  if (num_tokens <= 0 || hidden_size <= 0)
    return;

  norm_caps_t caps = norm_get_capabilities();

  if (caps.has_neon) {
    rms_norm_bf16_kernel(out, input, weight, epsilon, num_tokens, hidden_size);
    return;
  }

  /* Scalar fallback */
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Compute variance in FP32 */
    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      float x = bf16_to_float(in_row[j]);
      variance += x * x;
    }
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);

    /* Normalize and apply weight */
    for (int j = 0; j < hidden_size; j++) {
      float x = bf16_to_float(in_row[j]);
      float w = bf16_to_float(weight[j]);
      out_row[j] = float_to_bf16(x * scale * w);
    }
  }
}

void fused_add_rms_norm_bf16(uint16_t *out, const uint16_t *input,
                             uint16_t *residual, const uint16_t *weight,
                             float epsilon, int num_tokens, int hidden_size) {
  if (num_tokens <= 0 || hidden_size <= 0)
    return;

  norm_caps_t caps = norm_get_capabilities();

  if (caps.has_neon) {
    fused_add_rms_norm_bf16_kernel(out, input, residual, weight, epsilon,
                                   num_tokens, hidden_size);
    return;
  }

  /* Scalar fallback */
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *res_row = residual + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Fuse: residual += input, compute variance */
    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      float x = bf16_to_float(in_row[j]);
      float r = bf16_to_float(res_row[j]);
      float sum = x + r;
      res_row[j] = float_to_bf16(sum);
      variance += sum * sum;
    }
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);

    /* Normalize residual and apply weight */
    for (int j = 0; j < hidden_size; j++) {
      float r = bf16_to_float(res_row[j]);
      float w = bf16_to_float(weight[j]);
      out_row[j] = float_to_bf16(r * scale * w);
    }
  }
}

/* ============ FP16 Implementation ============ */

static inline uint16_t float_to_fp16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t sign = bits & 0x80000000;
  uint32_t exp = (bits >> 23) & 0xFF;
  uint32_t mant = bits & 0x7FFFFF;

  if (exp == 0xFF) {
    if (mant != 0)
      return 0x7E00;
    return (uint16_t)((sign >> 16) | 0x7C00);
  }
  if (exp == 0 && mant == 0)
    return (uint16_t)(sign >> 16);

  int32_t new_exp = (int32_t)exp - 127 + 15;
  if (new_exp <= 0)
    return (uint16_t)(sign >> 16);
  if (new_exp >= 31)
    return (uint16_t)((sign >> 16) | 0x7C00);

  uint16_t fp16_exp = (uint16_t)(new_exp << 10);
  uint16_t fp16_mant = (uint16_t)(mant >> 13);
  return (uint16_t)((sign >> 16) | fp16_exp | fp16_mant);
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
      int e = -14;
      while ((mant & 0x400) == 0) {
        mant <<= 1;
        e--;
      }
      mant &= 0x3FF;
      f32_bits = sign | (((uint32_t)(e + 127)) << 23) | (mant << 13);
    }
  } else if (exp == 31) {
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    f32_bits = sign | (((uint32_t)(exp - 15 + 127)) << 23) | (mant << 13);
  }

  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

void rms_norm_f16(uint16_t *out, const uint16_t *input, const uint16_t *weight,
                  float epsilon, int num_tokens, int hidden_size) {
  if (num_tokens <= 0 || hidden_size <= 0)
    return;

  norm_caps_t caps = norm_get_capabilities();

  if (caps.has_neon) {
    rms_norm_f16_kernel(out, input, weight, epsilon, num_tokens, hidden_size);
    return;
  }

  /* Scalar fallback */
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Compute variance in FP32 */
    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      float x = fp16_to_float(in_row[j]);
      variance += x * x;
    }
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);

    /* Normalize and apply weight */
    for (int j = 0; j < hidden_size; j++) {
      float x = fp16_to_float(in_row[j]);
      float w = fp16_to_float(weight[j]);
      out_row[j] = float_to_fp16(x * scale * w);
    }
  }
}

void fused_add_rms_norm_f16(uint16_t *out, const uint16_t *input,
                            uint16_t *residual, const uint16_t *weight,
                            float epsilon, int num_tokens, int hidden_size) {
  if (num_tokens <= 0 || hidden_size <= 0)
    return;

  norm_caps_t caps = norm_get_capabilities();

  if (caps.has_neon) {
    fused_add_rms_norm_f16_kernel(out, input, residual, weight, epsilon,
                                  num_tokens, hidden_size);
    return;
  }

  /* Scalar fallback */
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *res_row = residual + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Fuse: residual += input, compute variance */
    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      float x = fp16_to_float(in_row[j]);
      float r = fp16_to_float(res_row[j]);
      float sum = x + r;
      res_row[j] = float_to_fp16(sum);
      variance += sum * sum;
    }
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);

    /* Normalize residual and apply weight */
    for (int j = 0; j < hidden_size; j++) {
      float r = fp16_to_float(res_row[j]);
      float w = fp16_to_float(weight[j]);
      out_row[j] = float_to_fp16(r * scale * w);
    }
  }
}

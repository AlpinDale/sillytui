/*
 * Activation Functions - Reference/Scalar Implementations
 */

#include "inference/kernels/activation/activation.h"
#include "inference/kernels/activation/activation_kernels.h"
#include <math.h>
#include <string.h>

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

/* ============ Scalar Math Helpers ============ */

static inline float scalar_silu(float x) { return x / (1.0f + expf(-x)); }

static inline float scalar_gelu(float x) {
  return x * 0.5f * (1.0f + erff(x * (float)M_SQRT1_2));
}

static inline float scalar_gelu_tanh(float x) {
  const float w1 = (float)(M_SQRT2 * M_2_SQRTPI * 0.5);
  const float w3 = 0.044715f;
  float x3 = x * x * x;
  float inner = w1 * (x + x3 * w3);
  return x * 0.5f * (1.0f + tanhf(inner));
}

static inline float scalar_gelu_quick(float x) {
  return x / (1.0f + expf(-1.702f * x));
}

/* ============ FP16/BF16 Conversion Helpers ============ */

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

/* ============ SiLU Implementation ============ */

void silu_f32(float *out, const float *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    silu_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      out[idx] = scalar_silu(input[idx]);
    }
  }
}

void silu_bf16(uint16_t *out, const uint16_t *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    silu_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = bf16_to_float(input[idx]);
      out[idx] = float_to_bf16(scalar_silu(x));
    }
  }
}

void silu_f16(uint16_t *out, const uint16_t *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    silu_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = fp16_to_float(input[idx]);
      out[idx] = float_to_fp16(scalar_silu(x));
    }
  }
}

/* ============ SiLU and Mul (SwiGLU) Implementation ============ */

void silu_and_mul_f32(float *out, const float *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    silu_and_mul_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = input[in_start + j];
      float gate = input[in_start + d + j];
      out[out_start + j] = scalar_silu(x) * gate;
    }
  }
}

void silu_and_mul_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                       int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    silu_and_mul_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = bf16_to_float(input[in_start + j]);
      float gate = bf16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_bf16(scalar_silu(x) * gate);
    }
  }
}

void silu_and_mul_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    silu_and_mul_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = fp16_to_float(input[in_start + j]);
      float gate = fp16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_fp16(scalar_silu(x) * gate);
    }
  }
}

/* ============ GELU Implementation ============ */

void gelu_f32(float *out, const float *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      out[idx] = scalar_gelu(input[idx]);
    }
  }
}

void gelu_bf16(uint16_t *out, const uint16_t *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = bf16_to_float(input[idx]);
      out[idx] = float_to_bf16(scalar_gelu(x));
    }
  }
}

void gelu_f16(uint16_t *out, const uint16_t *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = fp16_to_float(input[idx]);
      out[idx] = float_to_fp16(scalar_gelu(x));
    }
  }
}

/* ============ GELU and Mul (GeGLU) Implementation ============ */

void gelu_and_mul_f32(float *out, const float *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_and_mul_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = input[in_start + j];
      float gate = input[in_start + d + j];
      out[out_start + j] = scalar_gelu(x) * gate;
    }
  }
}

void gelu_and_mul_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                       int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_and_mul_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = bf16_to_float(input[in_start + j]);
      float gate = bf16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_bf16(scalar_gelu(x) * gate);
    }
  }
}

void gelu_and_mul_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_and_mul_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = fp16_to_float(input[in_start + j]);
      float gate = fp16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_fp16(scalar_gelu(x) * gate);
    }
  }
}

/* ============ GELU Tanh Implementation ============ */

void gelu_tanh_f32(float *out, const float *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_tanh_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      out[idx] = scalar_gelu_tanh(input[idx]);
    }
  }
}

void gelu_tanh_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                    int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_tanh_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = bf16_to_float(input[idx]);
      out[idx] = float_to_bf16(scalar_gelu_tanh(x));
    }
  }
}

void gelu_tanh_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                   int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_tanh_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = fp16_to_float(input[idx]);
      out[idx] = float_to_fp16(scalar_gelu_tanh(x));
    }
  }
}

/* ============ GELU Tanh and Mul Implementation ============ */

void gelu_tanh_and_mul_f32(float *out, const float *input, int num_tokens,
                           int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_tanh_and_mul_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = input[in_start + j];
      float gate = input[in_start + d + j];
      out[out_start + j] = scalar_gelu_tanh(x) * gate;
    }
  }
}

void gelu_tanh_and_mul_bf16(uint16_t *out, const uint16_t *input,
                            int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_tanh_and_mul_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = bf16_to_float(input[in_start + j]);
      float gate = bf16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_bf16(scalar_gelu_tanh(x) * gate);
    }
  }
}

void gelu_tanh_and_mul_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_tanh_and_mul_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = fp16_to_float(input[in_start + j]);
      float gate = fp16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_fp16(scalar_gelu_tanh(x) * gate);
    }
  }
}

/* ============ GELU Quick Implementation ============ */

void gelu_quick_f32(float *out, const float *input, int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_quick_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      out[idx] = scalar_gelu_quick(input[idx]);
    }
  }
}

void gelu_quick_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_quick_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = bf16_to_float(input[idx]);
      out[idx] = float_to_bf16(scalar_gelu_quick(x));
    }
  }
}

void gelu_quick_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                    int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_quick_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    for (int j = 0; j < d; j++) {
      int idx = i * d + j;
      float x = fp16_to_float(input[idx]);
      out[idx] = float_to_fp16(scalar_gelu_quick(x));
    }
  }
}

/* ============ GELU Quick and Mul Implementation ============ */

void gelu_quick_and_mul_f32(float *out, const float *input, int num_tokens,
                            int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_quick_and_mul_f32_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = input[in_start + j];
      float gate = input[in_start + d + j];
      out[out_start + j] = scalar_gelu_quick(x) * gate;
    }
  }
}

void gelu_quick_and_mul_bf16(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_quick_and_mul_bf16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = bf16_to_float(input[in_start + j]);
      float gate = bf16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_bf16(scalar_gelu_quick(x) * gate);
    }
  }
}

void gelu_quick_and_mul_f16(uint16_t *out, const uint16_t *input,
                            int num_tokens, int d) {
  if (num_tokens <= 0 || d <= 0)
    return;

  activation_caps_t caps = activation_get_capabilities();
  if (caps.has_neon) {
    gelu_quick_and_mul_f16_kernel(out, input, num_tokens, d);
    return;
  }

  for (int i = 0; i < num_tokens; i++) {
    int in_start = i * 2 * d;
    int out_start = i * d;
    for (int j = 0; j < d; j++) {
      float x = fp16_to_float(input[in_start + j]);
      float gate = fp16_to_float(input[in_start + d + j]);
      out[out_start + j] = float_to_fp16(scalar_gelu_quick(x) * gate);
    }
  }
}

/*
 * Activation Functions - NEON Optimized Implementations
 *
 * Uses fast polynomial approximations for transcendental functions.
 */

#include "inference/backend/caps.h"
#include "inference/kernels/activation/activation_kernels.h"
#include <math.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

activation_caps_t activation_get_capabilities(void) {
  activation_caps_t caps = {0};
  caps.has_neon = caps_has(CAP_NEON);
  caps.has_avx2 = caps_has(CAP_AVX2);
  caps.has_avx512 = caps_has(CAP_AVX512);
  return caps;
}

#if HAS_NEON

/* Fast exp approximation using polynomial
 * exp(x) ≈ 2^(x/ln2) = 2^(x * 1.4427)
 * Uses range reduction and 5th-order polynomial
 */
static inline float32x4_t fast_exp_f32x4(float32x4_t x) {
  const float32x4_t log2e = vdupq_n_f32(1.442695041f);
  const float32x4_t ln2 = vdupq_n_f32(0.6931471806f);
  const float32x4_t c0 = vdupq_n_f32(1.0f);
  const float32x4_t c1 = vdupq_n_f32(1.0f);
  const float32x4_t c2 = vdupq_n_f32(0.5f);
  const float32x4_t c3 = vdupq_n_f32(0.16666666666f);
  const float32x4_t c4 = vdupq_n_f32(0.04166666666f);
  const float32x4_t c5 = vdupq_n_f32(0.00833333333f);

  float32x4_t clamp_lo = vdupq_n_f32(-88.0f);
  float32x4_t clamp_hi = vdupq_n_f32(88.0f);
  x = vmaxq_f32(x, clamp_lo);
  x = vminq_f32(x, clamp_hi);

  float32x4_t z = vmulq_f32(x, log2e);
  float32x4_t floor_z = vrndmq_f32(z);
  int32x4_t n = vcvtq_s32_f32(floor_z);
  float32x4_t r = vmlsq_f32(x, floor_z, ln2);

  float32x4_t p = c5;
  p = vmlaq_f32(c4, p, r);
  p = vmlaq_f32(c3, p, r);
  p = vmlaq_f32(c2, p, r);
  p = vmlaq_f32(c1, p, r);
  p = vmlaq_f32(c0, p, r);

  n = vaddq_s32(n, vdupq_n_s32(127));
  n = vshlq_n_s32(n, 23);
  float32x4_t scale = vreinterpretq_f32_s32(n);

  return vmulq_f32(p, scale);
}

/* Fast sigmoid: 1 / (1 + exp(-x)) */
static inline float32x4_t fast_sigmoid_f32x4(float32x4_t x) {
  float32x4_t neg_x = vnegq_f32(x);
  float32x4_t exp_neg_x = fast_exp_f32x4(neg_x);
  float32x4_t one = vdupq_n_f32(1.0f);
  float32x4_t denom = vaddq_f32(one, exp_neg_x);
  return vdivq_f32(one, denom);
}

/* SiLU: x * sigmoid(x) = x / (1 + exp(-x)) */
static inline float32x4_t silu_f32x4(float32x4_t x) {
  return vmulq_f32(x, fast_sigmoid_f32x4(x));
}

/* Fast tanh approximation using identity: tanh(x) = 2*sigmoid(2x) - 1 */
static inline float32x4_t fast_tanh_f32x4(float32x4_t x) {
  float32x4_t two = vdupq_n_f32(2.0f);
  float32x4_t one = vdupq_n_f32(1.0f);
  float32x4_t two_x = vmulq_f32(two, x);
  return vsubq_f32(vmulq_f32(two, fast_sigmoid_f32x4(two_x)), one);
}

/* Fast erf approximation using Horner polynomial (Abramowitz & Stegun) */
static inline float32x4_t fast_erf_f32x4(float32x4_t x) {
  const float32x4_t a1 = vdupq_n_f32(0.254829592f);
  const float32x4_t a2 = vdupq_n_f32(-0.284496736f);
  const float32x4_t a3 = vdupq_n_f32(1.421413741f);
  const float32x4_t a4 = vdupq_n_f32(-1.453152027f);
  const float32x4_t a5 = vdupq_n_f32(1.061405429f);
  const float32x4_t p = vdupq_n_f32(0.3275911f);

  uint32x4_t sign_mask = vcltq_f32(x, vdupq_n_f32(0.0f));
  float32x4_t abs_x = vabsq_f32(x);

  float32x4_t t =
      vdivq_f32(vdupq_n_f32(1.0f), vmlaq_f32(vdupq_n_f32(1.0f), p, abs_x));

  float32x4_t t2 = vmulq_f32(t, t);
  float32x4_t t3 = vmulq_f32(t2, t);
  float32x4_t t4 = vmulq_f32(t3, t);
  float32x4_t t5 = vmulq_f32(t4, t);

  float32x4_t poly = vmulq_f32(a1, t);
  poly = vmlaq_f32(poly, a2, t2);
  poly = vmlaq_f32(poly, a3, t3);
  poly = vmlaq_f32(poly, a4, t4);
  poly = vmlaq_f32(poly, a5, t5);

  float32x4_t neg_x2 = vnegq_f32(vmulq_f32(abs_x, abs_x));
  float32x4_t exp_term = fast_exp_f32x4(neg_x2);

  float32x4_t result = vsubq_f32(vdupq_n_f32(1.0f), vmulq_f32(poly, exp_term));

  float32x4_t neg_result = vnegq_f32(result);
  return vbslq_f32(sign_mask, neg_result, result);
}

/* GELU exact: x * 0.5 * (1 + erf(x / sqrt(2))) */
static inline float32x4_t gelu_f32x4(float32x4_t x) {
  const float32x4_t sqrt1_2 = vdupq_n_f32(0.70710678118f);
  const float32x4_t half = vdupq_n_f32(0.5f);
  const float32x4_t one = vdupq_n_f32(1.0f);

  float32x4_t arg = vmulq_f32(x, sqrt1_2);
  float32x4_t erf_val = fast_erf_f32x4(arg);
  float32x4_t inner = vaddq_f32(one, erf_val);
  return vmulq_f32(vmulq_f32(x, half), inner);
}

/* GELU tanh: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3))) */
static inline float32x4_t gelu_tanh_f32x4(float32x4_t x) {
  const float32x4_t sqrt_2_pi = vdupq_n_f32(0.7978845608f);
  const float32x4_t coef = vdupq_n_f32(0.044715f);
  const float32x4_t half = vdupq_n_f32(0.5f);
  const float32x4_t one = vdupq_n_f32(1.0f);

  float32x4_t x3 = vmulq_f32(vmulq_f32(x, x), x);
  float32x4_t inner = vmulq_f32(sqrt_2_pi, vmlaq_f32(x, coef, x3));
  float32x4_t tanh_val = fast_tanh_f32x4(inner);
  return vmulq_f32(vmulq_f32(x, half), vaddq_f32(one, tanh_val));
}

/* GELU quick: x / (1 + exp(-1.702 * x)) */
static inline float32x4_t gelu_quick_f32x4(float32x4_t x) {
  const float32x4_t coef = vdupq_n_f32(1.702f);
  float32x4_t scaled = vmulq_f32(coef, x);
  return vmulq_f32(x, fast_sigmoid_f32x4(scaled));
}

/* ============ BF16/FP16 Conversion Helpers ============ */

static inline float32x4_t bf16x4_to_f32x4(uint16x4_t bf16) {
  uint32x4_t bits = vshll_n_u16(bf16, 16);
  return vreinterpretq_f32_u32(bits);
}

static inline uint16x4_t f32x4_to_bf16x4(float32x4_t f32) {
  uint32x4_t bits = vreinterpretq_u32_f32(f32);
  uint32x4_t lsb = vshrq_n_u32(bits, 16);
  lsb = vandq_u32(lsb, vdupq_n_u32(1));
  uint32x4_t rounding = vaddq_u32(vdupq_n_u32(0x7fff), lsb);
  bits = vaddq_u32(bits, rounding);
  return vshrn_n_u32(bits, 16);
}

static inline float32x4_t fp16x4_to_f32x4(uint16x4_t fp16) {
  return vcvt_f32_f16(vreinterpret_f16_u16(fp16));
}

static inline uint16x4_t f32x4_to_fp16x4(float32x4_t f32) {
  return vreinterpret_u16_f16(vcvt_f16_f32(f32));
}

static inline float scalar_fp16_to_f32(uint16_t fp16) {
  float16x4_t fp16_vec = vreinterpret_f16_u16(vdup_n_u16(fp16));
  float32x4_t f32_vec = vcvt_f32_f16(fp16_vec);
  return vgetq_lane_f32(f32_vec, 0);
}

static inline uint16_t scalar_f32_to_fp16(float f32) {
  float32x4_t f32_vec = vdupq_n_f32(f32);
  float16x4_t fp16_vec = vcvt_f16_f32(f32_vec);
  return vget_lane_u16(vreinterpret_u16_f16(fp16_vec), 0);
}

/* ============ SiLU Kernels ============ */

void silu_f32_kernel(float *out, const float *input, int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(in_row + j);
      float32x4_t y = silu_f32x4(x);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = in_row[j];
      out_row[j] = x / (1.0f + expf(-x));
    }
  }
}

void silu_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      uint16x4_t bf16_in = vld1_u16(in_row + j);
      float32x4_t x = bf16x4_to_f32x4(bf16_in);
      float32x4_t y = silu_f32x4(x);
      uint16x4_t bf16_out = f32x4_to_bf16x4(y);
      vst1_u16(out_row + j, bf16_out);
    }
    for (; j < d; j++) {
      uint32_t bits = ((uint32_t)in_row[j]) << 16;
      float x;
      memcpy(&x, &bits, 4);
      float y = x / (1.0f + expf(-x));
      memcpy(&bits, &y, 4);
      uint32_t lsb = (bits >> 16) & 1;
      bits += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits >> 16);
    }
  }
}

void silu_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      uint16x4_t fp16_in = vld1_u16(in_row + j);
      float32x4_t x = fp16x4_to_f32x4(fp16_in);
      float32x4_t y = silu_f32x4(x);
      uint16x4_t fp16_out = f32x4_to_fp16x4(y);
      vst1_u16(out_row + j, fp16_out);
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(in_row[j]);
      float y = x / (1.0f + expf(-x));
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

/* ============ SiLU and Mul (SwiGLU) Kernels ============ */

void silu_and_mul_f32_kernel(float *out, const float *input, int num_tokens,
                             int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *x_row = input + i * 2 * d;
    const float *gate_row = x_row + d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(x_row + j);
      float32x4_t gate = vld1q_f32(gate_row + j);
      float32x4_t y = vmulq_f32(silu_f32x4(x), gate);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = x_row[j];
      float gate = gate_row[j];
      out_row[j] = (x / (1.0f + expf(-x))) * gate;
    }
  }
}

void silu_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                              int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = bf16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = bf16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(silu_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_bf16x4(y));
    }
    for (; j < d; j++) {
      uint32_t bits_x = ((uint32_t)x_row[j]) << 16;
      uint32_t bits_g = ((uint32_t)gate_row[j]) << 16;
      float x, gate;
      memcpy(&x, &bits_x, 4);
      memcpy(&gate, &bits_g, 4);
      float y = (x / (1.0f + expf(-x))) * gate;
      uint32_t bits_y;
      memcpy(&bits_y, &y, 4);
      uint32_t lsb = (bits_y >> 16) & 1;
      bits_y += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits_y >> 16);
    }
  }
}

void silu_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = fp16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = fp16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(silu_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_fp16x4(y));
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(x_row[j]);
      float gate = scalar_fp16_to_f32(gate_row[j]);
      float y = (x / (1.0f + expf(-x))) * gate;
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

/* ============ GELU Kernels ============ */

void gelu_f32_kernel(float *out, const float *input, int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(in_row + j);
      float32x4_t y = gelu_f32x4(x);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = in_row[j];
      out_row[j] = x * 0.5f * (1.0f + erff(x * 0.70710678118f));
    }
  }
}

void gelu_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = bf16x4_to_f32x4(vld1_u16(in_row + j));
      float32x4_t y = gelu_f32x4(x);
      vst1_u16(out_row + j, f32x4_to_bf16x4(y));
    }
    for (; j < d; j++) {
      uint32_t bits = ((uint32_t)in_row[j]) << 16;
      float x;
      memcpy(&x, &bits, 4);
      float y = x * 0.5f * (1.0f + erff(x * 0.70710678118f));
      memcpy(&bits, &y, 4);
      uint32_t lsb = (bits >> 16) & 1;
      bits += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits >> 16);
    }
  }
}

void gelu_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = fp16x4_to_f32x4(vld1_u16(in_row + j));
      float32x4_t y = gelu_f32x4(x);
      vst1_u16(out_row + j, f32x4_to_fp16x4(y));
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(in_row[j]);
      float y = x * 0.5f * (1.0f + erff(x * 0.70710678118f));
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

/* ============ GELU and Mul (GeGLU) Kernels ============ */

void gelu_and_mul_f32_kernel(float *out, const float *input, int num_tokens,
                             int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *x_row = input + i * 2 * d;
    const float *gate_row = x_row + d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(x_row + j);
      float32x4_t gate = vld1q_f32(gate_row + j);
      float32x4_t y = vmulq_f32(gelu_f32x4(x), gate);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = x_row[j];
      float gate = gate_row[j];
      out_row[j] = (x * 0.5f * (1.0f + erff(x * 0.70710678118f))) * gate;
    }
  }
}

void gelu_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                              int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = bf16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = bf16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(gelu_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_bf16x4(y));
    }
    for (; j < d; j++) {
      uint32_t bits_x = ((uint32_t)x_row[j]) << 16;
      uint32_t bits_g = ((uint32_t)gate_row[j]) << 16;
      float x, gate;
      memcpy(&x, &bits_x, 4);
      memcpy(&gate, &bits_g, 4);
      float y = (x * 0.5f * (1.0f + erff(x * 0.70710678118f))) * gate;
      uint32_t bits_y;
      memcpy(&bits_y, &y, 4);
      uint32_t lsb = (bits_y >> 16) & 1;
      bits_y += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits_y >> 16);
    }
  }
}

void gelu_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = fp16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = fp16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(gelu_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_fp16x4(y));
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(x_row[j]);
      float gate = scalar_fp16_to_f32(gate_row[j]);
      float y = (x * 0.5f * (1.0f + erff(x * 0.70710678118f))) * gate;
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

/* ============ GELU Tanh Kernels ============ */

void gelu_tanh_f32_kernel(float *out, const float *input, int num_tokens,
                          int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(in_row + j);
      float32x4_t y = gelu_tanh_f32x4(x);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = in_row[j];
      float x3 = x * x * x;
      float inner = 0.7978845608f * (x + 0.044715f * x3);
      out_row[j] = x * 0.5f * (1.0f + tanhf(inner));
    }
  }
}

void gelu_tanh_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = bf16x4_to_f32x4(vld1_u16(in_row + j));
      float32x4_t y = gelu_tanh_f32x4(x);
      vst1_u16(out_row + j, f32x4_to_bf16x4(y));
    }
    for (; j < d; j++) {
      uint32_t bits = ((uint32_t)in_row[j]) << 16;
      float x;
      memcpy(&x, &bits, 4);
      float x3 = x * x * x;
      float inner = 0.7978845608f * (x + 0.044715f * x3);
      float y = x * 0.5f * (1.0f + tanhf(inner));
      memcpy(&bits, &y, 4);
      uint32_t lsb = (bits >> 16) & 1;
      bits += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits >> 16);
    }
  }
}

void gelu_tanh_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                          int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = fp16x4_to_f32x4(vld1_u16(in_row + j));
      float32x4_t y = gelu_tanh_f32x4(x);
      vst1_u16(out_row + j, f32x4_to_fp16x4(y));
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(in_row[j]);
      float x3 = x * x * x;
      float inner = 0.7978845608f * (x + 0.044715f * x3);
      float y = x * 0.5f * (1.0f + tanhf(inner));
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

/* ============ GELU Tanh and Mul Kernels ============ */

void gelu_tanh_and_mul_f32_kernel(float *out, const float *input,
                                  int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *x_row = input + i * 2 * d;
    const float *gate_row = x_row + d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(x_row + j);
      float32x4_t gate = vld1q_f32(gate_row + j);
      float32x4_t y = vmulq_f32(gelu_tanh_f32x4(x), gate);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = x_row[j];
      float gate = gate_row[j];
      float x3 = x * x * x;
      float inner = 0.7978845608f * (x + 0.044715f * x3);
      out_row[j] = (x * 0.5f * (1.0f + tanhf(inner))) * gate;
    }
  }
}

void gelu_tanh_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                                   int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = bf16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = bf16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(gelu_tanh_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_bf16x4(y));
    }
    for (; j < d; j++) {
      uint32_t bits_x = ((uint32_t)x_row[j]) << 16;
      uint32_t bits_g = ((uint32_t)gate_row[j]) << 16;
      float x, gate;
      memcpy(&x, &bits_x, 4);
      memcpy(&gate, &bits_g, 4);
      float x3 = x * x * x;
      float inner = 0.7978845608f * (x + 0.044715f * x3);
      float y = (x * 0.5f * (1.0f + tanhf(inner))) * gate;
      uint32_t bits_y;
      memcpy(&bits_y, &y, 4);
      uint32_t lsb = (bits_y >> 16) & 1;
      bits_y += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits_y >> 16);
    }
  }
}

void gelu_tanh_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                                  int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = fp16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = fp16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(gelu_tanh_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_fp16x4(y));
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(x_row[j]);
      float gate = scalar_fp16_to_f32(gate_row[j]);
      float x3 = x * x * x;
      float inner = 0.7978845608f * (x + 0.044715f * x3);
      float y = (x * 0.5f * (1.0f + tanhf(inner))) * gate;
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

/* ============ GELU Quick Kernels ============ */

void gelu_quick_f32_kernel(float *out, const float *input, int num_tokens,
                           int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(in_row + j);
      float32x4_t y = gelu_quick_f32x4(x);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = in_row[j];
      out_row[j] = x / (1.0f + expf(-1.702f * x));
    }
  }
}

void gelu_quick_bf16_kernel(uint16_t *out, const uint16_t *input,
                            int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = bf16x4_to_f32x4(vld1_u16(in_row + j));
      float32x4_t y = gelu_quick_f32x4(x);
      vst1_u16(out_row + j, f32x4_to_bf16x4(y));
    }
    for (; j < d; j++) {
      uint32_t bits = ((uint32_t)in_row[j]) << 16;
      float x;
      memcpy(&x, &bits, 4);
      float y = x / (1.0f + expf(-1.702f * x));
      memcpy(&bits, &y, 4);
      uint32_t lsb = (bits >> 16) & 1;
      bits += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits >> 16);
    }
  }
}

void gelu_quick_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = fp16x4_to_f32x4(vld1_u16(in_row + j));
      float32x4_t y = gelu_quick_f32x4(x);
      vst1_u16(out_row + j, f32x4_to_fp16x4(y));
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(in_row[j]);
      float y = x / (1.0f + expf(-1.702f * x));
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

/* ============ GELU Quick and Mul Kernels ============ */

void gelu_quick_and_mul_f32_kernel(float *out, const float *input,
                                   int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const float *x_row = input + i * 2 * d;
    const float *gate_row = x_row + d;
    float *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = vld1q_f32(x_row + j);
      float32x4_t gate = vld1q_f32(gate_row + j);
      float32x4_t y = vmulq_f32(gelu_quick_f32x4(x), gate);
      vst1q_f32(out_row + j, y);
    }
    for (; j < d; j++) {
      float x = x_row[j];
      float gate = gate_row[j];
      out_row[j] = (x / (1.0f + expf(-1.702f * x))) * gate;
    }
  }
}

void gelu_quick_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                                    int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = bf16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = bf16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(gelu_quick_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_bf16x4(y));
    }
    for (; j < d; j++) {
      uint32_t bits_x = ((uint32_t)x_row[j]) << 16;
      uint32_t bits_g = ((uint32_t)gate_row[j]) << 16;
      float x, gate;
      memcpy(&x, &bits_x, 4);
      memcpy(&gate, &bits_g, 4);
      float y = (x / (1.0f + expf(-1.702f * x))) * gate;
      uint32_t bits_y;
      memcpy(&bits_y, &y, 4);
      uint32_t lsb = (bits_y >> 16) & 1;
      bits_y += 0x7fff + lsb;
      out_row[j] = (uint16_t)(bits_y >> 16);
    }
  }
}

void gelu_quick_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                                   int num_tokens, int d) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *x_row = input + i * 2 * d;
    const uint16_t *gate_row = x_row + d;
    uint16_t *out_row = out + i * d;

    int j = 0;
    for (; j <= d - 4; j += 4) {
      float32x4_t x = fp16x4_to_f32x4(vld1_u16(x_row + j));
      float32x4_t gate = fp16x4_to_f32x4(vld1_u16(gate_row + j));
      float32x4_t y = vmulq_f32(gelu_quick_f32x4(x), gate);
      vst1_u16(out_row + j, f32x4_to_fp16x4(y));
    }
    for (; j < d; j++) {
      float x = scalar_fp16_to_f32(x_row[j]);
      float gate = scalar_fp16_to_f32(gate_row[j]);
      float y = (x / (1.0f + expf(-1.702f * x))) * gate;
      out_row[j] = scalar_f32_to_fp16(y);
    }
  }
}

#else /* !HAS_NEON - stubs */

void silu_f32_kernel(float *out, const float *input, int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void silu_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void silu_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void silu_and_mul_f32_kernel(float *out, const float *input, int num_tokens,
                             int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void silu_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                              int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void silu_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_f32_kernel(float *out, const float *input, int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_and_mul_f32_kernel(float *out, const float *input, int num_tokens,
                             int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                              int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_tanh_f32_kernel(float *out, const float *input, int num_tokens,
                          int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_tanh_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_tanh_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                          int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_tanh_and_mul_f32_kernel(float *out, const float *input,
                                  int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_tanh_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                                   int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_tanh_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                                  int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_quick_f32_kernel(float *out, const float *input, int num_tokens,
                           int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_quick_bf16_kernel(uint16_t *out, const uint16_t *input,
                            int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_quick_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_quick_and_mul_f32_kernel(float *out, const float *input,
                                   int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_quick_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                                    int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}
void gelu_quick_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                                   int num_tokens, int d) {
  (void)out;
  (void)input;
  (void)num_tokens;
  (void)d;
}

#endif /* HAS_NEON */

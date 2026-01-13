/*
 * ARM NEON optimized layernorm kernels
 */

#include "inference/backend/caps.h"
#include "inference/kernels/norm/layernorm_kernels.h"
#include <math.h>
#include <string.h>

norm_caps_t norm_get_capabilities(void) {
  norm_caps_t caps = {0};
  caps.has_neon = caps_has(CAP_NEON);
  return caps;
}

#if defined(__ARM_NEON) || defined(__aarch64__)

#include <arm_neon.h>

/* ============ FP32 NEON Kernels ============ */

void rms_norm_f32_kernel(float *out, const float *input, const float *weight,
                         float epsilon, int num_tokens, int hidden_size) {
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * hidden_size;
    float *out_row = out + i * hidden_size;

    /* Compute variance = mean(x^2) using NEON */
    float32x4_t var_acc0 = vdupq_n_f32(0.0f);
    float32x4_t var_acc1 = vdupq_n_f32(0.0f);

    int j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float32x4_t x0 = vld1q_f32(in_row + j);
      float32x4_t x1 = vld1q_f32(in_row + j + 4);
      var_acc0 = vmlaq_f32(var_acc0, x0, x0);
      var_acc1 = vmlaq_f32(var_acc1, x1, x1);
    }

    /* Handle tail */
    float variance_tail = 0.0f;
    for (; j < hidden_size; j++) {
      float x = in_row[j];
      variance_tail += x * x;
    }

    /* Reduce accumulators */
    float32x4_t var_sum = vaddq_f32(var_acc0, var_acc1);
    float variance = vaddvq_f32(var_sum) + variance_tail;
    variance /= (float)hidden_size;

    /* Compute scale = 1 / sqrt(variance + epsilon) */
    float scale = 1.0f / sqrtf(variance + epsilon);
    float32x4_t scale_vec = vdupq_n_f32(scale);

    /* Normalize: out = input * scale * weight */
    j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float32x4_t x0 = vld1q_f32(in_row + j);
      float32x4_t x1 = vld1q_f32(in_row + j + 4);
      float32x4_t w0 = vld1q_f32(weight + j);
      float32x4_t w1 = vld1q_f32(weight + j + 4);

      float32x4_t out0 = vmulq_f32(vmulq_f32(x0, scale_vec), w0);
      float32x4_t out1 = vmulq_f32(vmulq_f32(x1, scale_vec), w1);

      vst1q_f32(out_row + j, out0);
      vst1q_f32(out_row + j + 4, out1);
    }

    /* Handle tail */
    for (; j < hidden_size; j++) {
      out_row[j] = in_row[j] * scale * weight[j];
    }
  }
}

void fused_add_rms_norm_f32_kernel(float *out, const float *input,
                                   float *residual, const float *weight,
                                   float epsilon, int num_tokens,
                                   int hidden_size) {
  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * hidden_size;
    float *res_row = residual + i * hidden_size;
    float *out_row = out + i * hidden_size;

    /* Fuse: residual += input, compute variance */
    float32x4_t var_acc0 = vdupq_n_f32(0.0f);
    float32x4_t var_acc1 = vdupq_n_f32(0.0f);

    int j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float32x4_t x0 = vld1q_f32(in_row + j);
      float32x4_t x1 = vld1q_f32(in_row + j + 4);
      float32x4_t r0 = vld1q_f32(res_row + j);
      float32x4_t r1 = vld1q_f32(res_row + j + 4);

      float32x4_t sum0 = vaddq_f32(x0, r0);
      float32x4_t sum1 = vaddq_f32(x1, r1);

      vst1q_f32(res_row + j, sum0);
      vst1q_f32(res_row + j + 4, sum1);

      var_acc0 = vmlaq_f32(var_acc0, sum0, sum0);
      var_acc1 = vmlaq_f32(var_acc1, sum1, sum1);
    }

    /* Handle tail */
    float variance_tail = 0.0f;
    for (; j < hidden_size; j++) {
      float sum = in_row[j] + res_row[j];
      res_row[j] = sum;
      variance_tail += sum * sum;
    }

    /* Reduce and compute scale */
    float32x4_t var_sum = vaddq_f32(var_acc0, var_acc1);
    float variance = vaddvq_f32(var_sum) + variance_tail;
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);
    float32x4_t scale_vec = vdupq_n_f32(scale);

    /* Normalize residual */
    j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float32x4_t r0 = vld1q_f32(res_row + j);
      float32x4_t r1 = vld1q_f32(res_row + j + 4);
      float32x4_t w0 = vld1q_f32(weight + j);
      float32x4_t w1 = vld1q_f32(weight + j + 4);

      float32x4_t out0 = vmulq_f32(vmulq_f32(r0, scale_vec), w0);
      float32x4_t out1 = vmulq_f32(vmulq_f32(r1, scale_vec), w1);

      vst1q_f32(out_row + j, out0);
      vst1q_f32(out_row + j + 4, out1);
    }

    /* Handle tail */
    for (; j < hidden_size; j++) {
      out_row[j] = res_row[j] * scale * weight[j];
    }
  }
}

/* ============ BF16 NEON Kernels ============ */

static inline float32x4_t bf16_to_f32_neon(uint16x4_t bf16) {
  uint32x4_t u32 = vshll_n_u16(bf16, 16);
  return vreinterpretq_f32_u32(u32);
}

static inline uint16x4_t f32_to_bf16_neon(float32x4_t f32) {
  uint32x4_t u32 = vreinterpretq_u32_f32(f32);
  uint32x4_t rounding =
      vshrq_n_u32(vandq_u32(u32, vdupq_n_u32(0x00010000)), 16);
  u32 = vaddq_u32(u32, vdupq_n_u32(0x7FFF));
  u32 = vaddq_u32(u32, rounding);
  return vmovn_u32(vshrq_n_u32(u32, 16));
}

void rms_norm_bf16_kernel(uint16_t *out, const uint16_t *input,
                          const uint16_t *weight, float epsilon, int num_tokens,
                          int hidden_size) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Compute variance in FP32 */
    float32x4_t var_acc0 = vdupq_n_f32(0.0f);
    float32x4_t var_acc1 = vdupq_n_f32(0.0f);

    int j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      uint16x8_t bf16_vec = vld1q_u16(in_row + j);
      uint16x4_t bf16_lo = vget_low_u16(bf16_vec);
      uint16x4_t bf16_hi = vget_high_u16(bf16_vec);

      float32x4_t x0 = bf16_to_f32_neon(bf16_lo);
      float32x4_t x1 = bf16_to_f32_neon(bf16_hi);

      var_acc0 = vmlaq_f32(var_acc0, x0, x0);
      var_acc1 = vmlaq_f32(var_acc1, x1, x1);
    }

    /* Tail handling */
    float variance_tail = 0.0f;
    for (; j < hidden_size; j++) {
      uint32_t bits = ((uint32_t)in_row[j]) << 16;
      float x;
      memcpy(&x, &bits, 4);
      variance_tail += x * x;
    }

    float32x4_t var_sum = vaddq_f32(var_acc0, var_acc1);
    float variance = vaddvq_f32(var_sum) + variance_tail;
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);
    float32x4_t scale_vec = vdupq_n_f32(scale);

    /* Normalize */
    j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      uint16x8_t in_bf16 = vld1q_u16(in_row + j);
      uint16x8_t w_bf16 = vld1q_u16(weight + j);

      float32x4_t x0 = bf16_to_f32_neon(vget_low_u16(in_bf16));
      float32x4_t x1 = bf16_to_f32_neon(vget_high_u16(in_bf16));
      float32x4_t w0 = bf16_to_f32_neon(vget_low_u16(w_bf16));
      float32x4_t w1 = bf16_to_f32_neon(vget_high_u16(w_bf16));

      float32x4_t out0 = vmulq_f32(vmulq_f32(x0, scale_vec), w0);
      float32x4_t out1 = vmulq_f32(vmulq_f32(x1, scale_vec), w1);

      uint16x4_t out_bf16_lo = f32_to_bf16_neon(out0);
      uint16x4_t out_bf16_hi = f32_to_bf16_neon(out1);
      uint16x8_t out_bf16 = vcombine_u16(out_bf16_lo, out_bf16_hi);

      vst1q_u16(out_row + j, out_bf16);
    }

    /* Tail */
    for (; j < hidden_size; j++) {
      uint32_t x_bits = ((uint32_t)in_row[j]) << 16;
      uint32_t w_bits = ((uint32_t)weight[j]) << 16;
      float x, w;
      memcpy(&x, &x_bits, 4);
      memcpy(&w, &w_bits, 4);
      float result = x * scale * w;
      uint32_t r_bits;
      memcpy(&r_bits, &result, 4);
      out_row[j] = (uint16_t)(r_bits >> 16);
    }
  }
}

void fused_add_rms_norm_bf16_kernel(uint16_t *out, const uint16_t *input,
                                    uint16_t *residual, const uint16_t *weight,
                                    float epsilon, int num_tokens,
                                    int hidden_size) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *res_row = residual + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Fuse add and variance computation */
    float32x4_t var_acc0 = vdupq_n_f32(0.0f);
    float32x4_t var_acc1 = vdupq_n_f32(0.0f);

    int j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      uint16x8_t x_bf16 = vld1q_u16(in_row + j);
      uint16x8_t r_bf16 = vld1q_u16(res_row + j);

      float32x4_t x0 = bf16_to_f32_neon(vget_low_u16(x_bf16));
      float32x4_t x1 = bf16_to_f32_neon(vget_high_u16(x_bf16));
      float32x4_t r0 = bf16_to_f32_neon(vget_low_u16(r_bf16));
      float32x4_t r1 = bf16_to_f32_neon(vget_high_u16(r_bf16));

      float32x4_t sum0 = vaddq_f32(x0, r0);
      float32x4_t sum1 = vaddq_f32(x1, r1);

      /* Store back to residual */
      uint16x4_t sum_bf16_lo = f32_to_bf16_neon(sum0);
      uint16x4_t sum_bf16_hi = f32_to_bf16_neon(sum1);
      vst1q_u16(res_row + j, vcombine_u16(sum_bf16_lo, sum_bf16_hi));

      var_acc0 = vmlaq_f32(var_acc0, sum0, sum0);
      var_acc1 = vmlaq_f32(var_acc1, sum1, sum1);
    }

    /* Tail */
    float variance_tail = 0.0f;
    for (; j < hidden_size; j++) {
      uint32_t x_bits = ((uint32_t)in_row[j]) << 16;
      uint32_t r_bits = ((uint32_t)res_row[j]) << 16;
      float x, r;
      memcpy(&x, &x_bits, 4);
      memcpy(&r, &r_bits, 4);
      float sum = x + r;
      uint32_t sum_bits;
      memcpy(&sum_bits, &sum, 4);
      res_row[j] = (uint16_t)(sum_bits >> 16);
      variance_tail += sum * sum;
    }

    float32x4_t var_sum = vaddq_f32(var_acc0, var_acc1);
    float variance = vaddvq_f32(var_sum) + variance_tail;
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);
    float32x4_t scale_vec = vdupq_n_f32(scale);

    /* Normalize residual */
    j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      uint16x8_t r_bf16 = vld1q_u16(res_row + j);
      uint16x8_t w_bf16 = vld1q_u16(weight + j);

      float32x4_t r0 = bf16_to_f32_neon(vget_low_u16(r_bf16));
      float32x4_t r1 = bf16_to_f32_neon(vget_high_u16(r_bf16));
      float32x4_t w0 = bf16_to_f32_neon(vget_low_u16(w_bf16));
      float32x4_t w1 = bf16_to_f32_neon(vget_high_u16(w_bf16));

      float32x4_t out0 = vmulq_f32(vmulq_f32(r0, scale_vec), w0);
      float32x4_t out1 = vmulq_f32(vmulq_f32(r1, scale_vec), w1);

      uint16x4_t out_bf16_lo = f32_to_bf16_neon(out0);
      uint16x4_t out_bf16_hi = f32_to_bf16_neon(out1);
      vst1q_u16(out_row + j, vcombine_u16(out_bf16_lo, out_bf16_hi));
    }

    /* Tail */
    for (; j < hidden_size; j++) {
      uint32_t r_bits = ((uint32_t)res_row[j]) << 16;
      uint32_t w_bits = ((uint32_t)weight[j]) << 16;
      float r, w;
      memcpy(&r, &r_bits, 4);
      memcpy(&w, &w_bits, 4);
      float result = r * scale * w;
      uint32_t result_bits;
      memcpy(&result_bits, &result, 4);
      out_row[j] = (uint16_t)(result_bits >> 16);
    }
  }
}

/* ============ FP16 NEON Kernels ============ */

void rms_norm_f16_kernel(uint16_t *out, const uint16_t *input,
                         const uint16_t *weight, float epsilon, int num_tokens,
                         int hidden_size) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Compute variance in FP32 */
    float32x4_t var_acc0 = vdupq_n_f32(0.0f);
    float32x4_t var_acc1 = vdupq_n_f32(0.0f);

    int j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float16x8_t x_f16 = vreinterpretq_f16_u16(vld1q_u16(in_row + j));
      float32x4_t x0 = vcvt_f32_f16(vget_low_f16(x_f16));
      float32x4_t x1 = vcvt_f32_f16(vget_high_f16(x_f16));

      var_acc0 = vmlaq_f32(var_acc0, x0, x0);
      var_acc1 = vmlaq_f32(var_acc1, x1, x1);
    }

    /* Tail */
    float variance_tail = 0.0f;
    for (; j < hidden_size; j++) {
      float16_t f16;
      memcpy(&f16, &in_row[j], 2);
      float x = (float)f16;
      variance_tail += x * x;
    }

    float32x4_t var_sum = vaddq_f32(var_acc0, var_acc1);
    float variance = vaddvq_f32(var_sum) + variance_tail;
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);
    float32x4_t scale_vec = vdupq_n_f32(scale);

    /* Normalize */
    j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float16x8_t x_f16 = vreinterpretq_f16_u16(vld1q_u16(in_row + j));
      float16x8_t w_f16 = vreinterpretq_f16_u16(vld1q_u16(weight + j));

      float32x4_t x0 = vcvt_f32_f16(vget_low_f16(x_f16));
      float32x4_t x1 = vcvt_f32_f16(vget_high_f16(x_f16));
      float32x4_t w0 = vcvt_f32_f16(vget_low_f16(w_f16));
      float32x4_t w1 = vcvt_f32_f16(vget_high_f16(w_f16));

      float32x4_t out0 = vmulq_f32(vmulq_f32(x0, scale_vec), w0);
      float32x4_t out1 = vmulq_f32(vmulq_f32(x1, scale_vec), w1);

      float16x4_t out_f16_lo = vcvt_f16_f32(out0);
      float16x4_t out_f16_hi = vcvt_f16_f32(out1);
      float16x8_t out_f16 = vcombine_f16(out_f16_lo, out_f16_hi);

      vst1q_u16(out_row + j, vreinterpretq_u16_f16(out_f16));
    }

    /* Tail */
    for (; j < hidden_size; j++) {
      float16_t x_f16, w_f16;
      memcpy(&x_f16, &in_row[j], 2);
      memcpy(&w_f16, &weight[j], 2);
      float result = (float)x_f16 * scale * (float)w_f16;
      float16_t out_f16 = (float16_t)result;
      memcpy(&out_row[j], &out_f16, 2);
    }
  }
}

void fused_add_rms_norm_f16_kernel(uint16_t *out, const uint16_t *input,
                                   uint16_t *residual, const uint16_t *weight,
                                   float epsilon, int num_tokens,
                                   int hidden_size) {
  for (int i = 0; i < num_tokens; i++) {
    const uint16_t *in_row = input + i * hidden_size;
    uint16_t *res_row = residual + i * hidden_size;
    uint16_t *out_row = out + i * hidden_size;

    /* Fuse add and variance */
    float32x4_t var_acc0 = vdupq_n_f32(0.0f);
    float32x4_t var_acc1 = vdupq_n_f32(0.0f);

    int j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float16x8_t x_f16 = vreinterpretq_f16_u16(vld1q_u16(in_row + j));
      float16x8_t r_f16 = vreinterpretq_f16_u16(vld1q_u16(res_row + j));

      float32x4_t x0 = vcvt_f32_f16(vget_low_f16(x_f16));
      float32x4_t x1 = vcvt_f32_f16(vget_high_f16(x_f16));
      float32x4_t r0 = vcvt_f32_f16(vget_low_f16(r_f16));
      float32x4_t r1 = vcvt_f32_f16(vget_high_f16(r_f16));

      float32x4_t sum0 = vaddq_f32(x0, r0);
      float32x4_t sum1 = vaddq_f32(x1, r1);

      /* Store back to residual */
      float16x4_t sum_f16_lo = vcvt_f16_f32(sum0);
      float16x4_t sum_f16_hi = vcvt_f16_f32(sum1);
      vst1q_u16(res_row + j,
                vreinterpretq_u16_f16(vcombine_f16(sum_f16_lo, sum_f16_hi)));

      var_acc0 = vmlaq_f32(var_acc0, sum0, sum0);
      var_acc1 = vmlaq_f32(var_acc1, sum1, sum1);
    }

    /* Tail */
    float variance_tail = 0.0f;
    for (; j < hidden_size; j++) {
      float16_t x_f16, r_f16;
      memcpy(&x_f16, &in_row[j], 2);
      memcpy(&r_f16, &res_row[j], 2);
      float sum = (float)x_f16 + (float)r_f16;
      float16_t sum_f16 = (float16_t)sum;
      memcpy(&res_row[j], &sum_f16, 2);
      variance_tail += sum * sum;
    }

    float32x4_t var_sum = vaddq_f32(var_acc0, var_acc1);
    float variance = vaddvq_f32(var_sum) + variance_tail;
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + epsilon);
    float32x4_t scale_vec = vdupq_n_f32(scale);

    /* Normalize residual */
    j = 0;
    for (; j + 8 <= hidden_size; j += 8) {
      float16x8_t r_f16 = vreinterpretq_f16_u16(vld1q_u16(res_row + j));
      float16x8_t w_f16 = vreinterpretq_f16_u16(vld1q_u16(weight + j));

      float32x4_t r0 = vcvt_f32_f16(vget_low_f16(r_f16));
      float32x4_t r1 = vcvt_f32_f16(vget_high_f16(r_f16));
      float32x4_t w0 = vcvt_f32_f16(vget_low_f16(w_f16));
      float32x4_t w1 = vcvt_f32_f16(vget_high_f16(w_f16));

      float32x4_t out0 = vmulq_f32(vmulq_f32(r0, scale_vec), w0);
      float32x4_t out1 = vmulq_f32(vmulq_f32(r1, scale_vec), w1);

      float16x4_t out_f16_lo = vcvt_f16_f32(out0);
      float16x4_t out_f16_hi = vcvt_f16_f32(out1);
      vst1q_u16(out_row + j,
                vreinterpretq_u16_f16(vcombine_f16(out_f16_lo, out_f16_hi)));
    }

    /* Tail */
    for (; j < hidden_size; j++) {
      float16_t r_f16, w_f16;
      memcpy(&r_f16, &res_row[j], 2);
      memcpy(&w_f16, &weight[j], 2);
      float result = (float)r_f16 * scale * (float)w_f16;
      float16_t out_f16 = (float16_t)result;
      memcpy(&out_row[j], &out_f16, 2);
    }
  }
}

#else
/* Stub implementations for non-NEON platforms */
void rms_norm_f32_kernel(float *out, const float *input, const float *weight,
                         float epsilon, int num_tokens, int hidden_size) {}
void fused_add_rms_norm_f32_kernel(float *out, const float *input,
                                   float *residual, const float *weight,
                                   float epsilon, int num_tokens,
                                   int hidden_size) {}
void rms_norm_bf16_kernel(uint16_t *out, const uint16_t *input,
                          const uint16_t *weight, float epsilon, int num_tokens,
                          int hidden_size) {}
void fused_add_rms_norm_bf16_kernel(uint16_t *out, const uint16_t *input,
                                    uint16_t *residual, const uint16_t *weight,
                                    float epsilon, int num_tokens,
                                    int hidden_size) {}
void rms_norm_f16_kernel(uint16_t *out, const uint16_t *input,
                         const uint16_t *weight, float epsilon, int num_tokens,
                         int hidden_size) {}
void fused_add_rms_norm_f16_kernel(uint16_t *out, const uint16_t *input,
                                   uint16_t *residual, const uint16_t *weight,
                                   float epsilon, int num_tokens,
                                   int hidden_size) {}
#endif

#include "attention_layer.h"
#include "inference/kernels/attention/attention.h"
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/kv_cache/kv_cache.h"
#include "inference/kernels/norm/layernorm.h"
#include "inference/kernels/rope/rope.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

#if defined(__APPLE__)
#include <Accelerate/Accelerate.h>
#define HAS_ACCELERATE 1
#else
#define HAS_ACCELERATE 0
#endif

void qwen3_attention_layer_f32(
    float *output, const float *input, const float *q_proj, const float *k_proj,
    const float *v_proj, const float *o_proj, const float *q_norm,
    const float *k_norm, float *key_cache, float *value_cache,
    const int64_t *position_ids, const float *cos_sin_cache, int seq_len,
    int cache_len, int hidden_size, int num_heads, int num_kv_heads,
    int head_dim, float rope_theta, int max_position) {
  (void)rope_theta;
  (void)max_position;
  int q_dim = num_heads * head_dim;
  int kv_dim = num_kv_heads * head_dim;

  float *q = (float *)malloc(seq_len * q_dim * sizeof(float));
  float *k = (float *)malloc(seq_len * kv_dim * sizeof(float));
  float *v = (float *)malloc(seq_len * kv_dim * sizeof(float));
  if (!q || !k || !v) {
    if (q)
      free(q);
    if (k)
      free(k);
    if (v)
      free(v);
    return;
  }

  gemm_f32(input, q_proj, q, seq_len, q_dim, hidden_size, false, true);
  gemm_f32(input, k_proj, k, seq_len, kv_dim, hidden_size, false, true);
  gemm_f32(input, v_proj, v, seq_len, kv_dim, hidden_size, false, true);

  for (int i = 0; i < seq_len; i++) {
    for (int h = 0; h < num_heads; h++) {
      float *q_head = q + i * q_dim + h * head_dim;
      rms_norm_f32(q_head, q_head, q_norm, 1e-6f, 1, head_dim);
    }
  }

  for (int i = 0; i < seq_len; i++) {
    for (int h = 0; h < num_kv_heads; h++) {
      float *k_head = k + i * kv_dim + h * head_dim;
      rms_norm_f32(k_head, k_head, k_norm, 1e-6f, 1, head_dim);
    }
  }

  rope_f32(position_ids, q, k, cos_sin_cache, seq_len, num_heads, num_kv_heads,
           head_dim, head_dim, true);

  kv_cache_append_f32(key_cache, value_cache, k, v, cache_len, seq_len,
                      num_kv_heads, head_dim);

  int total_seq_len = cache_len + seq_len;
  float scale = 1.0f / sqrtf((float)head_dim);
  int heads_per_kv = num_heads / num_kv_heads;

  float *attn_out = (float *)malloc(seq_len * q_dim * sizeof(float));
  if (!attn_out) {
    free(q);
    free(k);
    free(v);
    return;
  }

  memset(attn_out, 0, seq_len * q_dim * sizeof(float));

  for (int i = 0; i < seq_len; i++) {
    int64_t query_abs_pos = position_ids[i];
    for (int h = 0; h < num_heads; h++) {
      int kv_head = h / heads_per_kv;
      float *q_head = q + i * q_dim + h * head_dim;
      float *out_head = attn_out + i * q_dim + h * head_dim;

      memset(out_head, 0, head_dim * sizeof(float));
      float M = -1e9f;
      float sum = 0.0f;

      for (int pos = 0; pos <= query_abs_pos && pos < total_seq_len; pos++) {
        float *k_pos =
            key_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;
        float *v_pos =
            value_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;

        float score = 0.0f;
        for (int d = 0; d < head_dim; d++) {
          score += q_head[d] * k_pos[d];
        }
        score *= scale;

        float M_new = (score > M) ? score : M;
        float alpha = expf(M - M_new);

        for (int d = 0; d < head_dim; d++) {
          out_head[d] = out_head[d] * alpha + v_pos[d] * expf(score - M_new);
        }
        sum = sum * alpha + expf(score - M_new);
        M = M_new;
      }

      if (sum > 0.0f) {
        for (int d = 0; d < head_dim; d++) {
          out_head[d] /= sum;
        }
      }
    }
  }

  gemm_f32(attn_out, o_proj, output, seq_len, hidden_size, q_dim, false, true);

  free(q);
  free(k);
  free(v);
  free(attn_out);
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

#if HAS_NEON
static inline float dot_product_f16_neon(const uint16_t *a, const uint16_t *b,
                                         int n) {
  float32x4_t sum0 = vdupq_n_f32(0.0f);
  float32x4_t sum1 = vdupq_n_f32(0.0f);
  int i = 0;
  for (; i + 8 <= n; i += 8) {
    float16x8_t a_f16 = vld1q_f16((const float16_t *)&a[i]);
    float16x8_t b_f16 = vld1q_f16((const float16_t *)&b[i]);
    float32x4_t a_lo = vcvt_f32_f16(vget_low_f16(a_f16));
    float32x4_t a_hi = vcvt_f32_f16(vget_high_f16(a_f16));
    float32x4_t b_lo = vcvt_f32_f16(vget_low_f16(b_f16));
    float32x4_t b_hi = vcvt_f32_f16(vget_high_f16(b_f16));
    sum0 = vfmaq_f32(sum0, a_lo, b_lo);
    sum1 = vfmaq_f32(sum1, a_hi, b_hi);
  }
  float32x4_t sum = vaddq_f32(sum0, sum1);
  float result = vaddvq_f32(sum);
  for (; i < n; i++) {
    result += fp16_to_float(a[i]) * fp16_to_float(b[i]);
  }
  return result;
}

static inline void scale_accumulate_f16_neon(float *out, const uint16_t *v,
                                             float alpha, float weight, int n) {
  float32x4_t alpha_v = vdupq_n_f32(alpha);
  float32x4_t weight_v = vdupq_n_f32(weight);
  int i = 0;
  for (; i + 8 <= n; i += 8) {
    float16x8_t v_f16 = vld1q_f16((const float16_t *)&v[i]);
    float32x4_t v_lo = vcvt_f32_f16(vget_low_f16(v_f16));
    float32x4_t v_hi = vcvt_f32_f16(vget_high_f16(v_f16));
    float32x4_t out_lo = vld1q_f32(&out[i]);
    float32x4_t out_hi = vld1q_f32(&out[i + 4]);
    out_lo = vfmaq_f32(vmulq_f32(out_lo, alpha_v), v_lo, weight_v);
    out_hi = vfmaq_f32(vmulq_f32(out_hi, alpha_v), v_hi, weight_v);
    vst1q_f32(&out[i], out_lo);
    vst1q_f32(&out[i + 4], out_hi);
  }
  for (; i < n; i++) {
    out[i] = out[i] * alpha + fp16_to_float(v[i]) * weight;
  }
}

static inline void f32_to_f16_neon(uint16_t *out, const float *in, float scale,
                                   int n) {
  float32x4_t scale_v = vdupq_n_f32(scale);
  int i = 0;
  for (; i + 8 <= n; i += 8) {
    float32x4_t in_lo = vmulq_f32(vld1q_f32(&in[i]), scale_v);
    float32x4_t in_hi = vmulq_f32(vld1q_f32(&in[i + 4]), scale_v);
    float16x8_t out_f16 =
        vcombine_f16(vcvt_f16_f32(in_lo), vcvt_f16_f32(in_hi));
    vst1q_f16((float16_t *)&out[i], out_f16);
  }
  for (; i < n; i++) {
    out[i] = float_to_fp16(in[i] * scale);
  }
}
#endif

void qwen3_attention_layer_f16(uint16_t *output, const uint16_t *input,
                               const uint16_t *q_proj, const uint16_t *k_proj,
                               const uint16_t *v_proj, const uint16_t *o_proj,
                               const uint16_t *q_norm, const uint16_t *k_norm,
                               uint16_t *key_cache, uint16_t *value_cache,
                               const int64_t *position_ids,
                               const uint16_t *cos_sin_cache, int seq_len,
                               int cache_len, int hidden_size, int num_heads,
                               int num_kv_heads, int head_dim, float rope_theta,
                               int max_position) {
  (void)rope_theta;
  (void)max_position;
  int q_dim = num_heads * head_dim;
  int kv_dim = num_kv_heads * head_dim;

  uint16_t *q = (uint16_t *)malloc(seq_len * q_dim * sizeof(uint16_t));
  uint16_t *k = (uint16_t *)malloc(seq_len * kv_dim * sizeof(uint16_t));
  uint16_t *v = (uint16_t *)malloc(seq_len * kv_dim * sizeof(uint16_t));
  if (!q || !k || !v) {
    if (q)
      free(q);
    if (k)
      free(k);
    if (v)
      free(v);
    return;
  }

  gemm_f16(input, q_proj, q, seq_len, q_dim, hidden_size);
  gemm_f16(input, k_proj, k, seq_len, kv_dim, hidden_size);
  gemm_f16(input, v_proj, v, seq_len, kv_dim, hidden_size);

  for (int i = 0; i < seq_len; i++) {
    for (int h = 0; h < num_heads; h++) {
      uint16_t *q_head = q + i * q_dim + h * head_dim;
      rms_norm_f16(q_head, q_head, q_norm, 1e-6f, 1, head_dim);
    }
  }

  for (int i = 0; i < seq_len; i++) {
    for (int h = 0; h < num_kv_heads; h++) {
      uint16_t *k_head = k + i * kv_dim + h * head_dim;
      rms_norm_f16(k_head, k_head, k_norm, 1e-6f, 1, head_dim);
    }
  }

  rope_f16(position_ids, q, k, cos_sin_cache, seq_len, num_heads, num_kv_heads,
           head_dim, head_dim, true);

  kv_cache_append_f16(key_cache, value_cache, k, v, cache_len, seq_len,
                      num_kv_heads, head_dim);

  int total_seq_len = cache_len + seq_len;
  float scale = 1.0f / sqrtf((float)head_dim);
  int heads_per_kv = num_heads / num_kv_heads;

  uint16_t *attn_out = (uint16_t *)malloc(seq_len * q_dim * sizeof(uint16_t));
  if (!attn_out) {
    free(q);
    free(k);
    free(v);
    return;
  }

#if HAS_ACCELERATE
  float *q_f32 = (float *)malloc(head_dim * sizeof(float));
  float *k_f32 = (float *)malloc(total_seq_len * head_dim * sizeof(float));
  float *v_f32 = (float *)malloc(total_seq_len * head_dim * sizeof(float));
  float *scores = (float *)malloc(total_seq_len * sizeof(float));
  float *out_f32 = (float *)malloc(head_dim * sizeof(float));

  if (q_f32 && k_f32 && v_f32 && scores && out_f32) {
    for (int i = 0; i < seq_len; i++) {
      int64_t query_abs_pos = position_ids[i];
      int kv_len = (int)query_abs_pos + 1;
      if (kv_len > total_seq_len)
        kv_len = total_seq_len;

      for (int h = 0; h < num_heads; h++) {
        int kv_head = h / heads_per_kv;
        uint16_t *q_head = q + i * q_dim + h * head_dim;
        uint16_t *out_head = attn_out + i * q_dim + h * head_dim;

        // Convert query to FP32
        for (int d = 0; d < head_dim; d++) {
          q_f32[d] = fp16_to_float(q_head[d]);
        }

        // Gather and convert keys for this kv_head into contiguous FP32 buffer
        for (int pos = 0; pos < kv_len; pos++) {
          uint16_t *k_pos =
              key_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;
          for (int d = 0; d < head_dim; d++) {
            k_f32[pos * head_dim + d] = fp16_to_float(k_pos[d]);
          }
        }

        // Compute attention scores: scores = K @ q (using GEMV)
        // K is [kv_len, head_dim], q is [head_dim], scores is [kv_len]
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        cblas_sgemv(CblasRowMajor, CblasNoTrans, kv_len, head_dim, scale, k_f32,
                    head_dim, q_f32, 1, 0.0f, scores, 1);
#pragma clang diagnostic pop

        // Softmax using vDSP
        float max_score;
        vDSP_maxv(scores, 1, &max_score, kv_len);
        float neg_max = -max_score;
        vDSP_vsadd(scores, 1, &neg_max, scores, 1, kv_len);
        int n = kv_len;
        vvexpf(scores, scores, &n);
        float sum_exp;
        vDSP_sve(scores, 1, &sum_exp, kv_len);
        if (sum_exp > 0.0f) {
          float inv_sum = 1.0f / sum_exp;
          vDSP_vsmul(scores, 1, &inv_sum, scores, 1, kv_len);
        }

        // Gather and convert values for this kv_head into contiguous FP32
        // buffer
        for (int pos = 0; pos < kv_len; pos++) {
          uint16_t *v_pos =
              value_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;
          for (int d = 0; d < head_dim; d++) {
            v_f32[pos * head_dim + d] = fp16_to_float(v_pos[d]);
          }
        }

        // Compute output: out = V^T @ scores (using GEMV with transpose)
        // V is [kv_len, head_dim], scores is [kv_len], out is [head_dim]
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        cblas_sgemv(CblasRowMajor, CblasTrans, kv_len, head_dim, 1.0f, v_f32,
                    head_dim, scores, 1, 0.0f, out_f32, 1);
#pragma clang diagnostic pop

        // Convert output to FP16
        for (int d = 0; d < head_dim; d++) {
          out_head[d] = float_to_fp16(out_f32[d]);
        }
      }
    }

    free(q_f32);
    free(k_f32);
    free(v_f32);
    free(scores);
    free(out_f32);
  } else {
    // Cleanup on allocation failure and fall through to NEON path
    if (q_f32)
      free(q_f32);
    if (k_f32)
      free(k_f32);
    if (v_f32)
      free(v_f32);
    if (scores)
      free(scores);
    if (out_f32)
      free(out_f32);
    goto neon_path;
  }
  goto output_proj;

neon_path:
#endif

  // NEON-optimized attention with online softmax
  for (int i = 0; i < seq_len; i++) {
    int64_t query_abs_pos = position_ids[i];
    for (int h = 0; h < num_heads; h++) {
      int kv_head = h / heads_per_kv;
      uint16_t *q_head = q + i * q_dim + h * head_dim;
      uint16_t *out_head = attn_out + i * q_dim + h * head_dim;

      float out_f32_buf[256];
      memset(out_f32_buf, 0, head_dim * sizeof(float));
      float M = -1e9f;
      float sum = 0.0f;

      for (int pos = 0; pos <= query_abs_pos && pos < total_seq_len; pos++) {
        uint16_t *k_pos =
            key_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;
        uint16_t *v_pos =
            value_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;

#if HAS_NEON
        float score = dot_product_f16_neon(q_head, k_pos, head_dim) * scale;
#else
        float score = 0.0f;
        for (int d = 0; d < head_dim; d++) {
          score += fp16_to_float(q_head[d]) * fp16_to_float(k_pos[d]);
        }
        score *= scale;
#endif

        float M_new = (score > M) ? score : M;
        float alpha = expf(M - M_new);
        float weight = expf(score - M_new);

#if HAS_NEON
        scale_accumulate_f16_neon(out_f32_buf, v_pos, alpha, weight, head_dim);
#else
        for (int d = 0; d < head_dim; d++) {
          out_f32_buf[d] =
              out_f32_buf[d] * alpha + fp16_to_float(v_pos[d]) * weight;
        }
#endif
        sum = sum * alpha + weight;
        M = M_new;
      }

      if (sum > 0.0f) {
#if HAS_NEON
        f32_to_f16_neon(out_head, out_f32_buf, 1.0f / sum, head_dim);
#else
        for (int d = 0; d < head_dim; d++) {
          out_head[d] = float_to_fp16(out_f32_buf[d] / sum);
        }
#endif
      }
    }
  }

#if HAS_ACCELERATE
output_proj:
#endif

  gemm_f16(attn_out, o_proj, output, seq_len, hidden_size, q_dim);

  free(q);
  free(k);
  free(v);
  free(attn_out);
}

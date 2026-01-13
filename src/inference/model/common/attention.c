#include "attention.h"
#include "inference/core/dtype.h"
#include "inference/ops/gemm.h"
#include "inference/ops/kv_cache.h"
#include "inference/ops/norm.h"
#include "inference/ops/rope.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

void attention_forward_f32(float *output, const float *input,
                           const attention_layer_t *attn, float *key_cache,
                           float *value_cache, const int64_t *position_ids,
                           const float *cos_sin_cache, int seq_len,
                           int cache_len) {
  if (!output || !input || !attn)
    return;

  int hidden_size = attn->hidden_size;
  int num_heads = attn->num_heads;
  int num_kv_heads = attn->num_kv_heads;
  int head_dim = attn->head_dim;
  int q_dim = num_heads * head_dim;
  int kv_dim = num_kv_heads * head_dim;

  const float *q_w = tensor_data_f32_const(attn->q_proj);
  const float *k_w = tensor_data_f32_const(attn->k_proj);
  const float *v_w = tensor_data_f32_const(attn->v_proj);
  const float *o_w = tensor_data_f32_const(attn->o_proj);

  float *q = (float *)malloc(seq_len * q_dim * sizeof(float));
  float *k = (float *)malloc(seq_len * kv_dim * sizeof(float));
  float *v = (float *)malloc(seq_len * kv_dim * sizeof(float));
  if (!q || !k || !v) {
    free(q);
    free(k);
    free(v);
    return;
  }

  gemm_f32(input, q_w, q, seq_len, q_dim, hidden_size, false, true);
  gemm_f32(input, k_w, k, seq_len, kv_dim, hidden_size, false, true);
  gemm_f32(input, v_w, v, seq_len, kv_dim, hidden_size, false, true);

  if (attn->has_qk_norm && attn->q_norm && attn->k_norm) {
    const float *q_norm_w = tensor_data_f32_const(attn->q_norm);
    const float *k_norm_w = tensor_data_f32_const(attn->k_norm);
    for (int i = 0; i < seq_len; i++) {
      for (int h = 0; h < num_heads; h++) {
        float *q_head = q + i * q_dim + h * head_dim;
        rms_norm_f32(q_head, q_head, q_norm_w, 1e-6f, 1, head_dim);
      }
    }
    for (int i = 0; i < seq_len; i++) {
      for (int h = 0; h < num_kv_heads; h++) {
        float *k_head = k + i * kv_dim + h * head_dim;
        rms_norm_f32(k_head, k_head, k_norm_w, 1e-6f, 1, head_dim);
      }
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

  gemm_f32(attn_out, o_w, output, seq_len, hidden_size, q_dim, false, true);

  free(q);
  free(k);
  free(v);
  free(attn_out);
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
    result += f16_to_f32(a[i]) * f16_to_f32(b[i]);
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
    out[i] = out[i] * alpha + f16_to_f32(v[i]) * weight;
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
    out[i] = f32_to_f16(in[i] * scale);
  }
}
#endif

void attention_forward_f16(uint16_t *output, const uint16_t *input,
                           const attention_layer_t *attn, uint16_t *key_cache,
                           uint16_t *value_cache, const int64_t *position_ids,
                           const uint16_t *cos_sin_cache, int seq_len,
                           int cache_len) {
  if (!output || !input || !attn)
    return;

  int hidden_size = attn->hidden_size;
  int num_heads = attn->num_heads;
  int num_kv_heads = attn->num_kv_heads;
  int head_dim = attn->head_dim;
  int q_dim = num_heads * head_dim;
  int kv_dim = num_kv_heads * head_dim;

  const uint16_t *q_w = tensor_data_f16_const(attn->q_proj);
  const uint16_t *k_w = tensor_data_f16_const(attn->k_proj);
  const uint16_t *v_w = tensor_data_f16_const(attn->v_proj);
  const uint16_t *o_w = tensor_data_f16_const(attn->o_proj);

  uint16_t *q = (uint16_t *)malloc(seq_len * q_dim * sizeof(uint16_t));
  uint16_t *k = (uint16_t *)malloc(seq_len * kv_dim * sizeof(uint16_t));
  uint16_t *v = (uint16_t *)malloc(seq_len * kv_dim * sizeof(uint16_t));
  if (!q || !k || !v) {
    free(q);
    free(k);
    free(v);
    return;
  }

  gemm_f16(input, q_w, q, seq_len, q_dim, hidden_size);
  gemm_f16(input, k_w, k, seq_len, kv_dim, hidden_size);
  gemm_f16(input, v_w, v, seq_len, kv_dim, hidden_size);

  if (attn->has_qk_norm && attn->q_norm && attn->k_norm) {
    const uint16_t *q_norm_w = tensor_data_f16_const(attn->q_norm);
    const uint16_t *k_norm_w = tensor_data_f16_const(attn->k_norm);
    for (int i = 0; i < seq_len; i++) {
      for (int h = 0; h < num_heads; h++) {
        uint16_t *q_head = q + i * q_dim + h * head_dim;
        rms_norm_f16(q_head, q_head, q_norm_w, 1e-6f, 1, head_dim);
      }
    }
    for (int i = 0; i < seq_len; i++) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k_head = k + i * kv_dim + h * head_dim;
        rms_norm_f16(k_head, k_head, k_norm_w, 1e-6f, 1, head_dim);
      }
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
          score += f16_to_f32(q_head[d]) * f16_to_f32(k_pos[d]);
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
              out_f32_buf[d] * alpha + f16_to_f32(v_pos[d]) * weight;
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
          out_head[d] = f32_to_f16(out_f32_buf[d] / sum);
        }
#endif
      }
    }
  }

  gemm_f16(attn_out, o_w, output, seq_len, hidden_size, q_dim);

  free(q);
  free(k);
  free(v);
  free(attn_out);
}

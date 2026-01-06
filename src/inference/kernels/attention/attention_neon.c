/*
 * Flash Attention - NEON Optimized Implementation
 * Based on llama.cpp's flash attention pattern
 */

#include "inference/kernels/attention/attention.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#define HAS_ACCELERATE 1
#else
#define HAS_ACCELERATE 0
#endif

#if HAS_NEON

static inline float hsum_f32x4(float32x4_t v) {
  float32x2_t sum2 = vadd_f32(vget_low_f32(v), vget_high_f32(v));
  sum2 = vpadd_f32(sum2, sum2);
  return vget_lane_f32(sum2, 0);
}

__attribute__((unused)) static float
dot_product_f32_neon(const float *a, const float *b, int n) {
  float32x4_t sum_v = vdupq_n_f32(0.0f);
  int i = 0;

  for (; i + 4 <= n; i += 4) {
    float32x4_t a_v = vld1q_f32(a + i);
    float32x4_t b_v = vld1q_f32(b + i);
    sum_v = vmlaq_f32(sum_v, a_v, b_v);
  }

  float sum = hsum_f32x4(sum_v);
  for (; i < n; i++) {
    sum += a[i] * b[i];
  }
  return sum;
}

static void vec_scale_f32_neon(float *v, float scale, int n) {
  float32x4_t scale_v = vdupq_n_f32(scale);
  int i = 0;

  for (; i + 4 <= n; i += 4) {
    float32x4_t vec = vld1q_f32(v + i);
    vec = vmulq_f32(vec, scale_v);
    vst1q_f32(v + i, vec);
  }

  for (; i < n; i++) {
    v[i] *= scale;
  }
}

static void vec_mad_f32_neon(float *acc, const float *v, float scale, int n) {
  float32x4_t scale_v = vdupq_n_f32(scale);
  int i = 0;

  for (; i + 4 <= n; i += 4) {
    float32x4_t acc_v = vld1q_f32(acc + i);
    float32x4_t v_v = vld1q_f32(v + i);
    acc_v = vmlaq_f32(acc_v, v_v, scale_v);
    vst1q_f32(acc + i, acc_v);
  }

  for (; i < n; i++) {
    acc[i] += v[i] * scale;
  }
}

void flash_attention_f32_neon(float *output, const float *query,
                              const float *key, const float *value,
                              int seq_len_q, int seq_len_kv, int head_dim,
                              float scale, const float *mask) {
  size_t alloc_size = ((head_dim * sizeof(float) + 63) / 64) * 64;
  float *VKQ = (float *)aligned_alloc(64, alloc_size);

#if HAS_ACCELERATE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  if (seq_len_q * seq_len_kv <= 65536 && seq_len_kv <= 2048) {
    size_t qk_size = ((seq_len_q * seq_len_kv * sizeof(float) + 63) / 64) * 64;
    float *qk = (float *)aligned_alloc(64, qk_size);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasTrans, seq_len_q, seq_len_kv,
                head_dim, scale, (float *)query, head_dim, (float *)key,
                head_dim, 0.0f, qk, seq_len_kv);

    if (mask) {
      for (int q = 0; q < seq_len_q; q++) {
        vDSP_vadd(qk + q * seq_len_kv, 1, mask + q * seq_len_kv, 1,
                  qk + q * seq_len_kv, 1, seq_len_kv);
      }
    }

    for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
      float *qk_row = qk + q_idx * seq_len_kv;

      float M;
      vDSP_maxv(qk_row, 1, &M, seq_len_kv);

      float neg_M = -M;
      vDSP_vsadd(qk_row, 1, &neg_M, qk_row, 1, seq_len_kv);

      int n = seq_len_kv;
      vvexpf(qk_row, qk_row, &n);

      float S;
      vDSP_sve(qk_row, 1, &S, seq_len_kv);

      float S_inv = (S == 0.0f) ? 0.0f : 1.0f / S;
      vDSP_vsmul(qk_row, 1, &S_inv, qk_row, 1, seq_len_kv);
    }

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, seq_len_q, head_dim,
                seq_len_kv, 1.0f, qk, seq_len_kv, (float *)value, head_dim,
                0.0f, output, head_dim);

    free(qk);
    free(VKQ);
    return;
  }

  if (seq_len_kv <= 512) {
    float *scores = (float *)aligned_alloc(
        64, ((seq_len_kv * sizeof(float) + 63) / 64) * 64);

    for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
      const float *q_vec = query + q_idx * head_dim;
      float *out_vec = output + q_idx * head_dim;

      cblas_sgemv(CblasRowMajor, CblasNoTrans, seq_len_kv, head_dim, scale,
                  (float *)key, head_dim, q_vec, 1, 0.0f, scores, 1);

      if (mask) {
        vDSP_vadd(scores, 1, mask + q_idx * seq_len_kv, 1, scores, 1,
                  seq_len_kv);
      }

      float M;
      vDSP_maxv(scores, 1, &M, seq_len_kv);

      float neg_M = -M;
      vDSP_vsadd(scores, 1, &neg_M, scores, 1, seq_len_kv);

      int n = seq_len_kv;
      vvexpf(scores, scores, &n);

      float S;
      vDSP_sve(scores, 1, &S, seq_len_kv);

      float S_inv = (S == 0.0f) ? 0.0f : 1.0f / S;
      vDSP_vsmul(scores, 1, &S_inv, scores, 1, seq_len_kv);

      cblas_sgemv(CblasRowMajor, CblasTrans, seq_len_kv, head_dim, 1.0f,
                  (float *)value, head_dim, scores, 1, 0.0f, out_vec, 1);
    }

    free(scores);
    free(VKQ);
    return;
  }
#pragma clang diagnostic pop
#endif

  for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
    const float *q_vec = query + q_idx * head_dim;
    float *out_vec = output + q_idx * head_dim;

    float M = -__builtin_inff();
    float S = 0.0f;
    memset(VKQ, 0, head_dim * sizeof(float));

    for (int kv_idx = 0; kv_idx < seq_len_kv; kv_idx++) {
      const float *k_vec = key + kv_idx * head_dim;
      const float *v_vec = value + kv_idx * head_dim;

#if HAS_ACCELERATE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      float s = cblas_sdot(head_dim, q_vec, 1, k_vec, 1) * scale;
#pragma clang diagnostic pop
#else
      float s = dot_product_f32_neon(q_vec, k_vec, head_dim) * scale;
#endif

      if (mask) {
        s += mask[q_idx * seq_len_kv + kv_idx];
      }

      if (s == -__builtin_inff()) {
        continue;
      }

      const float Mold = M;
      float ms = 1.0f;
      float vs = 1.0f;

      if (s > M) {
        M = s;
        ms = expf(Mold - M);
        vec_scale_f32_neon(VKQ, ms, head_dim);
      } else {
        vs = expf(s - M);
      }

      vec_mad_f32_neon(VKQ, v_vec, vs, head_dim);
      S = S * ms + vs;
    }

    if (S > 0.0f) {
      float S_inv = 1.0f / S;
      vec_scale_f32_neon(VKQ, S_inv, head_dim);
      memcpy(out_vec, VKQ, head_dim * sizeof(float));
    } else {
      memset(out_vec, 0, head_dim * sizeof(float));
    }
  }

  free(VKQ);
}

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

static inline float fp16_to_float_scalar(uint16_t fp16) {
  float16x4_t v = vreinterpret_f16_u16(vdup_n_u16(fp16));
  float32x4_t f32 = vcvt_f32_f16(v);
  return vgetq_lane_f32(f32, 0);
}

static inline uint16_t float_to_fp16_scalar(float f) {
  float32x4_t v = vdupq_n_f32(f);
  float16x4_t f16 = vcvt_f16_f32(v);
  return vget_lane_u16(vreinterpret_u16_f16(f16), 0);
}

void flash_attention_bf16_neon(uint16_t *output, const uint16_t *query,
                               const uint16_t *key, const uint16_t *value,
                               int seq_len_q, int seq_len_kv, int head_dim,
                               float scale, const float *mask) {
  size_t alloc_size = ((head_dim * sizeof(float) + 63) / 64) * 64;
  float *q_f32 = (float *)aligned_alloc(64, alloc_size);
  float *k_f32 = (float *)aligned_alloc(64, alloc_size);
  float *v_f32 = (float *)aligned_alloc(64, alloc_size);
  float *VKQ = (float *)aligned_alloc(64, alloc_size);

  for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
    const uint16_t *q_vec = query + q_idx * head_dim;
    uint16_t *out_vec = output + q_idx * head_dim;

    for (int d = 0; d < head_dim; d++) {
      q_f32[d] = bf16_to_float(q_vec[d]);
    }

    float M = -__builtin_inff();
    float S = 0.0f;
    memset(VKQ, 0, head_dim * sizeof(float));

    for (int kv_idx = 0; kv_idx < seq_len_kv; kv_idx++) {
      const uint16_t *k_vec = key + kv_idx * head_dim;
      const uint16_t *v_vec = value + kv_idx * head_dim;

      for (int d = 0; d < head_dim; d++) {
        k_f32[d] = bf16_to_float(k_vec[d]);
        v_f32[d] = bf16_to_float(v_vec[d]);
      }

#if HAS_ACCELERATE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      float s = cblas_sdot(head_dim, q_f32, 1, k_f32, 1) * scale;
#pragma clang diagnostic pop
#else
      float s = dot_product_f32_neon(q_f32, k_f32, head_dim) * scale;
#endif

      if (mask) {
        s += mask[q_idx * seq_len_kv + kv_idx];
      }

      if (s == -__builtin_inff()) {
        continue;
      }

      const float Mold = M;
      float ms = 1.0f;
      float vs = 1.0f;

      if (s > M) {
        M = s;
        ms = expf(Mold - M);
        vec_scale_f32_neon(VKQ, ms, head_dim);
      } else {
        vs = expf(s - M);
      }

      vec_mad_f32_neon(VKQ, v_f32, vs, head_dim);
      S = S * ms + vs;
    }

    if (S > 0.0f) {
      float S_inv = 1.0f / S;
      for (int d = 0; d < head_dim; d++) {
        out_vec[d] = float_to_bf16(VKQ[d] * S_inv);
      }
    } else {
      memset(out_vec, 0, head_dim * sizeof(uint16_t));
    }
  }

  free(q_f32);
  free(k_f32);
  free(v_f32);
  free(VKQ);
}

void flash_attention_f16_neon(uint16_t *output, const uint16_t *query,
                              const uint16_t *key, const uint16_t *value,
                              int seq_len_q, int seq_len_kv, int head_dim,
                              float scale, const float *mask) {
  size_t alloc_size = ((head_dim * sizeof(float) + 63) / 64) * 64;
  float *q_f32 = (float *)aligned_alloc(64, alloc_size);
  float *k_f32 = (float *)aligned_alloc(64, alloc_size);
  float *v_f32 = (float *)aligned_alloc(64, alloc_size);
  float *VKQ = (float *)aligned_alloc(64, alloc_size);

  for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
    const uint16_t *q_vec = query + q_idx * head_dim;
    uint16_t *out_vec = output + q_idx * head_dim;

    int d = 0;
    for (; d + 4 <= head_dim; d += 4) {
      float16x4_t f16 = vreinterpret_f16_u16(vld1_u16(q_vec + d));
      float32x4_t f32 = vcvt_f32_f16(f16);
      vst1q_f32(q_f32 + d, f32);
    }
    for (; d < head_dim; d++) {
      q_f32[d] = fp16_to_float_scalar(q_vec[d]);
    }

    float M = -__builtin_inff();
    float S = 0.0f;
    memset(VKQ, 0, head_dim * sizeof(float));

    for (int kv_idx = 0; kv_idx < seq_len_kv; kv_idx++) {
      const uint16_t *k_vec = key + kv_idx * head_dim;
      const uint16_t *v_vec = value + kv_idx * head_dim;

      d = 0;
      for (; d + 4 <= head_dim; d += 4) {
        float16x4_t k_f16 = vreinterpret_f16_u16(vld1_u16(k_vec + d));
        float16x4_t v_f16 = vreinterpret_f16_u16(vld1_u16(v_vec + d));
        vst1q_f32(k_f32 + d, vcvt_f32_f16(k_f16));
        vst1q_f32(v_f32 + d, vcvt_f32_f16(v_f16));
      }
      for (; d < head_dim; d++) {
        k_f32[d] = fp16_to_float_scalar(k_vec[d]);
        v_f32[d] = fp16_to_float_scalar(v_vec[d]);
      }

#if HAS_ACCELERATE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      float s = cblas_sdot(head_dim, q_f32, 1, k_f32, 1) * scale;
#pragma clang diagnostic pop
#else
      float s = dot_product_f32_neon(q_f32, k_f32, head_dim) * scale;
#endif

      if (mask) {
        s += mask[q_idx * seq_len_kv + kv_idx];
      }

      if (s == -__builtin_inff()) {
        continue;
      }

      const float Mold = M;
      float ms = 1.0f;
      float vs = 1.0f;

      if (s > M) {
        M = s;
        ms = expf(Mold - M);
        vec_scale_f32_neon(VKQ, ms, head_dim);
      } else {
        vs = expf(s - M);
      }

      vec_mad_f32_neon(VKQ, v_f32, vs, head_dim);
      S = S * ms + vs;
    }

    if (S > 0.0f) {
      float S_inv = 1.0f / S;
      d = 0;
      for (; d + 4 <= head_dim; d += 4) {
        float32x4_t acc_v = vld1q_f32(VKQ + d);
        acc_v = vmulq_n_f32(acc_v, S_inv);
        float16x4_t f16 = vcvt_f16_f32(acc_v);
        vst1_u16(out_vec + d, vreinterpret_u16_f16(f16));
      }
      for (; d < head_dim; d++) {
        out_vec[d] = float_to_fp16_scalar(VKQ[d] * S_inv);
      }
    } else {
      memset(out_vec, 0, head_dim * sizeof(uint16_t));
    }
  }

  free(q_f32);
  free(k_f32);
  free(v_f32);
  free(VKQ);
}

#endif

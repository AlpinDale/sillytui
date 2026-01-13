/*
 * Rotary Position Embeddings - NEON Optimized Implementations
 */

#include "inference/backend/caps.h"
#include "inference/kernels/rope/rope_kernels.h"
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

rope_caps_t rope_get_capabilities(void) {
  rope_caps_t caps = {0};
  caps.has_neon = caps_has(CAP_NEON);
  return caps;
}

#if HAS_NEON

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

static inline float scalar_bf16_to_f32(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static inline uint16_t scalar_f32_to_bf16(float f32) {
  uint32_t bits;
  memcpy(&bits, &f32, sizeof(float));
  uint32_t lsb = (bits >> 16) & 1;
  bits += 0x7fff + lsb;
  return (uint16_t)(bits >> 16);
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

/* ============ NeoX Style Kernels ============ */

void rope_neox_f32_kernel(const int64_t *positions, float *query, float *key,
                          const float *cos_sin_cache, int num_tokens,
                          int num_heads, int num_kv_heads, int head_size,
                          int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const float *cos_ptr = cos_sin_cache + pos * rot_dim;
    const float *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      float *q = query + t * query_stride + h * head_size;

      int i = 0;
      for (; i <= half_dim - 4; i += 4) {
        float32x4_t x = vld1q_f32(q + i);
        float32x4_t y = vld1q_f32(q + half_dim + i);
        float32x4_t cos_val = vld1q_f32(cos_ptr + i);
        float32x4_t sin_val = vld1q_f32(sin_ptr + i);

        float32x4_t out_x =
            vsubq_f32(vmulq_f32(x, cos_val), vmulq_f32(y, sin_val));
        float32x4_t out_y =
            vaddq_f32(vmulq_f32(y, cos_val), vmulq_f32(x, sin_val));

        vst1q_f32(q + i, out_x);
        vst1q_f32(q + half_dim + i, out_y);
      }

      for (; i < half_dim; i++) {
        float x = q[i];
        float y = q[half_dim + i];
        float cos_v = cos_ptr[i];
        float sin_v = sin_ptr[i];
        q[i] = x * cos_v - y * sin_v;
        q[half_dim + i] = y * cos_v + x * sin_v;
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        float *k = key + t * key_stride + h * head_size;

        int i = 0;
        for (; i <= half_dim - 4; i += 4) {
          float32x4_t x = vld1q_f32(k + i);
          float32x4_t y = vld1q_f32(k + half_dim + i);
          float32x4_t cos_val = vld1q_f32(cos_ptr + i);
          float32x4_t sin_val = vld1q_f32(sin_ptr + i);

          float32x4_t out_x =
              vsubq_f32(vmulq_f32(x, cos_val), vmulq_f32(y, sin_val));
          float32x4_t out_y =
              vaddq_f32(vmulq_f32(y, cos_val), vmulq_f32(x, sin_val));

          vst1q_f32(k + i, out_x);
          vst1q_f32(k + half_dim + i, out_y);
        }

        for (; i < half_dim; i++) {
          float x = k[i];
          float y = k[half_dim + i];
          float cos_v = cos_ptr[i];
          float sin_v = sin_ptr[i];
          k[i] = x * cos_v - y * sin_v;
          k[half_dim + i] = y * cos_v + x * sin_v;
        }
      }
    }
  }
}

void rope_neox_bf16_kernel(const int64_t *positions, uint16_t *query,
                           uint16_t *key, const uint16_t *cos_sin_cache,
                           int num_tokens, int num_heads, int num_kv_heads,
                           int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;

      int i = 0;
      for (; i <= half_dim - 4; i += 4) {
        float32x4_t x = bf16x4_to_f32x4(vld1_u16(q + i));
        float32x4_t y = bf16x4_to_f32x4(vld1_u16(q + half_dim + i));
        float32x4_t cos_val = bf16x4_to_f32x4(vld1_u16(cos_ptr + i));
        float32x4_t sin_val = bf16x4_to_f32x4(vld1_u16(sin_ptr + i));

        float32x4_t out_x =
            vsubq_f32(vmulq_f32(x, cos_val), vmulq_f32(y, sin_val));
        float32x4_t out_y =
            vaddq_f32(vmulq_f32(y, cos_val), vmulq_f32(x, sin_val));

        vst1_u16(q + i, f32x4_to_bf16x4(out_x));
        vst1_u16(q + half_dim + i, f32x4_to_bf16x4(out_y));
      }

      for (; i < half_dim; i++) {
        float x = scalar_bf16_to_f32(q[i]);
        float y = scalar_bf16_to_f32(q[half_dim + i]);
        float cos_v = scalar_bf16_to_f32(cos_ptr[i]);
        float sin_v = scalar_bf16_to_f32(sin_ptr[i]);
        q[i] = scalar_f32_to_bf16(x * cos_v - y * sin_v);
        q[half_dim + i] = scalar_f32_to_bf16(y * cos_v + x * sin_v);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;

        int i = 0;
        for (; i <= half_dim - 4; i += 4) {
          float32x4_t x = bf16x4_to_f32x4(vld1_u16(k + i));
          float32x4_t y = bf16x4_to_f32x4(vld1_u16(k + half_dim + i));
          float32x4_t cos_val = bf16x4_to_f32x4(vld1_u16(cos_ptr + i));
          float32x4_t sin_val = bf16x4_to_f32x4(vld1_u16(sin_ptr + i));

          float32x4_t out_x =
              vsubq_f32(vmulq_f32(x, cos_val), vmulq_f32(y, sin_val));
          float32x4_t out_y =
              vaddq_f32(vmulq_f32(y, cos_val), vmulq_f32(x, sin_val));

          vst1_u16(k + i, f32x4_to_bf16x4(out_x));
          vst1_u16(k + half_dim + i, f32x4_to_bf16x4(out_y));
        }

        for (; i < half_dim; i++) {
          float x = scalar_bf16_to_f32(k[i]);
          float y = scalar_bf16_to_f32(k[half_dim + i]);
          float cos_v = scalar_bf16_to_f32(cos_ptr[i]);
          float sin_v = scalar_bf16_to_f32(sin_ptr[i]);
          k[i] = scalar_f32_to_bf16(x * cos_v - y * sin_v);
          k[half_dim + i] = scalar_f32_to_bf16(y * cos_v + x * sin_v);
        }
      }
    }
  }
}

void rope_neox_f16_kernel(const int64_t *positions, uint16_t *query,
                          uint16_t *key, const uint16_t *cos_sin_cache,
                          int num_tokens, int num_heads, int num_kv_heads,
                          int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;

      int i = 0;
      for (; i <= half_dim - 4; i += 4) {
        float32x4_t x = fp16x4_to_f32x4(vld1_u16(q + i));
        float32x4_t y = fp16x4_to_f32x4(vld1_u16(q + half_dim + i));
        float32x4_t cos_val = fp16x4_to_f32x4(vld1_u16(cos_ptr + i));
        float32x4_t sin_val = fp16x4_to_f32x4(vld1_u16(sin_ptr + i));

        float32x4_t out_x =
            vsubq_f32(vmulq_f32(x, cos_val), vmulq_f32(y, sin_val));
        float32x4_t out_y =
            vaddq_f32(vmulq_f32(y, cos_val), vmulq_f32(x, sin_val));

        vst1_u16(q + i, f32x4_to_fp16x4(out_x));
        vst1_u16(q + half_dim + i, f32x4_to_fp16x4(out_y));
      }

      for (; i < half_dim; i++) {
        float x = scalar_fp16_to_f32(q[i]);
        float y = scalar_fp16_to_f32(q[half_dim + i]);
        float cos_v = scalar_fp16_to_f32(cos_ptr[i]);
        float sin_v = scalar_fp16_to_f32(sin_ptr[i]);
        q[i] = scalar_f32_to_fp16(x * cos_v - y * sin_v);
        q[half_dim + i] = scalar_f32_to_fp16(y * cos_v + x * sin_v);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;

        int i = 0;
        for (; i <= half_dim - 4; i += 4) {
          float32x4_t x = fp16x4_to_f32x4(vld1_u16(k + i));
          float32x4_t y = fp16x4_to_f32x4(vld1_u16(k + half_dim + i));
          float32x4_t cos_val = fp16x4_to_f32x4(vld1_u16(cos_ptr + i));
          float32x4_t sin_val = fp16x4_to_f32x4(vld1_u16(sin_ptr + i));

          float32x4_t out_x =
              vsubq_f32(vmulq_f32(x, cos_val), vmulq_f32(y, sin_val));
          float32x4_t out_y =
              vaddq_f32(vmulq_f32(y, cos_val), vmulq_f32(x, sin_val));

          vst1_u16(k + i, f32x4_to_fp16x4(out_x));
          vst1_u16(k + half_dim + i, f32x4_to_fp16x4(out_y));
        }

        for (; i < half_dim; i++) {
          float x = scalar_fp16_to_f32(k[i]);
          float y = scalar_fp16_to_f32(k[half_dim + i]);
          float cos_v = scalar_fp16_to_f32(cos_ptr[i]);
          float sin_v = scalar_fp16_to_f32(sin_ptr[i]);
          k[i] = scalar_f32_to_fp16(x * cos_v - y * sin_v);
          k[half_dim + i] = scalar_f32_to_fp16(y * cos_v + x * sin_v);
        }
      }
    }
  }
}

/* ============ GPT-J Style Kernels ============ */

void rope_gptj_f32_kernel(const int64_t *positions, float *query, float *key,
                          const float *cos_sin_cache, int num_tokens,
                          int num_heads, int num_kv_heads, int head_size,
                          int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const float *cos_ptr = cos_sin_cache + pos * rot_dim;
    const float *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      float *q = query + t * query_stride + h * head_size;

      for (int i = 0; i < half_dim; i++) {
        int x_idx = 2 * i;
        int y_idx = 2 * i + 1;
        float x = q[x_idx];
        float y = q[y_idx];
        float cos_v = cos_ptr[i];
        float sin_v = sin_ptr[i];
        q[x_idx] = x * cos_v - y * sin_v;
        q[y_idx] = y * cos_v + x * sin_v;
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        float *k = key + t * key_stride + h * head_size;

        for (int i = 0; i < half_dim; i++) {
          int x_idx = 2 * i;
          int y_idx = 2 * i + 1;
          float x = k[x_idx];
          float y = k[y_idx];
          float cos_v = cos_ptr[i];
          float sin_v = sin_ptr[i];
          k[x_idx] = x * cos_v - y * sin_v;
          k[y_idx] = y * cos_v + x * sin_v;
        }
      }
    }
  }
}

void rope_gptj_bf16_kernel(const int64_t *positions, uint16_t *query,
                           uint16_t *key, const uint16_t *cos_sin_cache,
                           int num_tokens, int num_heads, int num_kv_heads,
                           int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;

      for (int i = 0; i < half_dim; i++) {
        int x_idx = 2 * i;
        int y_idx = 2 * i + 1;
        float x = scalar_bf16_to_f32(q[x_idx]);
        float y = scalar_bf16_to_f32(q[y_idx]);
        float cos_v = scalar_bf16_to_f32(cos_ptr[i]);
        float sin_v = scalar_bf16_to_f32(sin_ptr[i]);
        q[x_idx] = scalar_f32_to_bf16(x * cos_v - y * sin_v);
        q[y_idx] = scalar_f32_to_bf16(y * cos_v + x * sin_v);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;

        for (int i = 0; i < half_dim; i++) {
          int x_idx = 2 * i;
          int y_idx = 2 * i + 1;
          float x = scalar_bf16_to_f32(k[x_idx]);
          float y = scalar_bf16_to_f32(k[y_idx]);
          float cos_v = scalar_bf16_to_f32(cos_ptr[i]);
          float sin_v = scalar_bf16_to_f32(sin_ptr[i]);
          k[x_idx] = scalar_f32_to_bf16(x * cos_v - y * sin_v);
          k[y_idx] = scalar_f32_to_bf16(y * cos_v + x * sin_v);
        }
      }
    }
  }
}

void rope_gptj_f16_kernel(const int64_t *positions, uint16_t *query,
                          uint16_t *key, const uint16_t *cos_sin_cache,
                          int num_tokens, int num_heads, int num_kv_heads,
                          int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;

      for (int i = 0; i < half_dim; i++) {
        int x_idx = 2 * i;
        int y_idx = 2 * i + 1;
        float x = scalar_fp16_to_f32(q[x_idx]);
        float y = scalar_fp16_to_f32(q[y_idx]);
        float cos_v = scalar_fp16_to_f32(cos_ptr[i]);
        float sin_v = scalar_fp16_to_f32(sin_ptr[i]);
        q[x_idx] = scalar_f32_to_fp16(x * cos_v - y * sin_v);
        q[y_idx] = scalar_f32_to_fp16(y * cos_v + x * sin_v);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;

        for (int i = 0; i < half_dim; i++) {
          int x_idx = 2 * i;
          int y_idx = 2 * i + 1;
          float x = scalar_fp16_to_f32(k[x_idx]);
          float y = scalar_fp16_to_f32(k[y_idx]);
          float cos_v = scalar_fp16_to_f32(cos_ptr[i]);
          float sin_v = scalar_fp16_to_f32(sin_ptr[i]);
          k[x_idx] = scalar_f32_to_fp16(x * cos_v - y * sin_v);
          k[y_idx] = scalar_f32_to_fp16(y * cos_v + x * sin_v);
        }
      }
    }
  }
}

#else /* !HAS_NEON - stubs */

void rope_neox_f32_kernel(const int64_t *positions, float *query, float *key,
                          const float *cos_sin_cache, int num_tokens,
                          int num_heads, int num_kv_heads, int head_size,
                          int rot_dim) {
  (void)positions;
  (void)query;
  (void)key;
  (void)cos_sin_cache;
  (void)num_tokens;
  (void)num_heads;
  (void)num_kv_heads;
  (void)head_size;
  (void)rot_dim;
}

void rope_neox_bf16_kernel(const int64_t *positions, uint16_t *query,
                           uint16_t *key, const uint16_t *cos_sin_cache,
                           int num_tokens, int num_heads, int num_kv_heads,
                           int head_size, int rot_dim) {
  (void)positions;
  (void)query;
  (void)key;
  (void)cos_sin_cache;
  (void)num_tokens;
  (void)num_heads;
  (void)num_kv_heads;
  (void)head_size;
  (void)rot_dim;
}

void rope_neox_f16_kernel(const int64_t *positions, uint16_t *query,
                          uint16_t *key, const uint16_t *cos_sin_cache,
                          int num_tokens, int num_heads, int num_kv_heads,
                          int head_size, int rot_dim) {
  (void)positions;
  (void)query;
  (void)key;
  (void)cos_sin_cache;
  (void)num_tokens;
  (void)num_heads;
  (void)num_kv_heads;
  (void)head_size;
  (void)rot_dim;
}

void rope_gptj_f32_kernel(const int64_t *positions, float *query, float *key,
                          const float *cos_sin_cache, int num_tokens,
                          int num_heads, int num_kv_heads, int head_size,
                          int rot_dim) {
  (void)positions;
  (void)query;
  (void)key;
  (void)cos_sin_cache;
  (void)num_tokens;
  (void)num_heads;
  (void)num_kv_heads;
  (void)head_size;
  (void)rot_dim;
}

void rope_gptj_bf16_kernel(const int64_t *positions, uint16_t *query,
                           uint16_t *key, const uint16_t *cos_sin_cache,
                           int num_tokens, int num_heads, int num_kv_heads,
                           int head_size, int rot_dim) {
  (void)positions;
  (void)query;
  (void)key;
  (void)cos_sin_cache;
  (void)num_tokens;
  (void)num_heads;
  (void)num_kv_heads;
  (void)head_size;
  (void)rot_dim;
}

void rope_gptj_f16_kernel(const int64_t *positions, uint16_t *query,
                          uint16_t *key, const uint16_t *cos_sin_cache,
                          int num_tokens, int num_heads, int num_kv_heads,
                          int head_size, int rot_dim) {
  (void)positions;
  (void)query;
  (void)key;
  (void)cos_sin_cache;
  (void)num_tokens;
  (void)num_heads;
  (void)num_kv_heads;
  (void)head_size;
  (void)rot_dim;
}

#endif /* HAS_NEON */

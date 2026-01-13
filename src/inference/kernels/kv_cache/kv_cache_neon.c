/*
 * KV Cache - NEON Optimized Implementation
 */

#include "inference/backend/caps.h"
#include "inference/kernels/kv_cache/kv_cache_kernels.h"
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

kv_cache_caps_t kv_cache_get_capabilities(void) {
  kv_cache_caps_t caps = {0};
  caps.has_neon = caps_has(CAP_NEON);
  return caps;
}

#if HAS_NEON

static inline void memcpy_neon_f32(float *dst, const float *src, size_t count) {
  size_t i = 0;
  for (; i + 4 <= count; i += 4) {
    float32x4_t v = vld1q_f32(src + i);
    vst1q_f32(dst + i, v);
  }
  for (; i < count; i++) {
    dst[i] = src[i];
  }
}

static inline void memcpy_neon_u16(uint16_t *dst, const uint16_t *src,
                                   size_t count) {
  size_t i = 0;
  for (; i + 8 <= count; i += 8) {
    uint16x8_t v = vld1q_u16(src + i);
    vst1q_u16(dst + i, v);
  }
  for (; i < count; i++) {
    dst[i] = src[i];
  }
}

void kv_cache_append_f32_kernel(float *key_cache, float *value_cache,
                                const float *key, const float *value,
                                int cache_len, int num_tokens, int num_heads,
                                int head_dim) {
  int head_size = num_heads * head_dim;

  for (int t = 0; t < num_tokens; t++) {
    int cache_offset = (cache_len + t) * head_size;
    int input_offset = t * head_size;

    memcpy_neon_f32(key_cache + cache_offset, key + input_offset, head_size);
    memcpy_neon_f32(value_cache + cache_offset, value + input_offset,
                    head_size);
  }
}

void kv_cache_append_bf16_kernel(uint16_t *key_cache, uint16_t *value_cache,
                                 const uint16_t *key, const uint16_t *value,
                                 int cache_len, int num_tokens, int num_heads,
                                 int head_dim) {
  int head_size = num_heads * head_dim;

  for (int t = 0; t < num_tokens; t++) {
    int cache_offset = (cache_len + t) * head_size;
    int input_offset = t * head_size;

    memcpy_neon_u16(key_cache + cache_offset, key + input_offset, head_size);
    memcpy_neon_u16(value_cache + cache_offset, value + input_offset,
                    head_size);
  }
}

void kv_cache_append_f16_kernel(uint16_t *key_cache, uint16_t *value_cache,
                                const uint16_t *key, const uint16_t *value,
                                int cache_len, int num_tokens, int num_heads,
                                int head_dim) {
  int head_size = num_heads * head_dim;

  for (int t = 0; t < num_tokens; t++) {
    int cache_offset = (cache_len + t) * head_size;
    int input_offset = t * head_size;

    memcpy_neon_u16(key_cache + cache_offset, key + input_offset, head_size);
    memcpy_neon_u16(value_cache + cache_offset, value + input_offset,
                    head_size);
  }
}

#else // HAS_NEON

void kv_cache_append_f32_kernel(float *key_cache, float *value_cache,
                                const float *key, const float *value,
                                int cache_len, int num_tokens, int num_heads,
                                int head_dim) {
  (void)key_cache;
  (void)value_cache;
  (void)key;
  (void)value;
  (void)cache_len;
  (void)num_tokens;
  (void)num_heads;
  (void)head_dim;
}

void kv_cache_append_bf16_kernel(uint16_t *key_cache, uint16_t *value_cache,
                                 const uint16_t *key, const uint16_t *value,
                                 int cache_len, int num_tokens, int num_heads,
                                 int head_dim) {
  (void)key_cache;
  (void)value_cache;
  (void)key;
  (void)value;
  (void)cache_len;
  (void)num_tokens;
  (void)num_heads;
  (void)head_dim;
}

void kv_cache_append_f16_kernel(uint16_t *key_cache, uint16_t *value_cache,
                                const uint16_t *key, const uint16_t *value,
                                int cache_len, int num_tokens, int num_heads,
                                int head_dim) {
  (void)key_cache;
  (void)value_cache;
  (void)key;
  (void)value;
  (void)cache_len;
  (void)num_tokens;
  (void)num_heads;
  (void)head_dim;
}

#endif // HAS_NEON

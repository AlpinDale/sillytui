/*
 * RoPE kernel interface for architecture-specific implementations
 */

#ifndef ROPE_KERNELS_H
#define ROPE_KERNELS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool has_neon;
  bool has_avx2;
  bool has_avx512;
} rope_caps_t;

rope_caps_t rope_get_capabilities(void);

void rope_neox_f32_kernel(const int64_t *positions, float *query, float *key,
                          const float *cos_sin_cache, int num_tokens,
                          int num_heads, int num_kv_heads, int head_size,
                          int rot_dim);

void rope_neox_bf16_kernel(const int64_t *positions, uint16_t *query,
                           uint16_t *key, const uint16_t *cos_sin_cache,
                           int num_tokens, int num_heads, int num_kv_heads,
                           int head_size, int rot_dim);

void rope_neox_f16_kernel(const int64_t *positions, uint16_t *query,
                          uint16_t *key, const uint16_t *cos_sin_cache,
                          int num_tokens, int num_heads, int num_kv_heads,
                          int head_size, int rot_dim);

void rope_gptj_f32_kernel(const int64_t *positions, float *query, float *key,
                          const float *cos_sin_cache, int num_tokens,
                          int num_heads, int num_kv_heads, int head_size,
                          int rot_dim);

void rope_gptj_bf16_kernel(const int64_t *positions, uint16_t *query,
                           uint16_t *key, const uint16_t *cos_sin_cache,
                           int num_tokens, int num_heads, int num_kv_heads,
                           int head_size, int rot_dim);

void rope_gptj_f16_kernel(const int64_t *positions, uint16_t *query,
                          uint16_t *key, const uint16_t *cos_sin_cache,
                          int num_tokens, int num_heads, int num_kv_heads,
                          int head_size, int rot_dim);

#endif /* ROPE_KERNELS_H */

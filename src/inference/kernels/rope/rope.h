/*
 * Rotary Position Embeddings (RoPE) for Transformer Inference
 *
 * Implements RoPE for query and key tensors using precomputed cos/sin cache.
 * Supports both NeoX (Llama) and GPT-J interleaving styles.
 */

#ifndef ROPE_H
#define ROPE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Apply rotary position embeddings to query and key tensors.
 *
 * RoPE operation: For each position, rotate pairs of dimensions by the
 * corresponding angle from the cos/sin cache.
 *
 * NeoX style (is_neox=true, used by Llama/Qwen/Mistral):
 *   x, y = tensor[..., :rot_dim/2], tensor[..., rot_dim/2:rot_dim]
 *   out_x = x * cos - y * sin
 *   out_y = y * cos + x * sin
 *
 * GPT-J style (is_neox=false):
 *   x, y = tensor[..., 0::2], tensor[..., 1::2]
 *   out_x = x * cos - y * sin
 *   out_y = y * cos + x * sin
 *
 * Args:
 *   positions: Token positions [num_tokens]
 *   query: Query tensor [num_tokens, num_heads * head_size], modified in-place
 *   key: Key tensor [num_tokens, num_kv_heads * head_size], modified in-place
 *        Can be NULL if only applying to query
 *   cos_sin_cache: Precomputed cos/sin values [max_position, rot_dim]
 *                  Layout: [cos_0, cos_1, ..., sin_0, sin_1, ...]
 *   num_tokens: Number of tokens
 *   num_heads: Number of query heads
 *   num_kv_heads: Number of key/value heads (for GQA/MQA)
 *   head_size: Size of each attention head
 *   rot_dim: Number of dimensions to rotate (typically head_size or
 * head_size/2) is_neox: true for NeoX style (Llama), false for GPT-J style
 */
void rope_f32(const int64_t *positions, float *query, float *key,
              const float *cos_sin_cache, int num_tokens, int num_heads,
              int num_kv_heads, int head_size, int rot_dim, bool is_neox);

void rope_bf16(const int64_t *positions, uint16_t *query, uint16_t *key,
               const uint16_t *cos_sin_cache, int num_tokens, int num_heads,
               int num_kv_heads, int head_size, int rot_dim, bool is_neox);

void rope_f16(const int64_t *positions, uint16_t *query, uint16_t *key,
              const uint16_t *cos_sin_cache, int num_tokens, int num_heads,
              int num_kv_heads, int head_size, int rot_dim, bool is_neox);

/*
 * Compute cos/sin cache for RoPE.
 *
 * Precomputes cos(m * theta_i) and sin(m * theta_i) for all positions m
 * and dimension indices i, where theta_i = base^(-2i/dim).
 *
 * Args:
 *   cache: Output buffer [max_position, rot_dim]
 *          Layout: [cos_0..cos_{rot_dim/2-1}, sin_0..sin_{rot_dim/2-1}]
 *   max_position: Maximum sequence position to compute
 *   rot_dim: Number of rotary dimensions (must be even)
 *   base: RoPE base frequency (default 10000.0 for most models)
 */
void rope_compute_cos_sin_cache_f32(float *cache, int max_position, int rot_dim,
                                    float base);

#ifdef __cplusplus
}
#endif

#endif /* ROPE_H */

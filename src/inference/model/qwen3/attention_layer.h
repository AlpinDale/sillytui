#ifndef QWEN3_ATTENTION_LAYER_H
#define QWEN3_ATTENTION_LAYER_H

#include <stddef.h>
#include <stdint.h>

void qwen3_attention_layer_f32(
    float *output, const float *input, const float *q_proj, const float *k_proj,
    const float *v_proj, const float *o_proj, const float *q_norm,
    const float *k_norm, float *key_cache, float *value_cache,
    const int64_t *position_ids, const float *cos_sin_cache, int seq_len,
    int cache_len, int hidden_size, int num_heads, int num_kv_heads,
    int head_dim, float rope_theta, int max_position);

void qwen3_attention_layer_f16(uint16_t *output, const uint16_t *input,
                               const uint16_t *q_proj, const uint16_t *k_proj,
                               const uint16_t *v_proj, const uint16_t *o_proj,
                               const uint16_t *q_norm, const uint16_t *k_norm,
                               uint16_t *key_cache, uint16_t *value_cache,
                               const int64_t *position_ids,
                               const uint16_t *cos_sin_cache, int seq_len,
                               int cache_len, int hidden_size, int num_heads,
                               int num_kv_heads, int head_dim, float rope_theta,
                               int max_position);

#endif

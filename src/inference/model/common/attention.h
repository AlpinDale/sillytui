#ifndef INFERENCE_MODEL_COMMON_ATTENTION_H
#define INFERENCE_MODEL_COMMON_ATTENTION_H

#include "inference/core/tensor.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  tensor_t *q_proj;
  tensor_t *k_proj;
  tensor_t *v_proj;
  tensor_t *o_proj;
  tensor_t *q_norm;
  tensor_t *k_norm;
  int hidden_size;
  int num_heads;
  int num_kv_heads;
  int head_dim;
  float rope_theta;
  int max_position;
  bool has_qk_norm;
  bool has_bias;
} attention_layer_t;

typedef struct {
  void *key_cache;
  void *value_cache;
  int cache_len;
  int max_cache_len;
  int num_kv_heads;
  int head_dim;
  dtype_t dtype;
} kv_cache_t;

void attention_forward_f32(float *output, const float *input,
                           const attention_layer_t *attn, float *key_cache,
                           float *value_cache, const int64_t *position_ids,
                           const float *cos_sin_cache, int seq_len,
                           int cache_len);

void attention_forward_f16(uint16_t *output, const uint16_t *input,
                           const attention_layer_t *attn, uint16_t *key_cache,
                           uint16_t *value_cache, const int64_t *position_ids,
                           const uint16_t *cos_sin_cache, int seq_len,
                           int cache_len);

#endif

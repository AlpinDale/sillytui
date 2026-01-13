#ifndef INFERENCE_MODEL_COMMON_TRANSFORMER_H
#define INFERENCE_MODEL_COMMON_TRANSFORMER_H

#include "attention.h"
#include "ffn.h"
#include "inference/core/tensor.h"
#include "inference/model/config.h"
#include <stdint.h>

typedef struct {
  attention_layer_t attention;
  ffn_layer_t ffn;
  tensor_t *input_norm;
  tensor_t *post_attn_norm;
  norm_type_t norm_type;
  float norm_eps;
} transformer_layer_t;

typedef struct {
  tensor_t *embed_tokens;
  tensor_t *final_norm;
  tensor_t *lm_head;
  transformer_layer_t *layers;
  int num_layers;
  int hidden_size;
  int vocab_size;
  int max_position;
  norm_type_t norm_type;
  float norm_eps;
  bool tie_word_embeddings;
  void *cos_sin_cache;
  dtype_t dtype;
} transformer_model_t;

void transformer_layer_forward_f32(float *output, const float *input,
                                   const transformer_layer_t *layer,
                                   float *key_cache, float *value_cache,
                                   const int64_t *position_ids,
                                   const float *cos_sin_cache, int seq_len,
                                   int cache_len, int hidden_size);

void transformer_layer_forward_f16(uint16_t *output, const uint16_t *input,
                                   const transformer_layer_t *layer,
                                   uint16_t *key_cache, uint16_t *value_cache,
                                   const int64_t *position_ids,
                                   const uint16_t *cos_sin_cache, int seq_len,
                                   int cache_len, int hidden_size);

#endif

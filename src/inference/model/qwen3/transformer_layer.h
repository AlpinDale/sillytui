#ifndef QWEN3_TRANSFORMER_LAYER_H
#define QWEN3_TRANSFORMER_LAYER_H

#include "config.h"
#include "weights.h"
#include <stddef.h>
#include <stdint.h>

void qwen3_transformer_layer_f32(float *output, const float *input,
                                 const qwen3_layer_weights_t *weights,
                                 float *key_cache, float *value_cache,
                                 const int64_t *position_ids,
                                 const float *cos_sin_cache,
                                 const qwen3_config_t *config, int seq_len,
                                 int cache_len, int layer_idx);

void qwen3_transformer_layer_f16(uint16_t *output, const uint16_t *input,
                                 const qwen3_layer_weights_t *weights,
                                 uint16_t *key_cache, uint16_t *value_cache,
                                 const int64_t *position_ids,
                                 const uint16_t *cos_sin_cache,
                                 const qwen3_config_t *config, int seq_len,
                                 int cache_len, int layer_idx);

#endif

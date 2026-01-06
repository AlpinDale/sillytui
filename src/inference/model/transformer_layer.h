#ifndef QWEN3_TRANSFORMER_LAYER_H
#define QWEN3_TRANSFORMER_LAYER_H

#include "inference/model/config.h"
#include "inference/model/weights.h"
#include <stddef.h>

void qwen3_transformer_layer_f32(float *output, const float *input,
                                 const qwen3_layer_weights_t *weights,
                                 float *key_cache, float *value_cache,
                                 const int64_t *position_ids,
                                 const float *cos_sin_cache,
                                 const qwen3_config_t *config, int seq_len,
                                 int cache_len, int layer_idx);

#endif

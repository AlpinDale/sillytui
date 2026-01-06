#ifndef QWEN3_WEIGHTS_H
#define QWEN3_WEIGHTS_H

#include "inference/model/config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float *q_proj;
  float *k_proj;
  float *v_proj;
  float *o_proj;
  float *q_norm;
  float *k_norm;
  float *gate_proj;
  float *up_proj;
  float *down_proj;
  float *attn_norm;
  float *ffn_norm;
} qwen3_layer_weights_t;

typedef struct {
  float *embed_tokens;
  float *norm;
  float *lm_head;
  qwen3_layer_weights_t *layers;
  int num_layers;
} qwen3_weights_t;

bool qwen3_weights_load(qwen3_weights_t *weights, const qwen3_config_t *config,
                        const char *model_path);
void qwen3_weights_free(qwen3_weights_t *weights);

#ifdef __cplusplus
}
#endif

#endif

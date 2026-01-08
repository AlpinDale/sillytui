#ifndef QWEN3_WEIGHTS_H
#define QWEN3_WEIGHTS_H

#include "config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  QWEN3_DTYPE_F32 = 0,
  QWEN3_DTYPE_F16 = 1,
} qwen3_dtype_t;

typedef struct {
  void *q_proj;
  void *k_proj;
  void *v_proj;
  void *o_proj;
  void *q_norm;
  void *k_norm;
  void *gate_proj;
  void *up_proj;
  void *down_proj;
  void *attn_norm;
  void *ffn_norm;
} qwen3_layer_weights_t;

typedef struct {
  void *embed_tokens;
  void *norm;
  void *lm_head;
  qwen3_layer_weights_t *layers;
  int num_layers;
  qwen3_dtype_t dtype;
} qwen3_weights_t;

bool qwen3_weights_load(qwen3_weights_t *weights, const qwen3_config_t *config,
                        const char *model_path, qwen3_dtype_t dtype);
void qwen3_weights_free(qwen3_weights_t *weights);

#ifdef __cplusplus
}
#endif

#endif

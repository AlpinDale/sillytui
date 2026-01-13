#ifndef QWEN3_WEIGHTS_H
#define QWEN3_WEIGHTS_H

#include "inference/core/dtype.h"
#include "inference/core/tensor.h"
#include "inference/model/config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Per-layer weights for Qwen3 transformer.
 * Uses tensor_t for proper dtype tracking and memory management.
 */
typedef struct {
  /* Attention projections */
  tensor_t *q_proj;
  tensor_t *k_proj;
  tensor_t *v_proj;
  tensor_t *o_proj;

  /* QK normalization weights (Qwen3-specific) */
  tensor_t *q_norm;
  tensor_t *k_norm;

  /* FFN projections */
  tensor_t *gate_proj;
  tensor_t *up_proj;
  tensor_t *down_proj;

  /* Layer normalization weights */
  tensor_t *input_norm;     /* Pre-attention norm */
  tensor_t *post_attn_norm; /* Pre-FFN norm */
} qwen3_layer_weights_t;

/**
 * Full model weights for Qwen3.
 */
typedef struct {
  tensor_t *embed_tokens; /* Token embeddings [vocab_size, hidden_size] */
  tensor_t *final_norm;   /* Final layer norm [hidden_size] */
  tensor_t *lm_head;      /* Language model head [vocab_size, hidden_size] */

  qwen3_layer_weights_t *layers; /* Per-layer weights */
  int num_layers;

  dtype_t dtype; /* Inference dtype */
} qwen3_weights_t;

/**
 * Load model weights from safetensors file.
 *
 * @param weights Output weights structure
 * @param config Model configuration
 * @param model_path Path to model.safetensors
 * @param dtype Target dtype for inference (F32 or F16)
 * @return true on success, false on failure
 */
bool qwen3_weights_load(qwen3_weights_t *weights, const model_config_t *config,
                        const char *model_path, dtype_t dtype);

/**
 * Free all weights and tensors.
 */
void qwen3_weights_free(qwen3_weights_t *weights);

#ifdef __cplusplus
}
#endif

#endif

#ifndef QWEN3_H
#define QWEN3_H

#include "config.h"
#include "weights.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  qwen3_config_t config;
  qwen3_weights_t weights;
  qwen3_dtype_t dtype;

  void **key_cache;
  void **value_cache;
  int *cache_len;
  int max_seq_len;

  void *cos_sin_cache;

  void *temp_buffer;
  size_t temp_buffer_size;
} qwen3_model_t;

bool qwen3_model_load(qwen3_model_t *model, const char *model_dir,
                      qwen3_dtype_t dtype);
void qwen3_model_free(qwen3_model_t *model);
void qwen3_model_reset_cache(qwen3_model_t *model);

bool qwen3_forward(qwen3_model_t *model, float *logits, const int *token_ids,
                   int num_tokens);
int qwen3_generate(qwen3_model_t *model, int *output_tokens, int max_tokens,
                   const int *input_tokens, int num_input_tokens,
                   float temperature, int top_k, float top_p);

#endif

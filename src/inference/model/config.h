#ifndef QWEN3_CONFIG_H
#define QWEN3_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  int hidden_size;
  int num_attention_heads;
  int num_key_value_heads;
  int num_hidden_layers;
  int intermediate_size;
  int vocab_size;
  int max_position_embeddings;
  int head_dim;
  float rope_theta;
  float rms_norm_eps;
  char hidden_act[32];
  bool attention_bias;
  int bos_token_id;
  int eos_token_id;
  bool tie_word_embeddings;
} qwen3_config_t;

bool qwen3_config_load(qwen3_config_t *config, const char *config_path);
void qwen3_config_free(qwen3_config_t *config);

#endif

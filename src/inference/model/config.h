#ifndef INFERENCE_MODEL_CONFIG_H
#define INFERENCE_MODEL_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  ACT_SILU,
  ACT_GELU,
  ACT_GELU_TANH,
  ACT_GELU_QUICK,
  ACT_RELU,
  ACT_UNKNOWN
} activation_type_t;

typedef enum { NORM_RMS, NORM_LAYER, NORM_UNKNOWN } norm_type_t;

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
  float norm_eps;
  activation_type_t hidden_act;
  norm_type_t norm_type;
  bool attention_bias;
  bool mlp_bias;
  int bos_token_id;
  int eos_token_id;
  bool tie_word_embeddings;
  char model_type[64];
} model_config_t;

bool model_config_load(model_config_t *config, const char *config_path);
void model_config_free(model_config_t *config);
activation_type_t activation_from_string(const char *name);
const char *activation_to_string(activation_type_t act);

#endif

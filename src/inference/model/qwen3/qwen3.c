#include "qwen3.h"
#include "inference/kernels/embedding/embedding.h"
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/norm/layernorm.h"
#include "inference/kernels/rope/rope.h"
#include "inference/kernels/sampling/sampling.h"
#include "transformer_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool qwen3_model_load(qwen3_model_t *model, const char *model_dir,
                      qwen3_dtype_t dtype) {
  if (!model || !model_dir)
    return false;

  memset(model, 0, sizeof(*model));
  model->dtype = dtype;

  char config_path[512];
  snprintf(config_path, sizeof(config_path), "%s/config.json", model_dir);
  if (!qwen3_config_load(&model->config, config_path)) {
    fprintf(stderr, "Failed to load config from %s\n", config_path);
    return false;
  }

  char model_path[512];
  snprintf(model_path, sizeof(model_path), "%s/model.safetensors", model_dir);
  if (!qwen3_weights_load(&model->weights, &model->config, model_path, dtype)) {
    fprintf(stderr, "Failed to load weights from %s\n", model_path);
    qwen3_config_free(&model->config);
    return false;
  }

  model->max_seq_len = model->config.max_position_embeddings;

  model->key_cache =
      (void **)calloc(model->config.num_hidden_layers, sizeof(void *));
  model->value_cache =
      (void **)calloc(model->config.num_hidden_layers, sizeof(void *));
  model->cache_len =
      (int *)calloc(model->config.num_hidden_layers, sizeof(int));

  if (!model->key_cache || !model->value_cache || !model->cache_len) {
    qwen3_model_free(model);
    return false;
  }

  int kv_cache_size = model->max_seq_len * model->config.num_key_value_heads *
                      model->config.head_dim;
  size_t elem_size =
      (dtype == QWEN3_DTYPE_F16) ? sizeof(uint16_t) : sizeof(float);
  for (int i = 0; i < model->config.num_hidden_layers; i++) {
    model->key_cache[i] = calloc(kv_cache_size, elem_size);
    model->value_cache[i] = calloc(kv_cache_size, elem_size);
    if (!model->key_cache[i] || !model->value_cache[i]) {
      qwen3_model_free(model);
      return false;
    }
  }

  int rot_dim = model->config.head_dim;
  int cache_size = model->max_seq_len * rot_dim * 2;
  if (dtype == QWEN3_DTYPE_F16) {
    float *cos_sin_f32 = (float *)malloc(cache_size * sizeof(float));
    if (!cos_sin_f32) {
      qwen3_model_free(model);
      return false;
    }
    rope_compute_cos_sin_cache_f32(cos_sin_f32, model->max_seq_len, rot_dim,
                                   model->config.rope_theta);
    model->cos_sin_cache = malloc(cache_size * sizeof(uint16_t));
    if (!model->cos_sin_cache) {
      free(cos_sin_f32);
      qwen3_model_free(model);
      return false;
    }
    f32_array_to_f16(cos_sin_f32, (uint16_t *)model->cos_sin_cache, cache_size);
    free(cos_sin_f32);
  } else {
    model->cos_sin_cache = malloc(cache_size * sizeof(float));
    if (!model->cos_sin_cache) {
      qwen3_model_free(model);
      return false;
    }
    rope_compute_cos_sin_cache_f32((float *)model->cos_sin_cache,
                                   model->max_seq_len, rot_dim,
                                   model->config.rope_theta);
  }

  model->temp_buffer_size = model->config.hidden_size * 8;
  model->temp_buffer = malloc(model->temp_buffer_size * elem_size);
  if (!model->temp_buffer) {
    qwen3_model_free(model);
    return false;
  }

  return true;
}

void qwen3_model_free(qwen3_model_t *model) {
  if (!model)
    return;

  qwen3_config_free(&model->config);
  qwen3_weights_free(&model->weights);

  if (model->key_cache) {
    for (int i = 0; i < model->config.num_hidden_layers; i++) {
      if (model->key_cache[i])
        free(model->key_cache[i]);
    }
    free(model->key_cache);
  }

  if (model->value_cache) {
    for (int i = 0; i < model->config.num_hidden_layers; i++) {
      if (model->value_cache[i])
        free(model->value_cache[i]);
    }
    free(model->value_cache);
  }

  if (model->cache_len)
    free(model->cache_len);

  if (model->cos_sin_cache)
    free(model->cos_sin_cache);

  if (model->temp_buffer)
    free(model->temp_buffer);

  memset(model, 0, sizeof(*model));
}

void qwen3_model_reset_cache(qwen3_model_t *model) {
  if (!model || !model->cache_len)
    return;
  for (int i = 0; i < model->config.num_hidden_layers; i++) {
    model->cache_len[i] = 0;
  }
}

bool qwen3_forward(qwen3_model_t *model, float *logits, const int *token_ids,
                   int num_tokens) {
  if (!model || !logits || !token_ids || num_tokens <= 0)
    return false;

  int64_t *token_ids_i64 = (int64_t *)malloc(num_tokens * sizeof(int64_t));
  if (!token_ids_i64)
    return false;
  for (int i = 0; i < num_tokens; i++) {
    token_ids_i64[i] = token_ids[i];
  }

  int64_t *position_ids = (int64_t *)malloc(num_tokens * sizeof(int64_t));
  if (!position_ids) {
    free(token_ids_i64);
    return false;
  }
  int start_pos = 0;
  if (model->cache_len && model->cache_len[0] > 0) {
    start_pos = model->cache_len[0];
  }
  for (int i = 0; i < num_tokens; i++) {
    position_ids[i] = start_pos + i;
  }

  if (model->dtype == QWEN3_DTYPE_F16) {
    uint16_t *hidden = (uint16_t *)malloc(
        num_tokens * model->config.hidden_size * sizeof(uint16_t));
    if (!hidden) {
      free(token_ids_i64);
      free(position_ids);
      return false;
    }

    embedding_lookup_f16(
        hidden, token_ids_i64, (uint16_t *)model->weights.embed_tokens,
        num_tokens, model->config.vocab_size, model->config.hidden_size, -1);
    free(token_ids_i64);

    uint16_t *layer_input = hidden;
    uint16_t *layer_output = (uint16_t *)malloc(
        num_tokens * model->config.hidden_size * sizeof(uint16_t));
    if (!layer_output) {
      free(hidden);
      free(position_ids);
      return false;
    }

    for (int layer_idx = 0; layer_idx < model->config.num_hidden_layers;
         layer_idx++) {
      qwen3_transformer_layer_f16(
          layer_output, layer_input, &model->weights.layers[layer_idx],
          (uint16_t *)model->key_cache[layer_idx],
          (uint16_t *)model->value_cache[layer_idx], position_ids,
          (uint16_t *)model->cos_sin_cache, &model->config, num_tokens,
          model->cache_len[layer_idx], layer_idx);

      model->cache_len[layer_idx] += num_tokens;

      uint16_t *tmp = layer_input;
      layer_input = layer_output;
      layer_output = tmp;
    }

    uint16_t *final_hidden = layer_input;

    if (model->config.num_hidden_layers % 2 == 0) {
      free(layer_output);
    }

    rms_norm_f16(final_hidden, final_hidden, (uint16_t *)model->weights.norm,
                 model->config.rms_norm_eps, num_tokens,
                 model->config.hidden_size);

    uint16_t *all_logits_f16 = (uint16_t *)malloc(
        num_tokens * model->config.vocab_size * sizeof(uint16_t));
    if (!all_logits_f16) {
      free(hidden);
      free(position_ids);
      if (model->config.num_hidden_layers % 2 == 0) {
        free(layer_output);
      }
      return false;
    }

    gemm_f16(final_hidden, (uint16_t *)model->weights.lm_head, all_logits_f16,
             num_tokens, model->config.vocab_size, model->config.hidden_size);

    uint16_t *last_token_logits_f16 =
        all_logits_f16 + (num_tokens - 1) * model->config.vocab_size;
    f16_array_to_f32(last_token_logits_f16, logits, model->config.vocab_size);

    free(all_logits_f16);
    free(hidden);
    free(position_ids);

    if (model->config.num_hidden_layers % 2 == 1) {
      free(layer_output);
    }
  } else {
    float *hidden =
        (float *)malloc(num_tokens * model->config.hidden_size * sizeof(float));
    if (!hidden) {
      free(token_ids_i64);
      free(position_ids);
      return false;
    }

    embedding_lookup_f32(
        hidden, token_ids_i64, (float *)model->weights.embed_tokens, num_tokens,
        model->config.vocab_size, model->config.hidden_size, -1);
    free(token_ids_i64);

    float *layer_input = hidden;
    float *layer_output =
        (float *)malloc(num_tokens * model->config.hidden_size * sizeof(float));
    if (!layer_output) {
      free(hidden);
      free(position_ids);
      return false;
    }

    for (int layer_idx = 0; layer_idx < model->config.num_hidden_layers;
         layer_idx++) {
      qwen3_transformer_layer_f32(
          layer_output, layer_input, &model->weights.layers[layer_idx],
          (float *)model->key_cache[layer_idx],
          (float *)model->value_cache[layer_idx], position_ids,
          (float *)model->cos_sin_cache, &model->config, num_tokens,
          model->cache_len[layer_idx], layer_idx);

      model->cache_len[layer_idx] += num_tokens;

      float *tmp = layer_input;
      layer_input = layer_output;
      layer_output = tmp;
    }

    float *final_hidden = layer_input;

    if (model->config.num_hidden_layers % 2 == 0) {
      free(layer_output);
    }

    rms_norm_f32(final_hidden, final_hidden, (float *)model->weights.norm,
                 model->config.rms_norm_eps, num_tokens,
                 model->config.hidden_size);

    float *all_logits =
        (float *)malloc(num_tokens * model->config.vocab_size * sizeof(float));
    if (!all_logits) {
      free(hidden);
      free(position_ids);
      if (model->config.num_hidden_layers % 2 == 0) {
        free(layer_output);
      }
      return false;
    }

    gemm_f32((float *)model->weights.lm_head, final_hidden, all_logits,
             model->config.vocab_size, num_tokens, model->config.hidden_size,
             false, true);

    memcpy(logits, all_logits + (num_tokens - 1) * model->config.vocab_size,
           model->config.vocab_size * sizeof(float));

    free(all_logits);
    free(hidden);
    free(position_ids);

    if (model->config.num_hidden_layers % 2 == 1) {
      free(layer_output);
    }
  }

  return true;
}

int qwen3_generate(qwen3_model_t *model, int *output_tokens, int max_tokens,
                   const int *input_tokens, int num_input_tokens,
                   float temperature, int top_k, float top_p) {
  if (!model || !output_tokens || !input_tokens || num_input_tokens <= 0)
    return 0;

  qwen3_model_reset_cache(model);

  float *logits = (float *)malloc(model->config.vocab_size * sizeof(float));
  if (!logits)
    return 0;

  if (!qwen3_forward(model, logits, input_tokens, num_input_tokens)) {
    free(logits);
    return 0;
  }

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int num_generated = 0;
  int current_token;

  for (int i = 0; i < max_tokens; i++) {
    current_token = sampling_sample_f32(logits, model->config.vocab_size,
                                        temperature, top_k, top_p, 0.0f, &rng);
    output_tokens[i] = current_token;
    num_generated++;

    if (current_token == model->config.eos_token_id)
      break;

    int single_token[1] = {current_token};
    if (!qwen3_forward(model, logits, single_token, 1))
      break;
  }

  free(logits);
  return num_generated;
}

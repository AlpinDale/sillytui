#include "inference/model/qwen3.h"
#include "inference/kernels/embedding/embedding.h"
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/norm/layernorm.h"
#include "inference/kernels/rope/rope.h"
#include "inference/kernels/sampling/sampling.h"
#include "inference/model/transformer_layer.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #region agent log
static void debug_log(const char *location, const char *message,
                      const char *hypothesis_id, const char *data_json) {
  FILE *f = fopen("/Users/alpindale/repos/cursed-silly/.cursor/debug.log", "a");
  if (f) {
    fprintf(f,
            "{\"sessionId\":\"debug-session\",\"runId\":\"run1\","
            "\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\","
            "\"data\":%s,\"timestamp\":%ld}\n",
            hypothesis_id, location, message, data_json, (long)time(NULL));
    fclose(f);
  }
}
// #endregion

bool qwen3_model_load(qwen3_model_t *model, const char *model_dir) {
  if (!model || !model_dir)
    return false;

  memset(model, 0, sizeof(*model));

  char config_path[512];
  snprintf(config_path, sizeof(config_path), "%s/config.json", model_dir);
  if (!qwen3_config_load(&model->config, config_path)) {
    fprintf(stderr, "Failed to load config from %s\n", config_path);
    return false;
  }

  char model_path[512];
  snprintf(model_path, sizeof(model_path), "%s/model.safetensors", model_dir);
  if (!qwen3_weights_load(&model->weights, &model->config, model_path)) {
    fprintf(stderr, "Failed to load weights from %s\n", model_path);
    qwen3_config_free(&model->config);
    return false;
  }

  model->max_seq_len = model->config.max_position_embeddings;

  model->key_cache =
      (float **)calloc(model->config.num_hidden_layers, sizeof(float *));
  model->value_cache =
      (float **)calloc(model->config.num_hidden_layers, sizeof(float *));
  model->cache_len =
      (int *)calloc(model->config.num_hidden_layers, sizeof(int));

  if (!model->key_cache || !model->value_cache || !model->cache_len) {
    qwen3_model_free(model);
    return false;
  }

  int kv_cache_size = model->max_seq_len * model->config.num_key_value_heads *
                      model->config.head_dim;
  for (int i = 0; i < model->config.num_hidden_layers; i++) {
    model->key_cache[i] = (float *)calloc(kv_cache_size, sizeof(float));
    model->value_cache[i] = (float *)calloc(kv_cache_size, sizeof(float));
    if (!model->key_cache[i] || !model->value_cache[i]) {
      qwen3_model_free(model);
      return false;
    }
  }

  int rot_dim = model->config.head_dim;
  int cache_size = model->max_seq_len * rot_dim * 2;
  model->cos_sin_cache = (float *)malloc(cache_size * sizeof(float));
  if (!model->cos_sin_cache) {
    qwen3_model_free(model);
    return false;
  }
  rope_compute_cos_sin_cache_f32(model->cos_sin_cache, model->max_seq_len,
                                 rot_dim, model->config.rope_theta);

  model->temp_buffer_size = model->config.hidden_size * 8;
  model->temp_buffer = (float *)malloc(model->temp_buffer_size * sizeof(float));
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

  float *hidden =
      (float *)malloc(num_tokens * model->config.hidden_size * sizeof(float));
  if (!hidden)
    return false;

  int64_t *token_ids_i64 = (int64_t *)malloc(num_tokens * sizeof(int64_t));
  if (!token_ids_i64) {
    free(hidden);
    return false;
  }
  for (int i = 0; i < num_tokens; i++) {
    token_ids_i64[i] = token_ids[i];
  }
  embedding_lookup_f32(hidden, token_ids_i64, model->weights.embed_tokens,
                       num_tokens, model->config.vocab_size,
                       model->config.hidden_size, -1);
  free(token_ids_i64);
  // #region agent log
  char data_buf[512];
  snprintf(data_buf, sizeof(data_buf),
           "{\"num_tokens\":%d,\"hidden_size\":%d,\"embed_sum\":%.6f,\"embed_"
           "first3\":[%.6f,%.6f,%.6f]}",
           num_tokens, model->config.hidden_size,
           hidden[0] + hidden[1] + hidden[2], hidden[0], hidden[1], hidden[2]);
  debug_log("qwen3.c:136", "After embedding lookup", "A", data_buf);
  // #endregion

  int64_t *position_ids = (int64_t *)malloc(num_tokens * sizeof(int64_t));
  if (!position_ids) {
    free(hidden);
    return false;
  }

  int start_pos = 0;
  if (model->cache_len && model->cache_len[0] > 0) {
    start_pos = model->cache_len[0];
  }
  for (int i = 0; i < num_tokens; i++) {
    position_ids[i] = start_pos + i;
  }
  // #region agent log
  char pos_buf[256];
  snprintf(pos_buf, sizeof(pos_buf),
           "{\"start_pos\":%d,\"position_ids\":[%ld,%ld,%ld]}", start_pos,
           (long)position_ids[0], num_tokens > 1 ? (long)position_ids[1] : 0,
           num_tokens > 2 ? (long)position_ids[2] : 0);
  debug_log("qwen3.c:151", "Position IDs computed", "D", pos_buf);
  // #endregion

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
    // #region agent log
    if (layer_idx == 0) {
      char layer_buf[256];
      snprintf(layer_buf, sizeof(layer_buf),
               "{\"layer_idx\":%d,\"cache_len\":%d,\"input_sum\":%.6f,\"input_"
               "first3\":[%.6f,%.6f,%.6f]}",
               layer_idx, model->cache_len[layer_idx],
               layer_input[0] + layer_input[1] + layer_input[2], layer_input[0],
               layer_input[1], layer_input[2]);
      debug_log("qwen3.c:162", "Before transformer layer", "A", layer_buf);
    }
    // #endregion

    qwen3_transformer_layer_f32(
        layer_output, layer_input, &model->weights.layers[layer_idx],
        model->key_cache[layer_idx], model->value_cache[layer_idx],
        position_ids, model->cos_sin_cache, &model->config, num_tokens,
        model->cache_len[layer_idx], layer_idx);

    model->cache_len[layer_idx] += num_tokens;

    // #region agent log
    if (layer_idx == 0) {
      char out_buf[256];
      snprintf(out_buf, sizeof(out_buf),
               "{\"layer_idx\":%d,\"output_sum\":%.6f,\"output_first3\":[%.6f,%"
               ".6f,%.6f]}",
               layer_idx, layer_output[0] + layer_output[1] + layer_output[2],
               layer_output[0], layer_output[1], layer_output[2]);
      debug_log("qwen3.c:174", "After transformer layer", "A", out_buf);
    }
    // #endregion

    float *tmp = layer_input;
    layer_input = layer_output;
    layer_output = tmp;
  }

  float *final_hidden = layer_input;

  if (model->config.num_hidden_layers % 2 == 0) {
    free(layer_output);
  }

  rms_norm_f32(final_hidden, final_hidden, model->weights.norm,
               model->config.rms_norm_eps, num_tokens,
               model->config.hidden_size);

  // #region agent log
  if (num_tokens == 1) {
    float manual_lm0 = 0.0f;
    for (int j = 0; j < model->config.hidden_size; j++) {
      manual_lm0 += final_hidden[j] * model->weights.lm_head[j];
    }
    char pre_lm_buf[256];
    snprintf(pre_lm_buf, sizeof(pre_lm_buf),
             "{\"final_hidden_first3\":[%.6f,%.6f,%.6f],\"lm_head_first3\":[%."
             "6f,%.6f,%.6f],\"manual_lm0\":%.6f}",
             final_hidden[0], final_hidden[1], final_hidden[2],
             model->weights.lm_head[0], model->weights.lm_head[1],
             model->weights.lm_head[2], manual_lm0);
    debug_log("qwen3.c:228", "Before LM head projection", "I", pre_lm_buf);
  }
  // #endregion

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

  gemm_f32(final_hidden, model->weights.lm_head, all_logits, num_tokens,
           model->config.vocab_size, model->config.hidden_size, false, true);

  float *last_token_logits =
      all_logits + (num_tokens - 1) * model->config.vocab_size;
  memcpy(logits, last_token_logits, model->config.vocab_size * sizeof(float));

  // #region agent log
  if (num_tokens == 1) {
    char logits_buf[256];
    float max_logit = -INFINITY;
    int max_logit_idx = -1;
    for (int i = 0; i < model->config.vocab_size && i < 1000; ++i) {
      if (logits[i] > max_logit) {
        max_logit = logits[i];
        max_logit_idx = i;
      }
    }
    snprintf(logits_buf, sizeof(logits_buf),
             "{\"max_logit\":%.6f,\"max_logit_idx\":%d,\"first5_logits\":[%.6f,"
             "%.6f,%.6f,%.6f,%.6f],\"actual_lm0\":%.6f}",
             max_logit, max_logit_idx, logits[0], logits[1], logits[2],
             logits[3], logits[4], logits[0]);
    debug_log("qwen3.c:242", "Final logits", "I", logits_buf);
  }
  // #endregion

  // #region agent log
  float max_logit = logits[0];
  int max_idx = 0;
  for (int i = 1; i < model->config.vocab_size && i < 10; i++) {
    if (logits[i] > max_logit) {
      max_logit = logits[i];
      max_idx = i;
    }
  }
  char logit_buf[512];
  snprintf(logit_buf, sizeof(logit_buf),
           "{\"max_logit\":%.6f,\"max_idx\":%d,\"logits_first5\":[%.6f,%.6f,%."
           "6f,%.6f,%.6f]}",
           max_logit, max_idx, logits[0], logits[1], logits[2], logits[3],
           logits[4]);
  debug_log("qwen3.c:229", "Final logits", "A", logit_buf);
  // #endregion

  free(all_logits);
  free(hidden);
  free(position_ids);

  if (model->config.num_hidden_layers % 2 == 1) {
    free(layer_output);
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

  // #region agent log
  char gen_init_buf[256];
  snprintf(
      gen_init_buf, sizeof(gen_init_buf),
      "{\"num_input_tokens\":%d,\"input_tokens\":[%d],\"cache_reset\":true}",
      num_input_tokens, input_tokens[0]);
  debug_log("qwen3.c:291", "Generation start", "J", gen_init_buf);
  // #endregion

  if (!qwen3_forward(model, logits, input_tokens, num_input_tokens)) {
    free(logits);
    return 0;
  }

  // #region agent log
  float max_logit = logits[0];
  int max_idx = 0;
  for (int i = 1; i < model->config.vocab_size && i < 1000; i++) {
    if (logits[i] > max_logit) {
      max_logit = logits[i];
      max_idx = i;
    }
  }
  char first_logits_buf[256];
  snprintf(
      first_logits_buf, sizeof(first_logits_buf),
      "{\"step\":0,\"max_logit\":%.6f,\"max_idx\":%d,\"cache_len_after\":%d}",
      max_logit, max_idx, model->cache_len[0]);
  debug_log("qwen3.c:307", "After initial forward", "J", first_logits_buf);
  // #endregion

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int num_generated = 0;
  int current_token;

  for (int i = 0; i < max_tokens; i++) {
    current_token = sampling_sample_f32(logits, model->config.vocab_size,
                                        temperature, top_k, top_p, 0.0f, &rng);
    output_tokens[i] = current_token;
    num_generated++;

    // #region agent log
    char sampled_buf[256];
    snprintf(sampled_buf, sizeof(sampled_buf),
             "{\"step\":%d,\"sampled_token\":%d,\"cache_len_before\":%d}",
             i + 1, current_token, model->cache_len[0]);
    debug_log("qwen3.c:325", "Token sampled", "J", sampled_buf);
    // #endregion

    if (current_token == model->config.eos_token_id)
      break;

    int single_token[1] = {current_token};
    if (!qwen3_forward(model, logits, single_token, 1))
      break;

    // #region agent log
    max_logit = logits[0];
    max_idx = 0;
    for (int j = 1; j < model->config.vocab_size && j < 1000; j++) {
      if (logits[j] > max_logit) {
        max_logit = logits[j];
        max_idx = j;
      }
    }
    char step_logits_buf[256];
    snprintf(step_logits_buf, sizeof(step_logits_buf),
             "{\"step\":%d,\"max_logit\":%.6f,\"max_idx\":%d,\"cache_len_"
             "after\":%d}",
             i + 1, max_logit, max_idx, model->cache_len[0]);
    debug_log("qwen3.c:340", "After forward step", "J", step_logits_buf);
    // #endregion
  }

  free(logits);
  return num_generated;
}

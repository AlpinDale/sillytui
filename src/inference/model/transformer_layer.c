#include "inference/model/transformer_layer.h"
#include "inference/kernels/norm/layernorm.h"
#include "inference/model/attention_layer.h"
#include "inference/model/ffn.h"
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
            hypothesis_id, location, message, data_json, time(NULL));
    fclose(f);
  }
}
// #endregion

void qwen3_transformer_layer_f32(float *output, const float *input,
                                 const qwen3_layer_weights_t *weights,
                                 float *key_cache, float *value_cache,
                                 const int64_t *position_ids,
                                 const float *cos_sin_cache,
                                 const qwen3_config_t *config, int seq_len,
                                 int cache_len, int layer_idx) {
  float *residual =
      (float *)malloc(seq_len * config->hidden_size * sizeof(float));
  float *attn_out =
      (float *)malloc(seq_len * config->hidden_size * sizeof(float));
  float *norm_out =
      (float *)malloc(seq_len * config->hidden_size * sizeof(float));

  if (!residual || !attn_out || !norm_out) {
    if (residual)
      free(residual);
    if (attn_out)
      free(attn_out);
    if (norm_out)
      free(norm_out);
    return;
  }

  memcpy(residual, input, seq_len * config->hidden_size * sizeof(float));

  // #region agent log
  if (seq_len == 1 && cache_len == 0 && layer_idx == 0) {
    char layer_buf[256];
    snprintf(layer_buf, sizeof(layer_buf),
             "{\"layer_idx\":%d,\"input_first3\":[%.6f,%.6f,%.6f],\"residual_"
             "first3\":[%.6f,%.6f,%.6f]}",
             layer_idx, input[0], input[1], input[2], residual[0], residual[1],
             residual[2]);
    debug_log("transformer_layer.c:27", "Before attention", "F", layer_buf);
  }
  // #endregion

  rms_norm_f32(norm_out, input, weights->attn_norm, config->rms_norm_eps,
               seq_len, config->hidden_size);

  qwen3_attention_layer_f32(
      attn_out, norm_out, weights->q_proj, weights->k_proj, weights->v_proj,
      weights->o_proj, weights->q_norm, weights->k_norm, key_cache, value_cache,
      position_ids, cos_sin_cache, seq_len, cache_len, config->hidden_size,
      config->num_attention_heads, config->num_key_value_heads,
      config->head_dim, config->rope_theta, config->max_position_embeddings);

  // #region agent log
  if (seq_len == 1 && cache_len == 0 && layer_idx == 0) {
    char attn_res_buf[256];
    snprintf(attn_res_buf, sizeof(attn_res_buf),
             "{\"attn_out_first3\":[%.6f,%.6f,%.6f],\"residual_before_add_"
             "first3\":[%.6f,%.6f,%.6f]}",
             attn_out[0], attn_out[1], attn_out[2], residual[0], residual[1],
             residual[2]);
    debug_log("transformer_layer.c:39", "Before residual add (attention)", "F",
              attn_res_buf);
  }
  // #endregion

  for (int i = 0; i < seq_len * config->hidden_size; i++) {
    residual[i] += attn_out[i];
  }

  // #region agent log
  if (seq_len == 1 && cache_len == 0 && layer_idx == 0) {
    char after_attn_res_buf[256];
    snprintf(after_attn_res_buf, sizeof(after_attn_res_buf),
             "{\"residual_after_attn_add_first3\":[%.6f,%.6f,%.6f]}",
             residual[0], residual[1], residual[2]);
    debug_log("transformer_layer.c:42", "After residual add (attention)", "F",
              after_attn_res_buf);
  }
  // #endregion

  rms_norm_f32(norm_out, residual, weights->ffn_norm, config->rms_norm_eps,
               seq_len, config->hidden_size);

  // #region agent log
  if (seq_len == 1 && cache_len == 0 && layer_idx == 0) {
    char ffn_input_buf[256];
    snprintf(ffn_input_buf, sizeof(ffn_input_buf),
             "{\"ffn_input_first3\":[%.6f,%.6f,%.6f]}", norm_out[0],
             norm_out[1], norm_out[2]);
    debug_log("transformer_layer.c:46", "Before FFN", "G", ffn_input_buf);
  }
  // #endregion

  qwen3_ffn_f32(output, norm_out, weights->gate_proj, weights->up_proj,
                weights->down_proj, seq_len, config->hidden_size,
                config->intermediate_size);

  // #region agent log
  if (seq_len == 1 && cache_len == 0 && layer_idx == 0) {
    char ffn_output_buf[256];
    snprintf(ffn_output_buf, sizeof(ffn_output_buf),
             "{\"ffn_output_first3\":[%.6f,%.6f,%.6f],\"residual_before_ffn_"
             "add_first3\":[%.6f,%.6f,%.6f]}",
             output[0], output[1], output[2], residual[0], residual[1],
             residual[2]);
    debug_log("transformer_layer.c:50", "Before residual add (FFN)", "F",
              ffn_output_buf);
  }
  // #endregion

  for (int i = 0; i < seq_len * config->hidden_size; i++) {
    output[i] += residual[i];
  }

  // #region agent log
  if (seq_len == 1 && cache_len == 0 && layer_idx == 0) {
    char final_output_buf[256];
    snprintf(final_output_buf, sizeof(final_output_buf),
             "{\"layer_output_first3\":[%.6f,%.6f,%.6f]}", output[0], output[1],
             output[2]);
    debug_log("transformer_layer.c:54", "After residual add (FFN)", "F",
              final_output_buf);
  }
  // #endregion

  free(residual);
  free(attn_out);
  free(norm_out);
}

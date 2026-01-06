#include "inference/model/transformer_layer.h"
#include "inference/kernels/norm/layernorm.h"
#include "inference/model/attention_layer.h"
#include "inference/model/ffn.h"
#include <stdlib.h>
#include <string.h>

void qwen3_transformer_layer_f32(float *output, const float *input,
                                 const qwen3_layer_weights_t *weights,
                                 float *key_cache, float *value_cache,
                                 const int64_t *position_ids,
                                 const float *cos_sin_cache,
                                 const qwen3_config_t *config, int seq_len,
                                 int cache_len, int layer_idx) {
  (void)layer_idx;
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

  rms_norm_f32(norm_out, input, weights->attn_norm, config->rms_norm_eps,
               seq_len, config->hidden_size);

  qwen3_attention_layer_f32(
      attn_out, norm_out, weights->q_proj, weights->k_proj, weights->v_proj,
      weights->o_proj, weights->q_norm, weights->k_norm, key_cache, value_cache,
      position_ids, cos_sin_cache, seq_len, cache_len, config->hidden_size,
      config->num_attention_heads, config->num_key_value_heads,
      config->head_dim, config->rope_theta, config->max_position_embeddings);

  for (int i = 0; i < seq_len * config->hidden_size; i++) {
    residual[i] += attn_out[i];
  }

  rms_norm_f32(norm_out, residual, weights->ffn_norm, config->rms_norm_eps,
               seq_len, config->hidden_size);

  qwen3_ffn_f32(output, norm_out, weights->gate_proj, weights->up_proj,
                weights->down_proj, seq_len, config->hidden_size,
                config->intermediate_size);

  for (int i = 0; i < seq_len * config->hidden_size; i++) {
    output[i] += residual[i];
  }

  free(residual);
  free(attn_out);
  free(norm_out);
}

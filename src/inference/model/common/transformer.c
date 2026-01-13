#include "transformer.h"
#include "inference/core/dtype.h"
#include "inference/kernels/norm/layernorm.h"
#include <stdlib.h>
#include <string.h>

void transformer_layer_forward_f32(float *output, const float *input,
                                   const transformer_layer_t *layer,
                                   float *key_cache, float *value_cache,
                                   const int64_t *position_ids,
                                   const float *cos_sin_cache, int seq_len,
                                   int cache_len, int hidden_size) {
  if (!output || !input || !layer)
    return;

  float *normed = (float *)malloc(seq_len * hidden_size * sizeof(float));
  float *attn_out = (float *)malloc(seq_len * hidden_size * sizeof(float));
  float *residual = (float *)malloc(seq_len * hidden_size * sizeof(float));
  if (!normed || !attn_out || !residual) {
    free(normed);
    free(attn_out);
    free(residual);
    return;
  }

  const float *norm_w = tensor_data_f32_const(layer->input_norm);
  (void)layer->norm_type;
  rms_norm_f32(normed, input, norm_w, layer->norm_eps, seq_len, hidden_size);

  attention_forward_f32(attn_out, normed, &layer->attention, key_cache,
                        value_cache, position_ids, cos_sin_cache, seq_len,
                        cache_len);

  for (int i = 0; i < seq_len * hidden_size; i++) {
    residual[i] = input[i] + attn_out[i];
  }

  const float *post_norm_w = tensor_data_f32_const(layer->post_attn_norm);
  rms_norm_f32(normed, residual, post_norm_w, layer->norm_eps, seq_len,
               hidden_size);

  ffn_forward_f32(attn_out, normed, &layer->ffn, seq_len);

  for (int i = 0; i < seq_len * hidden_size; i++) {
    output[i] = residual[i] + attn_out[i];
  }

  free(normed);
  free(attn_out);
  free(residual);
}

void transformer_layer_forward_f16(uint16_t *output, const uint16_t *input,
                                   const transformer_layer_t *layer,
                                   uint16_t *key_cache, uint16_t *value_cache,
                                   const int64_t *position_ids,
                                   const uint16_t *cos_sin_cache, int seq_len,
                                   int cache_len, int hidden_size) {
  if (!output || !input || !layer)
    return;

  uint16_t *normed =
      (uint16_t *)malloc(seq_len * hidden_size * sizeof(uint16_t));
  uint16_t *attn_out =
      (uint16_t *)malloc(seq_len * hidden_size * sizeof(uint16_t));
  uint16_t *residual =
      (uint16_t *)malloc(seq_len * hidden_size * sizeof(uint16_t));
  if (!normed || !attn_out || !residual) {
    free(normed);
    free(attn_out);
    free(residual);
    return;
  }

  const uint16_t *norm_w = tensor_data_f16_const(layer->input_norm);
  rms_norm_f16(normed, input, norm_w, layer->norm_eps, seq_len, hidden_size);

  attention_forward_f16(attn_out, normed, &layer->attention, key_cache,
                        value_cache, position_ids, cos_sin_cache, seq_len,
                        cache_len);

  /* Residual connection: residual = input + attn_out */
  for (int i = 0; i < seq_len * hidden_size; i++) {
    float a = f16_to_f32(input[i]);
    float b = f16_to_f32(attn_out[i]);
    residual[i] = f32_to_f16(a + b);
  }

  const uint16_t *post_norm_w = tensor_data_f16_const(layer->post_attn_norm);
  rms_norm_f16(normed, residual, post_norm_w, layer->norm_eps, seq_len,
               hidden_size);

  ffn_forward_f16(attn_out, normed, &layer->ffn, seq_len);

  /* Residual connection: output = residual + ffn_out */
  for (int i = 0; i < seq_len * hidden_size; i++) {
    float a = f16_to_f32(residual[i]);
    float b = f16_to_f32(attn_out[i]);
    output[i] = f32_to_f16(a + b);
  }

  free(normed);
  free(attn_out);
  free(residual);
}

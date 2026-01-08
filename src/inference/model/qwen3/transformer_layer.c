#include "transformer_layer.h"
#include "attention_layer.h"
#include "ffn.h"
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/norm/layernorm.h"
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

static inline float fp16_to_float_tl(uint16_t fp16) {
  uint32_t sign = (fp16 & 0x8000) << 16;
  uint32_t exp = (fp16 >> 10) & 0x1F;
  uint32_t mant = fp16 & 0x3FF;

  uint32_t f32_bits;
  if (exp == 0) {
    if (mant == 0) {
      f32_bits = sign;
    } else {
      int e = -14;
      while ((mant & 0x400) == 0) {
        mant <<= 1;
        e--;
      }
      mant &= 0x3FF;
      f32_bits = sign | (((uint32_t)(e + 127)) << 23) | (mant << 13);
    }
  } else if (exp == 31) {
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    f32_bits = sign | (((uint32_t)(exp - 15 + 127)) << 23) | (mant << 13);
  }

  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_fp16_tl(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t sign = bits & 0x80000000;
  uint32_t exp = (bits >> 23) & 0xFF;
  uint32_t mant = bits & 0x7FFFFF;

  if (exp == 0xFF) {
    if (mant != 0)
      return 0x7E00;
    return (uint16_t)((sign >> 16) | 0x7C00);
  }
  if (exp == 0 && mant == 0)
    return (uint16_t)(sign >> 16);

  int32_t new_exp = (int32_t)exp - 127 + 15;
  if (new_exp <= 0)
    return (uint16_t)(sign >> 16);
  if (new_exp >= 31)
    return (uint16_t)((sign >> 16) | 0x7C00);

  uint16_t fp16_exp = (uint16_t)(new_exp << 10);
  uint16_t fp16_mant = (uint16_t)(mant >> 13);
  return (uint16_t)((sign >> 16) | fp16_exp | fp16_mant);
}

void qwen3_transformer_layer_f16(uint16_t *output, const uint16_t *input,
                                 const qwen3_layer_weights_t *weights,
                                 uint16_t *key_cache, uint16_t *value_cache,
                                 const int64_t *position_ids,
                                 const uint16_t *cos_sin_cache,
                                 const qwen3_config_t *config, int seq_len,
                                 int cache_len, int layer_idx) {
  (void)layer_idx;
  uint16_t *residual =
      (uint16_t *)malloc(seq_len * config->hidden_size * sizeof(uint16_t));
  uint16_t *attn_out =
      (uint16_t *)malloc(seq_len * config->hidden_size * sizeof(uint16_t));
  uint16_t *norm_out =
      (uint16_t *)malloc(seq_len * config->hidden_size * sizeof(uint16_t));

  if (!residual || !attn_out || !norm_out) {
    if (residual)
      free(residual);
    if (attn_out)
      free(attn_out);
    if (norm_out)
      free(norm_out);
    return;
  }

  memcpy(residual, input, seq_len * config->hidden_size * sizeof(uint16_t));

  rms_norm_f16(norm_out, input, weights->attn_norm, config->rms_norm_eps,
               seq_len, config->hidden_size);

  qwen3_attention_layer_f16(
      attn_out, norm_out, weights->q_proj, weights->k_proj, weights->v_proj,
      weights->o_proj, weights->q_norm, weights->k_norm, key_cache, value_cache,
      position_ids, cos_sin_cache, seq_len, cache_len, config->hidden_size,
      config->num_attention_heads, config->num_key_value_heads,
      config->head_dim, config->rope_theta, config->max_position_embeddings);

  fused_add_rms_norm_f16(norm_out, attn_out, residual, weights->ffn_norm,
                         config->rms_norm_eps, seq_len, config->hidden_size);

  qwen3_ffn_f16(attn_out, norm_out, weights->gate_proj, weights->up_proj,
                weights->down_proj, seq_len, config->hidden_size,
                config->intermediate_size);

  int size = seq_len * config->hidden_size;
  for (int i = 0; i < size; i++) {
    float o = fp16_to_float_tl(attn_out[i]);
    float r = fp16_to_float_tl(residual[i]);
    output[i] = float_to_fp16_tl(o + r);
  }

  free(residual);
  free(attn_out);
  free(norm_out);
}

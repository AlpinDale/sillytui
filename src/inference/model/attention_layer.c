#include "inference/model/attention_layer.h"
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/kv_cache/kv_cache.h"
#include "inference/kernels/norm/layernorm.h"
#include "inference/kernels/rope/rope.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void qwen3_attention_layer_f32(
    float *output, const float *input, const float *q_proj, const float *k_proj,
    const float *v_proj, const float *o_proj, const float *q_norm,
    const float *k_norm, float *key_cache, float *value_cache,
    const int64_t *position_ids, const float *cos_sin_cache, int seq_len,
    int cache_len, int hidden_size, int num_heads, int num_kv_heads,
    int head_dim, float rope_theta, int max_position) {
  (void)rope_theta;
  (void)max_position;
  int q_dim = num_heads * head_dim;
  int kv_dim = num_kv_heads * head_dim;

  float *q = (float *)malloc(seq_len * q_dim * sizeof(float));
  float *k = (float *)malloc(seq_len * kv_dim * sizeof(float));
  float *v = (float *)malloc(seq_len * kv_dim * sizeof(float));
  if (!q || !k || !v) {
    if (q)
      free(q);
    if (k)
      free(k);
    if (v)
      free(v);
    return;
  }

  gemm_f32(input, q_proj, q, seq_len, q_dim, hidden_size, false, true);
  gemm_f32(input, k_proj, k, seq_len, kv_dim, hidden_size, false, true);
  gemm_f32(input, v_proj, v, seq_len, kv_dim, hidden_size, false, true);

  for (int i = 0; i < seq_len; i++) {
    for (int h = 0; h < num_heads; h++) {
      float *q_head = q + i * q_dim + h * head_dim;
      rms_norm_f32(q_head, q_head, q_norm, 1e-6f, 1, head_dim);
    }
  }

  for (int i = 0; i < seq_len; i++) {
    for (int h = 0; h < num_kv_heads; h++) {
      float *k_head = k + i * kv_dim + h * head_dim;
      rms_norm_f32(k_head, k_head, k_norm, 1e-6f, 1, head_dim);
    }
  }

  rope_f32(position_ids, q, k, cos_sin_cache, seq_len, num_heads, num_kv_heads,
           head_dim, head_dim, true);

  kv_cache_append_f32(key_cache, value_cache, k, v, cache_len, seq_len,
                      num_kv_heads, head_dim);

  int total_seq_len = cache_len + seq_len;
  float scale = 1.0f / sqrtf((float)head_dim);
  int heads_per_kv = num_heads / num_kv_heads;

  float *attn_out = (float *)malloc(seq_len * q_dim * sizeof(float));
  if (!attn_out) {
    free(q);
    free(k);
    free(v);
    return;
  }

  memset(attn_out, 0, seq_len * q_dim * sizeof(float));

  for (int i = 0; i < seq_len; i++) {
    int64_t query_abs_pos = position_ids[i];
    for (int h = 0; h < num_heads; h++) {
      int kv_head = h / heads_per_kv;
      float *q_head = q + i * q_dim + h * head_dim;
      float *out_head = attn_out + i * q_dim + h * head_dim;

      memset(out_head, 0, head_dim * sizeof(float));
      float M = -1e9f;
      float sum = 0.0f;

      for (int pos = 0; pos <= query_abs_pos && pos < total_seq_len; pos++) {
        float *k_pos =
            key_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;
        float *v_pos =
            value_cache + pos * num_kv_heads * head_dim + kv_head * head_dim;

        float score = 0.0f;
        for (int d = 0; d < head_dim; d++) {
          score += q_head[d] * k_pos[d];
        }
        score *= scale;

        float M_new = (score > M) ? score : M;
        float alpha = expf(M - M_new);

        for (int d = 0; d < head_dim; d++) {
          out_head[d] = out_head[d] * alpha + v_pos[d] * expf(score - M_new);
        }
        sum = sum * alpha + expf(score - M_new);
        M = M_new;
      }

      if (sum > 0.0f) {
        for (int d = 0; d < head_dim; d++) {
          out_head[d] /= sum;
        }
      }
    }
  }

  gemm_f32(attn_out, o_proj, output, seq_len, hidden_size, q_dim, false, true);

  free(q);
  free(k);
  free(v);
  free(attn_out);
}

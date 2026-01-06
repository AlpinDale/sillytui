#include "inference/model/attention_layer.h"
#include "inference/kernels/attention/attention.h"
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/kv_cache/kv_cache.h"
#include "inference/kernels/norm/layernorm.h"
#include "inference/kernels/rope/rope.h"
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

  // #region agent log
  if (seq_len == 1 && cache_len == 0) {
    char pre_norm_buf[256];
    snprintf(pre_norm_buf, sizeof(pre_norm_buf),
             "{\"q_before_norm_first3\":[%.6f,%.6f,%.6f],\"k_before_norm_"
             "first3\":[%.6f,%.6f,%.6f],\"q_norm_first3\":[%.6f,%.6f,%.6f],\"k_"
             "norm_first3\":[%.6f,%.6f,%.6f]}",
             q[0], q[1], q[2], k[0], k[1], k[2], q_norm[0], q_norm[1],
             q_norm[2], k_norm[0], k_norm[1], k_norm[2]);
    debug_log("attention_layer.c:51", "Before Q/K normalization", "E",
              pre_norm_buf);
  }
  // #endregion

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

  // #region agent log
  if (seq_len == 1 && cache_len == 0) {
    char post_norm_buf[256];
    snprintf(post_norm_buf, sizeof(post_norm_buf),
             "{\"q_after_norm_first3\":[%.6f,%.6f,%.6f],\"k_after_norm_"
             "first3\":[%.6f,%.6f,%.6f]}",
             q[0], q[1], q[2], k[0], k[1], k[2]);
    debug_log("attention_layer.c:65", "After Q/K normalization", "E",
              post_norm_buf);
  }
  // #endregion

  rope_f32(position_ids, q, k, cos_sin_cache, seq_len, num_heads, num_kv_heads,
           head_dim, head_dim, true);

  // #region agent log
  if (seq_len == 1 && cache_len == 0) {
    char rope_buf[256];
    snprintf(rope_buf, sizeof(rope_buf),
             "{\"pos_id\":%ld,\"q_after_rope_first3\":[%.6f,%.6f,%.6f],\"k_"
             "after_rope_first3\":[%.6f,%.6f,%.6f]}",
             (long)position_ids[0], q[0], q[1], q[2], k[0], k[1], k[2]);
    debug_log("attention_layer.c:53", "After RoPE", "D", rope_buf);
  }
  // #endregion

  kv_cache_append_f32(key_cache, value_cache, k, v, cache_len, seq_len,
                      num_kv_heads, head_dim);

  // #region agent log
  if (seq_len == 1 && cache_len == 0) {
    char cache_buf[256];
    snprintf(cache_buf, sizeof(cache_buf),
             "{\"cache_len\":%d,\"total_seq_len\":%d,\"key_cache_first3\":[%."
             "6f,%.6f,%.6f],\"value_cache_first3\":[%.6f,%.6f,%.6f]}",
             cache_len, cache_len + seq_len, key_cache[0], key_cache[1],
             key_cache[2], value_cache[0], value_cache[1], value_cache[2]);
    debug_log("attention_layer.c:56", "After KV cache append", "C", cache_buf);
  }
  // #endregion

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

        // #region agent log
        if (i == 0 && h == 0 && pos == 0 && seq_len == 1 && cache_len == 0) {
          char score_buf[256];
          snprintf(score_buf, sizeof(score_buf),
                   "{\"query_pos\":%ld,\"kv_pos\":%d,\"score\":%.6f,\"scale\":%"
                   ".6f,\"q_head_first3\":[%.6f,%.6f,%.6f],\"k_pos_first3\":[%."
                   "6f,%.6f,%.6f]}",
                   (long)query_abs_pos, pos, score, scale, q_head[0], q_head[1],
                   q_head[2], k_pos[0], k_pos[1], k_pos[2]);
          debug_log("attention_layer.c:87", "Attention score computed", "B",
                    score_buf);
        }
        // #endregion

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

      // #region agent log
      if (i == 0 && h == 0 && seq_len == 1 && cache_len == 0) {
        char attn_buf[256];
        snprintf(attn_buf, sizeof(attn_buf),
                 "{\"out_head_sum\":%.6f,\"out_head_first3\":[%.6f,%.6f,%.6f],"
                 "\"sum\":%.6f,\"M\":%.6f}",
                 out_head[0] + out_head[1] + out_head[2], out_head[0],
                 out_head[1], out_head[2], sum, M);
        debug_log("attention_layer.c:107", "After attention computation", "B",
                  attn_buf);
      }
      // #endregion
    }
  }

  // #region agent log
  if (seq_len == 1 && cache_len == 0) {
    float manual_o0 = 0.0f;
    for (int j = 0; j < q_dim; j++) {
      manual_o0 += attn_out[j] * o_proj[j];
    }
    char pre_o_proj_buf[256];
    snprintf(pre_o_proj_buf, sizeof(pre_o_proj_buf),
             "{\"attn_out_first3\":[%.6f,%.6f,%.6f],\"o_proj_first3\":[%.6f,%."
             "6f,%.6f],\"manual_o0\":%.6f}",
             attn_out[0], attn_out[1], attn_out[2], o_proj[0], o_proj[1],
             o_proj[2], manual_o0);
    debug_log("attention_layer.c:180", "Before output projection", "H",
              pre_o_proj_buf);
  }
  // #endregion

  gemm_f32(attn_out, o_proj, output, seq_len, hidden_size, q_dim, false, true);

  // #region agent log
  if (seq_len == 1 && cache_len == 0) {
    char out_buf[256];
    snprintf(out_buf, sizeof(out_buf),
             "{\"output_first3\":[%.6f,%.6f,%.6f],\"actual_o0\":%.6f}",
             output[0], output[1], output[2], output[0]);
    debug_log("attention_layer.c:193", "After output projection", "H", out_buf);
  }
  // #endregion

  free(q);
  free(k);
  free(v);
  free(attn_out);
}

/*
 * Flash Attention - Memory-efficient Attention Implementation
 *
 * Based on "FlashAttention: Fast and Memory-Efficient Exact Attention"
 * https://arxiv.org/abs/2205.14135
 */

#include "inference/kernels/attention/attention.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define HAS_NEON 1
void flash_attention_f32_neon(float *output, const float *query,
                              const float *key, const float *value,
                              int seq_len_q, int seq_len_kv, int head_dim,
                              float scale, const float *mask);
void flash_attention_bf16_neon(uint16_t *output, const uint16_t *query,
                               const uint16_t *key, const uint16_t *value,
                               int seq_len_q, int seq_len_kv, int head_dim,
                               float scale, const float *mask);
void flash_attention_f16_neon(uint16_t *output, const uint16_t *query,
                              const uint16_t *key, const uint16_t *value,
                              int seq_len_q, int seq_len_kv, int head_dim,
                              float scale, const float *mask);
#else
#define HAS_NEON 0
#endif

static int g_num_threads = 0;

static int get_cpu_count(void) {
  int count = (int)sysconf(_SC_NPROCESSORS_ONLN);
  return count > 0 ? count : 1;
}

void attention_set_num_threads(int num_threads) {
  if (num_threads <= 0) {
    g_num_threads = get_cpu_count();
  } else {
    g_num_threads = num_threads;
  }
}

int attention_get_num_threads(void) {
  if (g_num_threads == 0) {
    g_num_threads = get_cpu_count();
  }
  return g_num_threads;
}

static inline float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_bf16(float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));
  return (uint16_t)(bits >> 16);
}

static inline float fp16_to_float(uint16_t fp16) {
  uint32_t sign = (fp16 & 0x8000) << 16;
  uint32_t exp = (fp16 >> 10) & 0x1F;
  uint32_t mant = fp16 & 0x3FF;
  uint32_t f32_bits;

  if (exp == 0) {
    if (mant == 0) {
      f32_bits = sign;
    } else {
      exp = 1;
      while ((mant & 0x400) == 0) {
        mant <<= 1;
        exp--;
      }
      mant &= 0x3FF;
      f32_bits = sign | ((127 - 15 + exp) << 23) | (mant << 13);
    }
  } else if (exp == 31) {
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    f32_bits = sign | ((exp - 15 + 127) << 23) | (mant << 13);
  }

  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_fp16(float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));

  uint32_t sign = (bits >> 16) & 0x8000;
  int32_t exp = ((bits >> 23) & 0xFF) - 127 + 15;
  uint32_t mant = (bits >> 13) & 0x3FF;

  if (exp <= 0) {
    if (exp < -10) {
      return (uint16_t)sign;
    }
    mant = (mant | 0x400) >> (1 - exp);
    return (uint16_t)(sign | mant);
  } else if (exp >= 31) {
    if (exp == 128 && mant) {
      return (uint16_t)(sign | 0x7C00 | (mant >> 10));
    }
    return (uint16_t)(sign | 0x7C00);
  }

  return (uint16_t)(sign | (exp << 10) | mant);
}

static float dot_product_f32(const float *a, const float *b, int n) {
  float sum = 0.0f;
  for (int i = 0; i < n; i++) {
    sum += a[i] * b[i];
  }
  return sum;
}

static void vec_scale_f32(float *v, float scale, int n) {
  for (int i = 0; i < n; i++) {
    v[i] *= scale;
  }
}

static void vec_mad_f32(float *acc, const float *v, float scale, int n) {
  for (int i = 0; i < n; i++) {
    acc[i] += v[i] * scale;
  }
}

__attribute__((unused)) static void
flash_attention_f32_scalar(float *output, const float *query, const float *key,
                           const float *value, int seq_len_q, int seq_len_kv,
                           int head_dim, float scale, const float *mask) {
  float *acc = (float *)calloc(head_dim, sizeof(float));

  for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
    const float *q_vec = query + q_idx * head_dim;
    float *out_vec = output + q_idx * head_dim;

    float M = -__builtin_inff();
    float L = 0.0f;
    memset(acc, 0, head_dim * sizeof(float));

    for (int kv_idx = 0; kv_idx < seq_len_kv; kv_idx++) {
      const float *k_vec = key + kv_idx * head_dim;
      const float *v_vec = value + kv_idx * head_dim;

      float s = dot_product_f32(q_vec, k_vec, head_dim) * scale;

      if (mask) {
        s += mask[q_idx * seq_len_kv + kv_idx];
      }

      if (s == -__builtin_inff()) {
        continue;
      }

      float M_new = (s > M) ? s : M;
      float exp_M_diff = expf(M - M_new);

      vec_scale_f32(acc, exp_M_diff, head_dim);
      L *= exp_M_diff;

      float p = expf(s - M_new);
      vec_mad_f32(acc, v_vec, p, head_dim);
      L += p;

      M = M_new;
    }

    if (L > 0.0f) {
      float inv_L = 1.0f / L;
      for (int d = 0; d < head_dim; d++) {
        out_vec[d] = acc[d] * inv_L;
      }
    } else {
      memset(out_vec, 0, head_dim * sizeof(float));
    }
  }

  free(acc);
}

void flash_attention_f32(float *output, const float *query, const float *key,
                         const float *value, int seq_len_q, int seq_len_kv,
                         int head_dim, float scale, const float *mask) {
#if HAS_NEON
  flash_attention_f32_neon(output, query, key, value, seq_len_q, seq_len_kv,
                           head_dim, scale, mask);
#else
  flash_attention_f32_scalar(output, query, key, value, seq_len_q, seq_len_kv,
                             head_dim, scale, mask);
#endif
}

__attribute__((unused)) static void
flash_attention_bf16_scalar(uint16_t *output, const uint16_t *query,
                            const uint16_t *key, const uint16_t *value,
                            int seq_len_q, int seq_len_kv, int head_dim,
                            float scale, const float *mask) {
  float *q_f32 = (float *)malloc(head_dim * sizeof(float));
  float *k_f32 = (float *)malloc(head_dim * sizeof(float));
  float *v_f32 = (float *)malloc(head_dim * sizeof(float));
  float *acc = (float *)calloc(head_dim, sizeof(float));

  for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
    const uint16_t *q_vec = query + q_idx * head_dim;
    uint16_t *out_vec = output + q_idx * head_dim;

    for (int d = 0; d < head_dim; d++) {
      q_f32[d] = bf16_to_float(q_vec[d]);
    }

    float M = -__builtin_inff();
    float L = 0.0f;
    memset(acc, 0, head_dim * sizeof(float));

    for (int kv_idx = 0; kv_idx < seq_len_kv; kv_idx++) {
      const uint16_t *k_vec = key + kv_idx * head_dim;
      const uint16_t *v_vec = value + kv_idx * head_dim;

      for (int d = 0; d < head_dim; d++) {
        k_f32[d] = bf16_to_float(k_vec[d]);
        v_f32[d] = bf16_to_float(v_vec[d]);
      }

      float s = dot_product_f32(q_f32, k_f32, head_dim) * scale;

      if (mask) {
        s += mask[q_idx * seq_len_kv + kv_idx];
      }

      if (s == -__builtin_inff()) {
        continue;
      }

      float M_new = (s > M) ? s : M;
      float exp_M_diff = expf(M - M_new);

      vec_scale_f32(acc, exp_M_diff, head_dim);
      L *= exp_M_diff;

      float p = expf(s - M_new);
      vec_mad_f32(acc, v_f32, p, head_dim);
      L += p;

      M = M_new;
    }

    if (L > 0.0f) {
      float inv_L = 1.0f / L;
      for (int d = 0; d < head_dim; d++) {
        out_vec[d] = float_to_bf16(acc[d] * inv_L);
      }
    } else {
      memset(out_vec, 0, head_dim * sizeof(uint16_t));
    }
  }

  free(q_f32);
  free(k_f32);
  free(v_f32);
  free(acc);
}

void flash_attention_bf16(uint16_t *output, const uint16_t *query,
                          const uint16_t *key, const uint16_t *value,
                          int seq_len_q, int seq_len_kv, int head_dim,
                          float scale, const float *mask) {
#if HAS_NEON
  flash_attention_bf16_neon(output, query, key, value, seq_len_q, seq_len_kv,
                            head_dim, scale, mask);
#else
  flash_attention_bf16_scalar(output, query, key, value, seq_len_q, seq_len_kv,
                              head_dim, scale, mask);
#endif
}

__attribute__((unused)) static void
flash_attention_f16_scalar(uint16_t *output, const uint16_t *query,
                           const uint16_t *key, const uint16_t *value,
                           int seq_len_q, int seq_len_kv, int head_dim,
                           float scale, const float *mask) {
  float *q_f32 = (float *)malloc(head_dim * sizeof(float));
  float *k_f32 = (float *)malloc(head_dim * sizeof(float));
  float *v_f32 = (float *)malloc(head_dim * sizeof(float));
  float *acc = (float *)calloc(head_dim, sizeof(float));

  for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
    const uint16_t *q_vec = query + q_idx * head_dim;
    uint16_t *out_vec = output + q_idx * head_dim;

    for (int d = 0; d < head_dim; d++) {
      q_f32[d] = fp16_to_float(q_vec[d]);
    }

    float M = -__builtin_inff();
    float L = 0.0f;
    memset(acc, 0, head_dim * sizeof(float));

    for (int kv_idx = 0; kv_idx < seq_len_kv; kv_idx++) {
      const uint16_t *k_vec = key + kv_idx * head_dim;
      const uint16_t *v_vec = value + kv_idx * head_dim;

      for (int d = 0; d < head_dim; d++) {
        k_f32[d] = fp16_to_float(k_vec[d]);
        v_f32[d] = fp16_to_float(v_vec[d]);
      }

      float s = dot_product_f32(q_f32, k_f32, head_dim) * scale;

      if (mask) {
        s += mask[q_idx * seq_len_kv + kv_idx];
      }

      if (s == -__builtin_inff()) {
        continue;
      }

      float M_new = (s > M) ? s : M;
      float exp_M_diff = expf(M - M_new);

      vec_scale_f32(acc, exp_M_diff, head_dim);
      L *= exp_M_diff;

      float p = expf(s - M_new);
      vec_mad_f32(acc, v_f32, p, head_dim);
      L += p;

      M = M_new;
    }

    if (L > 0.0f) {
      float inv_L = 1.0f / L;
      for (int d = 0; d < head_dim; d++) {
        out_vec[d] = float_to_fp16(acc[d] * inv_L);
      }
    } else {
      memset(out_vec, 0, head_dim * sizeof(uint16_t));
    }
  }

  free(q_f32);
  free(k_f32);
  free(v_f32);
  free(acc);
}

void flash_attention_f16(uint16_t *output, const uint16_t *query,
                         const uint16_t *key, const uint16_t *value,
                         int seq_len_q, int seq_len_kv, int head_dim,
                         float scale, const float *mask) {
#if HAS_NEON
  flash_attention_f16_neon(output, query, key, value, seq_len_q, seq_len_kv,
                           head_dim, scale, mask);
#else
  flash_attention_f16_scalar(output, query, key, value, seq_len_q, seq_len_kv,
                             head_dim, scale, mask);
#endif
}

typedef struct {
  float *output;
  const float *query;
  const float *key;
  const float *value;
  int batch;
  int num_heads;
  int num_kv_heads;
  int seq_len_q;
  int seq_len_kv;
  int head_dim;
  float scale;
  const float *mask;
} mha_ctx_t;

static void mha_work(mha_ctx_t *ctx, int start_head, int end_head) {
  int heads_per_kv = ctx->num_heads / ctx->num_kv_heads;

  for (int b = 0; b < ctx->batch; b++) {
    for (int h = start_head; h < end_head; h++) {
      int kv_h = h / heads_per_kv;

      const float *q = ctx->query + (b * ctx->num_heads + h) * ctx->seq_len_q *
                                        ctx->head_dim;
      const float *k = ctx->key + (b * ctx->num_kv_heads + kv_h) *
                                      ctx->seq_len_kv * ctx->head_dim;
      const float *v = ctx->value + (b * ctx->num_kv_heads + kv_h) *
                                        ctx->seq_len_kv * ctx->head_dim;
      float *out = ctx->output +
                   (b * ctx->num_heads + h) * ctx->seq_len_q * ctx->head_dim;

      flash_attention_f32(out, q, k, v, ctx->seq_len_q, ctx->seq_len_kv,
                          ctx->head_dim, ctx->scale, ctx->mask);
    }
  }
}

typedef struct {
  mha_ctx_t *ctx;
  int start_head;
  int end_head;
} mha_task_t;

static void *mha_thread_fn(void *arg) {
  mha_task_t *task = (mha_task_t *)arg;
  mha_work(task->ctx, task->start_head, task->end_head);
  return NULL;
}

void flash_attention_mha_f32(float *output, const float *query,
                             const float *key, const float *value, int batch,
                             int num_heads, int num_kv_heads, int seq_len_q,
                             int seq_len_kv, int head_dim, float scale,
                             const float *mask) {
  mha_ctx_t ctx = {output,     query,     key,          value,
                   batch,      num_heads, num_kv_heads, seq_len_q,
                   seq_len_kv, head_dim,  scale,        mask};

  int num_threads = attention_get_num_threads();
  int total_heads = batch * num_heads;

  if (num_threads <= 1 || total_heads < 4) {
    mha_work(&ctx, 0, num_heads);
    return;
  }

  if (num_threads > num_heads)
    num_threads = num_heads;

  pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  mha_task_t *tasks = (mha_task_t *)malloc(num_threads * sizeof(mha_task_t));

  int heads_per_thread = (num_heads + num_threads - 1) / num_threads;

  for (int t = 0; t < num_threads; t++) {
    tasks[t].ctx = &ctx;
    tasks[t].start_head = t * heads_per_thread;
    tasks[t].end_head = tasks[t].start_head + heads_per_thread;
    if (tasks[t].end_head > num_heads)
      tasks[t].end_head = num_heads;

    if (tasks[t].start_head < num_heads) {
      pthread_create(&threads[t], NULL, mha_thread_fn, &tasks[t]);
    }
  }

  for (int t = 0; t < num_threads; t++) {
    if (tasks[t].start_head < num_heads) {
      pthread_join(threads[t], NULL);
    }
  }

  free(threads);
  free(tasks);
}

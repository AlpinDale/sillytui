#include "inference/backend/cpu/scalar/scalar_backend.h"
#include "inference/backend/registry.h"
#include "inference/core/dtype.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

static bool scalar_init(backend_t *backend) {
  (void)backend;
  return true;
}

static void scalar_destroy(backend_t *backend) { (void)backend; }

static void scalar_gemm(backend_t *backend, const tensor_t *A,
                        const tensor_t *B, tensor_t *C, bool transpose_A,
                        bool transpose_B) {
  (void)backend;

  if (!A || !B || !C || !A->data || !B->data || !C->data)
    return;

  int M = (int)tensor_dim(C, 0);
  int N = (int)tensor_dim(C, 1);
  int K = transpose_A ? (int)tensor_dim(A, 0) : (int)tensor_dim(A, 1);

  const float *a = tensor_data_f32_const(A);
  const float *b = tensor_data_f32_const(B);
  float *c = tensor_data_f32(C);

  memset(c, 0, M * N * sizeof(float));

  if (!transpose_A && !transpose_B) {
    for (int i = 0; i < M; i++) {
      for (int k = 0; k < K; k++) {
        float aik = a[i * K + k];
        for (int j = 0; j < N; j++) {
          c[i * N + j] += aik * b[k * N + j];
        }
      }
    }
  } else if (transpose_A && !transpose_B) {
    for (int i = 0; i < M; i++) {
      for (int k = 0; k < K; k++) {
        float aki = a[k * M + i];
        for (int j = 0; j < N; j++) {
          c[i * N + j] += aki * b[k * N + j];
        }
      }
    }
  } else if (!transpose_A && transpose_B) {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
          sum += a[i * K + k] * b[j * K + k];
        }
        c[i * N + j] = sum;
      }
    }
  } else {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
          sum += a[k * M + i] * b[j * K + k];
        }
        c[i * N + j] = sum;
      }
    }
  }
}

static void scalar_gemv(backend_t *backend, const tensor_t *A,
                        const tensor_t *x, tensor_t *y) {
  (void)backend;

  if (!A || !x || !y)
    return;

  int M = (int)tensor_dim(A, 0);
  int K = (int)tensor_dim(A, 1);

  const float *a = tensor_data_f32_const(A);
  const float *xv = tensor_data_f32_const(x);
  float *yv = tensor_data_f32(y);

  for (int i = 0; i < M; i++) {
    float sum = 0.0f;
    for (int k = 0; k < K; k++) {
      sum += a[i * K + k] * xv[k];
    }
    yv[i] = sum;
  }
}

static void scalar_silu(backend_t *backend, tensor_t *out, const tensor_t *in) {
  (void)backend;

  if (!out || !in)
    return;

  size_t n = tensor_numel(in);
  const float *src = tensor_data_f32_const(in);
  float *dst = tensor_data_f32(out);

  for (size_t i = 0; i < n; i++) {
    dst[i] = src[i] / (1.0f + expf(-src[i]));
  }
}

static void scalar_silu_mul(backend_t *backend, tensor_t *out,
                            const tensor_t *gate, const tensor_t *up) {
  (void)backend;

  if (!out || !gate || !up)
    return;

  size_t n = tensor_numel(gate);
  const float *g = tensor_data_f32_const(gate);
  const float *u = tensor_data_f32_const(up);
  float *dst = tensor_data_f32(out);

  for (size_t i = 0; i < n; i++) {
    float silu_g = g[i] / (1.0f + expf(-g[i]));
    dst[i] = silu_g * u[i];
  }
}

static void scalar_gelu(backend_t *backend, tensor_t *out, const tensor_t *in) {
  (void)backend;

  if (!out || !in)
    return;

  size_t n = tensor_numel(in);
  const float *src = tensor_data_f32_const(in);
  float *dst = tensor_data_f32(out);

  for (size_t i = 0; i < n; i++) {
    dst[i] = src[i] * 0.5f * (1.0f + erff(src[i] * (float)M_SQRT1_2));
  }
}

static void scalar_gelu_tanh(backend_t *backend, tensor_t *out,
                             const tensor_t *in) {
  (void)backend;

  if (!out || !in)
    return;

  const float w1 = (float)(M_SQRT2 * M_2_SQRTPI * 0.5);
  const float w3 = 0.044715f;

  size_t n = tensor_numel(in);
  const float *src = tensor_data_f32_const(in);
  float *dst = tensor_data_f32(out);

  for (size_t i = 0; i < n; i++) {
    float x = src[i];
    float x3 = x * x * x;
    float inner = w1 * (x + x3 * w3);
    dst[i] = x * 0.5f * (1.0f + tanhf(inner));
  }
}

static void scalar_rms_norm(backend_t *backend, tensor_t *out,
                            const tensor_t *in, const tensor_t *weight,
                            float eps) {
  (void)backend;

  if (!out || !in || !weight)
    return;

  int num_tokens = (int)tensor_dim(in, 0);
  int hidden_size = (int)tensor_dim(in, 1);

  const float *input = tensor_data_f32_const(in);
  const float *w = tensor_data_f32_const(weight);
  float *output = tensor_data_f32(out);

  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * hidden_size;
    float *out_row = output + i * hidden_size;

    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      variance += in_row[j] * in_row[j];
    }
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + eps);

    for (int j = 0; j < hidden_size; j++) {
      out_row[j] = in_row[j] * scale * w[j];
    }
  }
}

static void scalar_layer_norm(backend_t *backend, tensor_t *out,
                              const tensor_t *in, const tensor_t *weight,
                              const tensor_t *bias, float eps) {
  (void)backend;

  if (!out || !in)
    return;

  int num_tokens = (int)tensor_dim(in, 0);
  int hidden_size = (int)tensor_dim(in, 1);

  const float *input = tensor_data_f32_const(in);
  const float *w = weight ? tensor_data_f32_const(weight) : NULL;
  const float *b = bias ? tensor_data_f32_const(bias) : NULL;
  float *output = tensor_data_f32(out);

  for (int i = 0; i < num_tokens; i++) {
    const float *in_row = input + i * hidden_size;
    float *out_row = output + i * hidden_size;

    float mean = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      mean += in_row[j];
    }
    mean /= (float)hidden_size;

    float variance = 0.0f;
    for (int j = 0; j < hidden_size; j++) {
      float diff = in_row[j] - mean;
      variance += diff * diff;
    }
    variance /= (float)hidden_size;

    float scale = 1.0f / sqrtf(variance + eps);

    for (int j = 0; j < hidden_size; j++) {
      float normalized = (in_row[j] - mean) * scale;
      if (w)
        normalized *= w[j];
      if (b)
        normalized += b[j];
      out_row[j] = normalized;
    }
  }
}

static void scalar_softmax(backend_t *backend, tensor_t *out,
                           const tensor_t *in, int axis) {
  (void)backend;
  (void)axis;

  if (!out || !in)
    return;

  int num_rows = (int)tensor_dim(in, 0);
  int row_size = (int)tensor_dim(in, 1);

  const float *input = tensor_data_f32_const(in);
  float *output = tensor_data_f32(out);

  for (int i = 0; i < num_rows; i++) {
    const float *in_row = input + i * row_size;
    float *out_row = output + i * row_size;

    float max_val = in_row[0];
    for (int j = 1; j < row_size; j++) {
      if (in_row[j] > max_val)
        max_val = in_row[j];
    }

    float sum = 0.0f;
    for (int j = 0; j < row_size; j++) {
      out_row[j] = expf(in_row[j] - max_val);
      sum += out_row[j];
    }

    float inv_sum = 1.0f / sum;
    for (int j = 0; j < row_size; j++) {
      out_row[j] *= inv_sum;
    }
  }
}

static void scalar_embedding_lookup(backend_t *backend, tensor_t *out,
                                    const tensor_t *weight,
                                    const int64_t *indices, int num_indices) {
  (void)backend;

  if (!out || !weight || !indices)
    return;

  int embed_dim = (int)tensor_dim(weight, 1);
  const float *w = tensor_data_f32_const(weight);
  float *output = tensor_data_f32(out);

  for (int i = 0; i < num_indices; i++) {
    int idx = (int)indices[i];
    memcpy(output + i * embed_dim, w + idx * embed_dim,
           embed_dim * sizeof(float));
  }
}

static int scalar_sample(backend_t *backend, const tensor_t *logits,
                         float temperature, int top_k, float top_p, void *rng) {
  (void)backend;
  (void)top_k;
  (void)top_p;
  (void)rng;

  if (!logits)
    return 0;

  int vocab_size = (int)tensor_numel(logits);
  const float *l = tensor_data_f32_const(logits);

  if (temperature <= 0.0f) {
    int max_idx = 0;
    float max_val = l[0];
    for (int i = 1; i < vocab_size; i++) {
      if (l[i] > max_val) {
        max_val = l[i];
        max_idx = i;
      }
    }
    return max_idx;
  }

  float max_val = l[0];
  for (int i = 1; i < vocab_size; i++) {
    if (l[i] > max_val)
      max_val = l[i];
  }

  float sum = 0.0f;
  float *probs = (float *)malloc(vocab_size * sizeof(float));
  for (int i = 0; i < vocab_size; i++) {
    probs[i] = expf((l[i] - max_val) / temperature);
    sum += probs[i];
  }

  float r = (float)rand() / (float)RAND_MAX * sum;
  float cumsum = 0.0f;
  int result = vocab_size - 1;
  for (int i = 0; i < vocab_size; i++) {
    cumsum += probs[i];
    if (cumsum >= r) {
      result = i;
      break;
    }
  }

  free(probs);
  return result;
}

/**
 * RoPE (Rotary Position Embeddings) - NeoX style
 *
 * Applies rotary embeddings to query and key tensors:
 *   q[i] = q[i] * cos - q[half_dim + i] * sin
 *   q[half_dim + i] = q[half_dim + i] * cos + q[i] * sin
 */
static void scalar_rope(backend_t *backend, tensor_t *query, tensor_t *key,
                        const tensor_t *cos_sin_cache, const int64_t *positions,
                        int num_heads, int num_kv_heads, int head_dim,
                        bool is_neox) {
  (void)backend;

  if (!query || !cos_sin_cache || !positions)
    return;

  int num_tokens = (int)tensor_dim(query, 0);
  int rot_dim = head_dim;
  int half_dim = rot_dim / 2;

  const float *cache = tensor_data_f32_const(cos_sin_cache);
  float *q = tensor_data_f32(query);
  float *k = key ? tensor_data_f32(key) : NULL;

  int query_stride = num_heads * head_dim;
  int key_stride = num_kv_heads * head_dim;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const float *cos_ptr = cache + pos * rot_dim;
    const float *sin_ptr = cos_ptr + half_dim;

    /* Apply RoPE to query heads */
    for (int h = 0; h < num_heads; h++) {
      float *qh = q + t * query_stride + h * head_dim;

      if (is_neox) {
        /* NeoX style: first half and second half */
        for (int i = 0; i < half_dim; i++) {
          float x = qh[i];
          float y = qh[half_dim + i];
          float cos_val = cos_ptr[i];
          float sin_val = sin_ptr[i];
          qh[i] = x * cos_val - y * sin_val;
          qh[half_dim + i] = y * cos_val + x * sin_val;
        }
      } else {
        /* GPT-J style: interleaved pairs */
        for (int i = 0; i < half_dim; i++) {
          int x_idx = 2 * i;
          int y_idx = 2 * i + 1;
          float x = qh[x_idx];
          float y = qh[y_idx];
          float cos_val = cos_ptr[i];
          float sin_val = sin_ptr[i];
          qh[x_idx] = x * cos_val - y * sin_val;
          qh[y_idx] = y * cos_val + x * sin_val;
        }
      }
    }

    /* Apply RoPE to key heads */
    if (k) {
      for (int h = 0; h < num_kv_heads; h++) {
        float *kh = k + t * key_stride + h * head_dim;

        if (is_neox) {
          for (int i = 0; i < half_dim; i++) {
            float x = kh[i];
            float y = kh[half_dim + i];
            float cos_val = cos_ptr[i];
            float sin_val = sin_ptr[i];
            kh[i] = x * cos_val - y * sin_val;
            kh[half_dim + i] = y * cos_val + x * sin_val;
          }
        } else {
          for (int i = 0; i < half_dim; i++) {
            int x_idx = 2 * i;
            int y_idx = 2 * i + 1;
            float x = kh[x_idx];
            float y = kh[y_idx];
            float cos_val = cos_ptr[i];
            float sin_val = sin_ptr[i];
            kh[x_idx] = x * cos_val - y * sin_val;
            kh[y_idx] = y * cos_val + x * sin_val;
          }
        }
      }
    }
  }
}

/**
 * Scaled Dot-Product Attention with KV Cache
 *
 * Computes: softmax(Q @ K^T / scale) @ V
 * Uses online softmax algorithm for numerical stability.
 */
static void scalar_attention(backend_t *backend, tensor_t *out,
                             const tensor_t *Q, const tensor_t *K,
                             const tensor_t *V, tensor_t *key_cache,
                             tensor_t *value_cache, int cache_len,
                             float scale) {
  (void)backend;

  if (!out || !Q || !K || !V)
    return;

  int seq_len_q = (int)tensor_dim(Q, 0);
  int head_dim = (int)tensor_dim(Q, 1);
  int seq_len_kv = (int)tensor_dim(K, 0);

  const float *query = tensor_data_f32_const(Q);
  const float *key = tensor_data_f32_const(K);
  const float *value = tensor_data_f32_const(V);
  float *output = tensor_data_f32(out);

  /* Get cached keys/values if available */
  const float *cached_key = key_cache ? tensor_data_f32_const(key_cache) : NULL;
  const float *cached_value =
      value_cache ? tensor_data_f32_const(value_cache) : NULL;

  int total_kv_len = cache_len + seq_len_kv;

  float *acc = (float *)calloc(head_dim, sizeof(float));

  for (int q_idx = 0; q_idx < seq_len_q; q_idx++) {
    const float *q_vec = query + q_idx * head_dim;
    float *out_vec = output + q_idx * head_dim;

    float M = -__builtin_inff();
    float L = 0.0f;
    memset(acc, 0, head_dim * sizeof(float));

    /* Process cached KV first */
    for (int kv_idx = 0; kv_idx < cache_len; kv_idx++) {
      const float *k_vec = cached_key + kv_idx * head_dim;
      const float *v_vec = cached_value + kv_idx * head_dim;

      /* Compute attention score */
      float s = 0.0f;
      for (int d = 0; d < head_dim; d++) {
        s += q_vec[d] * k_vec[d];
      }
      s *= scale;

      /* Online softmax update */
      float M_new = (s > M) ? s : M;
      float exp_M_diff = expf(M - M_new);

      for (int d = 0; d < head_dim; d++) {
        acc[d] *= exp_M_diff;
      }
      L *= exp_M_diff;

      float p = expf(s - M_new);
      for (int d = 0; d < head_dim; d++) {
        acc[d] += v_vec[d] * p;
      }
      L += p;

      M = M_new;
    }

    /* Process new KV */
    for (int kv_idx = 0; kv_idx < seq_len_kv; kv_idx++) {
      const float *k_vec = key + kv_idx * head_dim;
      const float *v_vec = value + kv_idx * head_dim;

      /* Apply causal mask: can only attend to positions <= q_idx */
      if (cache_len + kv_idx > cache_len + q_idx)
        continue;

      float s = 0.0f;
      for (int d = 0; d < head_dim; d++) {
        s += q_vec[d] * k_vec[d];
      }
      s *= scale;

      float M_new = (s > M) ? s : M;
      float exp_M_diff = expf(M - M_new);

      for (int d = 0; d < head_dim; d++) {
        acc[d] *= exp_M_diff;
      }
      L *= exp_M_diff;

      float p = expf(s - M_new);
      for (int d = 0; d < head_dim; d++) {
        acc[d] += v_vec[d] * p;
      }
      L += p;

      M = M_new;
    }

    /* Normalize */
    if (L > 0.0f) {
      float inv_L = 1.0f / L;
      for (int d = 0; d < head_dim; d++) {
        out_vec[d] = acc[d] * inv_L;
      }
    } else {
      memset(out_vec, 0, head_dim * sizeof(float));
    }
  }

  (void)total_kv_len;
  free(acc);
}

/**
 * KV Cache Update
 *
 * Appends new keys and values to the cache.
 * Returns the new cache length.
 */
static int scalar_kv_cache_update(backend_t *backend, tensor_t *key_cache,
                                  tensor_t *value_cache,
                                  const tensor_t *new_keys,
                                  const tensor_t *new_values, int cache_len) {
  (void)backend;

  if (!key_cache || !value_cache || !new_keys || !new_values)
    return cache_len;

  int num_tokens = (int)tensor_dim(new_keys, 0);
  int kv_dim = (int)tensor_dim(new_keys, 1);

  float *k_cache = tensor_data_f32(key_cache);
  float *v_cache = tensor_data_f32(value_cache);
  const float *keys = tensor_data_f32_const(new_keys);
  const float *values = tensor_data_f32_const(new_values);

  /* Append new keys and values */
  for (int t = 0; t < num_tokens; t++) {
    int cache_offset = (cache_len + t) * kv_dim;
    int input_offset = t * kv_dim;

    memcpy(k_cache + cache_offset, keys + input_offset, kv_dim * sizeof(float));
    memcpy(v_cache + cache_offset, values + input_offset,
           kv_dim * sizeof(float));
  }

  return cache_len + num_tokens;
}

static const backend_ops_t scalar_ops = {
    .name = "scalar",
    .capability = CAP_SCALAR,
    .init = scalar_init,
    .destroy = scalar_destroy,
    .gemm = scalar_gemm,
    .gemv = scalar_gemv,
    .silu = scalar_silu,
    .silu_mul = scalar_silu_mul,
    .gelu = scalar_gelu,
    .gelu_tanh = scalar_gelu_tanh,
    .rms_norm = scalar_rms_norm,
    .layer_norm = scalar_layer_norm,
    .rope = scalar_rope,
    .attention = scalar_attention,
    .softmax = scalar_softmax,
    .sample = scalar_sample,
    .embedding_lookup = scalar_embedding_lookup,
    .kv_cache_update = scalar_kv_cache_update,
};

const backend_ops_t *scalar_backend_ops(void) { return &scalar_ops; }

__attribute__((constructor)) static void register_scalar_backend(void) {
  backend_register(&scalar_ops);
}

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
    .rope = NULL,
    .attention = NULL,
    .softmax = scalar_softmax,
    .sample = scalar_sample,
    .embedding_lookup = scalar_embedding_lookup,
    .kv_cache_update = NULL,
};

const backend_ops_t *scalar_backend_ops(void) { return &scalar_ops; }

__attribute__((constructor)) static void register_scalar_backend(void) {
  backend_register(&scalar_ops);
}

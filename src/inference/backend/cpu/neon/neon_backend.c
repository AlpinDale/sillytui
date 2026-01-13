#include "inference/backend/cpu/neon/neon_backend.h"
#include "inference/backend/registry.h"

#if defined(__ARM_NEON) || defined(__ARM_NEON__)

#include "inference/core/dtype.h"
#include "inference/kernels/activation/activation_kernels.h"
#include "inference/kernels/attention/attention.h"
#include "inference/kernels/embedding/embedding_kernels.h"
#include "inference/kernels/gemm/gemm_kernels.h"
#include "inference/kernels/kv_cache/kv_cache_kernels.h"
#include "inference/kernels/norm/layernorm_kernels.h"
#include "inference/kernels/rope/rope_kernels.h"
#include "inference/kernels/sampling/sampling_kernels.h"
#include "inference/kernels/softmax/softmax_kernels.h"
#include <arm_neon.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool neon_init(backend_t *backend) {
  (void)backend;
  return true;
}

static void neon_destroy(backend_t *backend) { (void)backend; }

static void neon_gemm(backend_t *backend, const tensor_t *A, const tensor_t *B,
                      tensor_t *C, bool transpose_A, bool transpose_B) {
  if (!A || !B || !C || !A->data || !B->data || !C->data)
    return;

  if (transpose_A || transpose_B)
    return;

  int M = (int)tensor_dim(C, 0);
  int N = (int)tensor_dim(C, 1);
  int K = (int)tensor_dim(A, 1);
  int num_threads = backend ? backend->num_threads : 1;

  if (A->dtype == DTYPE_F32) {
    const float *a = tensor_data_f32_const(A);
    const float *b = tensor_data_f32_const(B);
    float *c = tensor_data_f32(C);

    if (num_threads > 1 && M >= 64) {
      gemm_f32_kernel_mt(a, b, c, M, N, K, num_threads);
    } else {
      gemm_f32_kernel(a, b, c, M, N, K);
    }
  } else if (A->dtype == DTYPE_F16) {
    const uint16_t *a = tensor_data_f16_const(A);
    const uint16_t *b = tensor_data_f16_const(B);
    uint16_t *c = tensor_data_f16(C);

    if (num_threads > 1 && M >= 64) {
      gemm_f16_kernel_mt(a, b, c, M, N, K, num_threads);
    } else {
      gemm_f16_kernel(a, b, c, M, N, K);
    }
  } else if (A->dtype == DTYPE_BF16) {
    const uint16_t *a = tensor_data_f16_const(A);
    const uint16_t *b = tensor_data_f16_const(B);
    uint16_t *c = tensor_data_f16(C);

    if (num_threads > 1 && M >= 64) {
      gemm_bf16_kernel_mt(a, b, c, M, N, K, num_threads);
    } else {
      gemm_bf16_kernel(a, b, c, M, N, K);
    }
  }
}

static void neon_silu(backend_t *backend, tensor_t *out, const tensor_t *in) {
  (void)backend;

  if (!out || !in)
    return;

  int n = (int)tensor_numel(in);

  if (in->dtype == DTYPE_F32) {
    silu_f32_kernel(tensor_data_f32(out), tensor_data_f32_const(in), 1, n);
  } else if (in->dtype == DTYPE_F16) {
    silu_f16_kernel(tensor_data_f16(out), tensor_data_f16_const(in), 1, n);
  } else if (in->dtype == DTYPE_BF16) {
    silu_bf16_kernel(tensor_data_f16(out), tensor_data_f16_const(in), 1, n);
  }
}

static void neon_silu_mul(backend_t *backend, tensor_t *out,
                          const tensor_t *gate, const tensor_t *up) {
  (void)backend;

  if (!out || !gate || !up)
    return;

  size_t n = tensor_numel(gate);
  const float *g = tensor_data_f32_const(gate);
  const float *u = tensor_data_f32_const(up);
  float *dst = tensor_data_f32(out);

  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    float32x4_t gv = vld1q_f32(g + i);
    float32x4_t uv = vld1q_f32(u + i);
    float32x4_t neg_g = vnegq_f32(gv);
    float32x4_t one = vdupq_n_f32(1.0f);

    float32x4_t clamp_lo = vdupq_n_f32(-88.0f);
    float32x4_t clamp_hi = vdupq_n_f32(88.0f);
    neg_g = vmaxq_f32(neg_g, clamp_lo);
    neg_g = vminq_f32(neg_g, clamp_hi);

    float32x4_t log2e = vdupq_n_f32(1.442695041f);
    float32x4_t ln2 = vdupq_n_f32(0.6931471806f);
    float32x4_t z = vmulq_f32(neg_g, log2e);
    float32x4_t floor_z = vrndmq_f32(z);
    int32x4_t exp_n = vcvtq_s32_f32(floor_z);
    float32x4_t r = vmlsq_f32(neg_g, floor_z, ln2);

    float32x4_t c0 = vdupq_n_f32(1.0f);
    float32x4_t c2 = vdupq_n_f32(0.5f);
    float32x4_t c3 = vdupq_n_f32(0.16666666666f);
    float32x4_t c4 = vdupq_n_f32(0.04166666666f);
    float32x4_t c5 = vdupq_n_f32(0.00833333333f);

    float32x4_t p = c5;
    p = vmlaq_f32(c4, p, r);
    p = vmlaq_f32(c3, p, r);
    p = vmlaq_f32(c2, p, r);
    p = vmlaq_f32(c0, p, r);
    p = vmlaq_f32(c0, p, r);

    exp_n = vaddq_s32(exp_n, vdupq_n_s32(127));
    exp_n = vshlq_n_s32(exp_n, 23);
    float32x4_t scale = vreinterpretq_f32_s32(exp_n);
    float32x4_t exp_neg_g = vmulq_f32(p, scale);

    float32x4_t sigmoid = vdivq_f32(one, vaddq_f32(one, exp_neg_g));
    float32x4_t silu_g = vmulq_f32(gv, sigmoid);
    float32x4_t result = vmulq_f32(silu_g, uv);
    vst1q_f32(dst + i, result);
  }

  for (; i < n; i++) {
    float silu_g = g[i] / (1.0f + expf(-g[i]));
    dst[i] = silu_g * u[i];
  }
}

static void neon_gelu(backend_t *backend, tensor_t *out, const tensor_t *in) {
  (void)backend;

  if (!out || !in)
    return;

  int n = (int)tensor_numel(in);

  if (in->dtype == DTYPE_F32) {
    gelu_f32_kernel(tensor_data_f32(out), tensor_data_f32_const(in), 1, n);
  } else if (in->dtype == DTYPE_F16) {
    gelu_f16_kernel(tensor_data_f16(out), tensor_data_f16_const(in), 1, n);
  } else if (in->dtype == DTYPE_BF16) {
    gelu_bf16_kernel(tensor_data_f16(out), tensor_data_f16_const(in), 1, n);
  }
}

static void neon_gelu_tanh(backend_t *backend, tensor_t *out,
                           const tensor_t *in) {
  (void)backend;

  if (!out || !in)
    return;

  int n = (int)tensor_numel(in);

  if (in->dtype == DTYPE_F32) {
    gelu_tanh_f32_kernel(tensor_data_f32(out), tensor_data_f32_const(in), 1, n);
  } else if (in->dtype == DTYPE_F16) {
    gelu_tanh_f16_kernel(tensor_data_f16(out), tensor_data_f16_const(in), 1, n);
  } else if (in->dtype == DTYPE_BF16) {
    gelu_tanh_bf16_kernel(tensor_data_f16(out), tensor_data_f16_const(in), 1,
                          n);
  }
}

static void neon_rms_norm(backend_t *backend, tensor_t *out, const tensor_t *in,
                          const tensor_t *weight, float eps) {
  (void)backend;

  if (!out || !in || !weight)
    return;

  int num_tokens = (int)tensor_dim(in, 0);
  int hidden_size = (int)tensor_dim(in, 1);

  if (in->dtype == DTYPE_F32) {
    rms_norm_f32_kernel(tensor_data_f32(out), tensor_data_f32_const(in),
                        tensor_data_f32_const(weight), eps, num_tokens,
                        hidden_size);
  } else if (in->dtype == DTYPE_F16) {
    rms_norm_f16_kernel(tensor_data_f16(out), tensor_data_f16_const(in),
                        tensor_data_f16_const(weight), eps, num_tokens,
                        hidden_size);
  } else if (in->dtype == DTYPE_BF16) {
    rms_norm_bf16_kernel(tensor_data_f16(out), tensor_data_f16_const(in),
                         tensor_data_f16_const(weight), eps, num_tokens,
                         hidden_size);
  }
}

static void neon_softmax(backend_t *backend, tensor_t *out, const tensor_t *in,
                         int axis) {
  (void)backend;
  (void)axis;

  if (!out || !in)
    return;

  int num_rows = (int)tensor_dim(in, 0);
  int row_size = (int)tensor_dim(in, 1);
  float scale = 1.0f;

  if (in->dtype == DTYPE_F32) {
    softmax_f32_kernel(tensor_data_f32(out), tensor_data_f32_const(in),
                       num_rows, row_size, scale);
  } else if (in->dtype == DTYPE_F16) {
    softmax_f16_kernel(tensor_data_f16(out), tensor_data_f16_const(in),
                       num_rows, row_size, scale);
  } else if (in->dtype == DTYPE_BF16) {
    softmax_bf16_kernel(tensor_data_f16(out), tensor_data_f16_const(in),
                        num_rows, row_size, scale);
  }
}

/* ============================================================================
 * GEMV - Matrix-Vector Multiplication with NEON
 * Computes: y = A @ x where A is [M x K] and x is [K]
 * ============================================================================
 */
static void neon_gemv(backend_t *backend, const tensor_t *A, const tensor_t *x,
                      tensor_t *y) {
  (void)backend;

  if (!A || !x || !y || !A->data || !x->data || !y->data)
    return;

  int M = (int)tensor_dim(A, 0);
  int K = (int)tensor_dim(A, 1);

  if (A->dtype == DTYPE_F32) {
    const float *a_data = tensor_data_f32_const(A);
    const float *x_data = tensor_data_f32_const(x);
    float *y_data = tensor_data_f32(y);

    for (int m = 0; m < M; m++) {
      const float *row = a_data + m * K;
      float32x4_t sum_vec = vdupq_n_f32(0.0f);

      int k = 0;
      for (; k + 16 <= K; k += 16) {
        float32x4_t a0 = vld1q_f32(row + k);
        float32x4_t a1 = vld1q_f32(row + k + 4);
        float32x4_t a2 = vld1q_f32(row + k + 8);
        float32x4_t a3 = vld1q_f32(row + k + 12);
        float32x4_t x0 = vld1q_f32(x_data + k);
        float32x4_t x1 = vld1q_f32(x_data + k + 4);
        float32x4_t x2 = vld1q_f32(x_data + k + 8);
        float32x4_t x3 = vld1q_f32(x_data + k + 12);
        sum_vec = vmlaq_f32(sum_vec, a0, x0);
        sum_vec = vmlaq_f32(sum_vec, a1, x1);
        sum_vec = vmlaq_f32(sum_vec, a2, x2);
        sum_vec = vmlaq_f32(sum_vec, a3, x3);
      }
      for (; k + 4 <= K; k += 4) {
        float32x4_t a_v = vld1q_f32(row + k);
        float32x4_t x_v = vld1q_f32(x_data + k);
        sum_vec = vmlaq_f32(sum_vec, a_v, x_v);
      }

      float sum = vaddvq_f32(sum_vec);
      for (; k < K; k++) {
        sum += row[k] * x_data[k];
      }
      y_data[m] = sum;
    }
  } else if (A->dtype == DTYPE_F16) {
    const uint16_t *a_data = tensor_data_f16_const(A);
    const uint16_t *x_data = tensor_data_f16_const(x);
    uint16_t *y_data = tensor_data_f16(y);

    for (int m = 0; m < M; m++) {
      const uint16_t *row = a_data + m * K;
      float32x4_t sum_vec = vdupq_n_f32(0.0f);

      int k = 0;
      for (; k + 8 <= K; k += 8) {
        float16x8_t a_h = vld1q_f16((const float16_t *)(row + k));
        float16x8_t x_h = vld1q_f16((const float16_t *)(x_data + k));
        float32x4_t a_lo = vcvt_f32_f16(vget_low_f16(a_h));
        float32x4_t a_hi = vcvt_f32_f16(vget_high_f16(a_h));
        float32x4_t x_lo = vcvt_f32_f16(vget_low_f16(x_h));
        float32x4_t x_hi = vcvt_f32_f16(vget_high_f16(x_h));
        sum_vec = vmlaq_f32(sum_vec, a_lo, x_lo);
        sum_vec = vmlaq_f32(sum_vec, a_hi, x_hi);
      }
      for (; k + 4 <= K; k += 4) {
        float16x4_t a_h = vld1_f16((const float16_t *)(row + k));
        float16x4_t x_h = vld1_f16((const float16_t *)(x_data + k));
        float32x4_t a_f = vcvt_f32_f16(a_h);
        float32x4_t x_f = vcvt_f32_f16(x_h);
        sum_vec = vmlaq_f32(sum_vec, a_f, x_f);
      }

      float sum = vaddvq_f32(sum_vec);
      for (; k < K; k++) {
        sum += f16_to_f32(row[k]) * f16_to_f32(x_data[k]);
      }
      y_data[m] = f32_to_f16(sum);
    }
  } else if (A->dtype == DTYPE_BF16) {
    const uint16_t *a_data = tensor_data_f16_const(A);
    const uint16_t *x_data = tensor_data_f16_const(x);
    uint16_t *y_data = tensor_data_f16(y);

    for (int m = 0; m < M; m++) {
      const uint16_t *row = a_data + m * K;
      float32x4_t sum_vec = vdupq_n_f32(0.0f);

      int k = 0;
      for (; k + 4 <= K; k += 4) {
        float a0 = bf16_to_f32(row[k]);
        float a1 = bf16_to_f32(row[k + 1]);
        float a2 = bf16_to_f32(row[k + 2]);
        float a3 = bf16_to_f32(row[k + 3]);
        float x0 = bf16_to_f32(x_data[k]);
        float x1 = bf16_to_f32(x_data[k + 1]);
        float x2 = bf16_to_f32(x_data[k + 2]);
        float x3 = bf16_to_f32(x_data[k + 3]);
        float32x4_t a_v = {a0, a1, a2, a3};
        float32x4_t x_v = {x0, x1, x2, x3};
        sum_vec = vmlaq_f32(sum_vec, a_v, x_v);
      }

      float sum = vaddvq_f32(sum_vec);
      for (; k < K; k++) {
        sum += bf16_to_f32(row[k]) * bf16_to_f32(x_data[k]);
      }
      y_data[m] = f32_to_bf16(sum);
    }
  }
}

/* ============================================================================
 * Layer Normalization with NEON
 * out = (in - mean) / sqrt(var + eps) * weight + bias
 * ============================================================================
 */
static void neon_layer_norm(backend_t *backend, tensor_t *out,
                            const tensor_t *in, const tensor_t *weight,
                            const tensor_t *bias, float eps) {
  (void)backend;

  if (!out || !in)
    return;

  int num_tokens = (int)tensor_dim(in, 0);
  int hidden_size = (int)tensor_dim(in, 1);

  if (in->dtype == DTYPE_F32) {
    const float *in_data = tensor_data_f32_const(in);
    float *out_data = tensor_data_f32(out);
    const float *w_data = weight ? tensor_data_f32_const(weight) : NULL;
    const float *b_data = bias ? tensor_data_f32_const(bias) : NULL;

    for (int t = 0; t < num_tokens; t++) {
      const float *row = in_data + t * hidden_size;
      float *out_row = out_data + t * hidden_size;

      /* Compute mean using NEON */
      float32x4_t sum_vec = vdupq_n_f32(0.0f);
      int i = 0;
      for (; i + 4 <= hidden_size; i += 4) {
        sum_vec = vaddq_f32(sum_vec, vld1q_f32(row + i));
      }
      float mean = vaddvq_f32(sum_vec);
      for (; i < hidden_size; i++) {
        mean += row[i];
      }
      mean /= (float)hidden_size;

      /* Compute variance using NEON */
      float32x4_t var_vec = vdupq_n_f32(0.0f);
      float32x4_t mean_vec = vdupq_n_f32(mean);
      i = 0;
      for (; i + 4 <= hidden_size; i += 4) {
        float32x4_t v = vld1q_f32(row + i);
        float32x4_t diff = vsubq_f32(v, mean_vec);
        var_vec = vmlaq_f32(var_vec, diff, diff);
      }
      float var = vaddvq_f32(var_vec);
      for (; i < hidden_size; i++) {
        float diff = row[i] - mean;
        var += diff * diff;
      }
      var /= (float)hidden_size;

      float inv_std = 1.0f / sqrtf(var + eps);
      float32x4_t inv_std_vec = vdupq_n_f32(inv_std);

      /* Normalize and apply weight/bias */
      i = 0;
      for (; i + 4 <= hidden_size; i += 4) {
        float32x4_t v = vld1q_f32(row + i);
        float32x4_t norm = vmulq_f32(vsubq_f32(v, mean_vec), inv_std_vec);
        if (w_data) {
          norm = vmulq_f32(norm, vld1q_f32(w_data + i));
        }
        if (b_data) {
          norm = vaddq_f32(norm, vld1q_f32(b_data + i));
        }
        vst1q_f32(out_row + i, norm);
      }
      for (; i < hidden_size; i++) {
        float norm = (row[i] - mean) * inv_std;
        if (w_data)
          norm *= w_data[i];
        if (b_data)
          norm += b_data[i];
        out_row[i] = norm;
      }
    }
  } else if (in->dtype == DTYPE_F16) {
    const uint16_t *in_data = tensor_data_f16_const(in);
    uint16_t *out_data = tensor_data_f16(out);
    const uint16_t *w_data = weight ? tensor_data_f16_const(weight) : NULL;
    const uint16_t *b_data = bias ? tensor_data_f16_const(bias) : NULL;

    for (int t = 0; t < num_tokens; t++) {
      const uint16_t *row = in_data + t * hidden_size;
      uint16_t *out_row = out_data + t * hidden_size;

      /* Compute mean */
      float32x4_t sum_vec = vdupq_n_f32(0.0f);
      int i = 0;
      for (; i + 8 <= hidden_size; i += 8) {
        float16x8_t v_h = vld1q_f16((const float16_t *)(row + i));
        float32x4_t lo = vcvt_f32_f16(vget_low_f16(v_h));
        float32x4_t hi = vcvt_f32_f16(vget_high_f16(v_h));
        sum_vec = vaddq_f32(sum_vec, lo);
        sum_vec = vaddq_f32(sum_vec, hi);
      }
      float mean = vaddvq_f32(sum_vec);
      for (; i < hidden_size; i++) {
        mean += f16_to_f32(row[i]);
      }
      mean /= (float)hidden_size;

      /* Compute variance */
      float32x4_t var_vec = vdupq_n_f32(0.0f);
      float32x4_t mean_vec = vdupq_n_f32(mean);
      i = 0;
      for (; i + 8 <= hidden_size; i += 8) {
        float16x8_t v_h = vld1q_f16((const float16_t *)(row + i));
        float32x4_t lo = vcvt_f32_f16(vget_low_f16(v_h));
        float32x4_t hi = vcvt_f32_f16(vget_high_f16(v_h));
        float32x4_t diff_lo = vsubq_f32(lo, mean_vec);
        float32x4_t diff_hi = vsubq_f32(hi, mean_vec);
        var_vec = vmlaq_f32(var_vec, diff_lo, diff_lo);
        var_vec = vmlaq_f32(var_vec, diff_hi, diff_hi);
      }
      float var = vaddvq_f32(var_vec);
      for (; i < hidden_size; i++) {
        float diff = f16_to_f32(row[i]) - mean;
        var += diff * diff;
      }
      var /= (float)hidden_size;

      float inv_std = 1.0f / sqrtf(var + eps);

      /* Normalize and apply weight/bias */
      for (i = 0; i < hidden_size; i++) {
        float val = f16_to_f32(row[i]);
        float norm = (val - mean) * inv_std;
        if (w_data)
          norm *= f16_to_f32(w_data[i]);
        if (b_data)
          norm += f16_to_f32(b_data[i]);
        out_row[i] = f32_to_f16(norm);
      }
    }
  } else if (in->dtype == DTYPE_BF16) {
    const uint16_t *in_data = tensor_data_f16_const(in);
    uint16_t *out_data = tensor_data_f16(out);
    const uint16_t *w_data = weight ? tensor_data_f16_const(weight) : NULL;
    const uint16_t *b_data = bias ? tensor_data_f16_const(bias) : NULL;

    for (int t = 0; t < num_tokens; t++) {
      const uint16_t *row = in_data + t * hidden_size;
      uint16_t *out_row = out_data + t * hidden_size;

      /* Compute mean */
      float mean = 0.0f;
      for (int i = 0; i < hidden_size; i++) {
        mean += bf16_to_f32(row[i]);
      }
      mean /= (float)hidden_size;

      /* Compute variance */
      float var = 0.0f;
      for (int i = 0; i < hidden_size; i++) {
        float diff = bf16_to_f32(row[i]) - mean;
        var += diff * diff;
      }
      var /= (float)hidden_size;

      float inv_std = 1.0f / sqrtf(var + eps);

      /* Normalize and apply weight/bias */
      for (int i = 0; i < hidden_size; i++) {
        float val = bf16_to_f32(row[i]);
        float norm = (val - mean) * inv_std;
        if (w_data)
          norm *= bf16_to_f32(w_data[i]);
        if (b_data)
          norm += bf16_to_f32(b_data[i]);
        out_row[i] = f32_to_bf16(norm);
      }
    }
  }
}

/* ============================================================================
 * RoPE - Rotary Position Embeddings using NEON kernels
 * ============================================================================
 */
static void neon_rope(backend_t *backend, tensor_t *query, tensor_t *key,
                      const tensor_t *cos_sin_cache, const int64_t *positions,
                      int num_heads, int num_kv_heads, int head_dim,
                      bool is_neox) {
  (void)backend;

  if (!query || !cos_sin_cache || !positions)
    return;

  int num_tokens = (int)tensor_dim(query, 0);
  int rot_dim = head_dim;

  if (query->dtype == DTYPE_F32) {
    float *q_data = tensor_data_f32(query);
    float *k_data = key ? tensor_data_f32(key) : NULL;
    const float *cache = tensor_data_f32_const(cos_sin_cache);

    if (is_neox) {
      rope_neox_f32_kernel(positions, q_data, k_data, cache, num_tokens,
                           num_heads, num_kv_heads, head_dim, rot_dim);
    } else {
      rope_gptj_f32_kernel(positions, q_data, k_data, cache, num_tokens,
                           num_heads, num_kv_heads, head_dim, rot_dim);
    }
  } else if (query->dtype == DTYPE_F16) {
    uint16_t *q_data = tensor_data_f16(query);
    uint16_t *k_data = key ? tensor_data_f16(key) : NULL;
    const uint16_t *cache = tensor_data_f16_const(cos_sin_cache);

    if (is_neox) {
      rope_neox_f16_kernel(positions, q_data, k_data, cache, num_tokens,
                           num_heads, num_kv_heads, head_dim, rot_dim);
    } else {
      rope_gptj_f16_kernel(positions, q_data, k_data, cache, num_tokens,
                           num_heads, num_kv_heads, head_dim, rot_dim);
    }
  } else if (query->dtype == DTYPE_BF16) {
    uint16_t *q_data = tensor_data_f16(query);
    uint16_t *k_data = key ? tensor_data_f16(key) : NULL;
    const uint16_t *cache = tensor_data_f16_const(cos_sin_cache);

    if (is_neox) {
      rope_neox_bf16_kernel(positions, q_data, k_data, cache, num_tokens,
                            num_heads, num_kv_heads, head_dim, rot_dim);
    } else {
      rope_gptj_bf16_kernel(positions, q_data, k_data, cache, num_tokens,
                            num_heads, num_kv_heads, head_dim, rot_dim);
    }
  }
}

/* ============================================================================
 * Attention - Scaled dot-product attention with KV cache
 * Uses flash attention for memory efficiency
 * ============================================================================
 */
static void neon_attention(backend_t *backend, tensor_t *out, const tensor_t *Q,
                           const tensor_t *K, const tensor_t *V,
                           tensor_t *key_cache, tensor_t *value_cache,
                           int cache_len, float scale) {
  (void)backend;

  if (!out || !Q || !K || !V || !key_cache || !value_cache)
    return;

  int num_tokens = (int)tensor_dim(Q, 0);
  int num_heads = (int)tensor_dim(Q, 1);
  int head_dim = (int)tensor_dim(Q, 2);
  int num_kv_heads = (int)tensor_dim(K, 1);
  int seq_len_kv = cache_len + num_tokens;

  /* First, append new K/V to caches */
  if (Q->dtype == DTYPE_F32) {
    kv_cache_append_f32_kernel(
        tensor_data_f32(key_cache), tensor_data_f32(value_cache),
        tensor_data_f32_const(K), tensor_data_f32_const(V), cache_len,
        num_tokens, num_kv_heads, head_dim);
  } else if (Q->dtype == DTYPE_F16) {
    kv_cache_append_f16_kernel(
        tensor_data_f16(key_cache), tensor_data_f16(value_cache),
        tensor_data_f16_const(K), tensor_data_f16_const(V), cache_len,
        num_tokens, num_kv_heads, head_dim);
  } else if (Q->dtype == DTYPE_BF16) {
    kv_cache_append_bf16_kernel(
        tensor_data_f16(key_cache), tensor_data_f16(value_cache),
        tensor_data_f16_const(K), tensor_data_f16_const(V), cache_len,
        num_tokens, num_kv_heads, head_dim);
  }

  /* Compute attention for each head */
  int heads_per_kv = num_heads / num_kv_heads;

  if (Q->dtype == DTYPE_F32) {
    const float *q_data = tensor_data_f32_const(Q);
    const float *k_cache = tensor_data_f32_const(key_cache);
    const float *v_cache = tensor_data_f32_const(value_cache);
    float *out_data = tensor_data_f32(out);

    for (int h = 0; h < num_heads; h++) {
      int kv_h = h / heads_per_kv;

      const float *q = q_data + h * num_tokens * head_dim;
      const float *k = k_cache + kv_h * seq_len_kv * head_dim;
      const float *v = v_cache + kv_h * seq_len_kv * head_dim;
      float *o = out_data + h * num_tokens * head_dim;

      flash_attention_f32(o, q, k, v, num_tokens, seq_len_kv, head_dim, scale,
                          NULL);
    }
  } else if (Q->dtype == DTYPE_F16) {
    const uint16_t *q_data = tensor_data_f16_const(Q);
    const uint16_t *k_cache = tensor_data_f16_const(key_cache);
    const uint16_t *v_cache = tensor_data_f16_const(value_cache);
    uint16_t *out_data = tensor_data_f16(out);

    for (int h = 0; h < num_heads; h++) {
      int kv_h = h / heads_per_kv;

      const uint16_t *q = q_data + h * num_tokens * head_dim;
      const uint16_t *k = k_cache + kv_h * seq_len_kv * head_dim;
      const uint16_t *v = v_cache + kv_h * seq_len_kv * head_dim;
      uint16_t *o = out_data + h * num_tokens * head_dim;

      flash_attention_f16(o, q, k, v, num_tokens, seq_len_kv, head_dim, scale,
                          NULL);
    }
  } else if (Q->dtype == DTYPE_BF16) {
    const uint16_t *q_data = tensor_data_f16_const(Q);
    const uint16_t *k_cache = tensor_data_f16_const(key_cache);
    const uint16_t *v_cache = tensor_data_f16_const(value_cache);
    uint16_t *out_data = tensor_data_f16(out);

    for (int h = 0; h < num_heads; h++) {
      int kv_h = h / heads_per_kv;

      const uint16_t *q = q_data + h * num_tokens * head_dim;
      const uint16_t *k = k_cache + kv_h * seq_len_kv * head_dim;
      const uint16_t *v = v_cache + kv_h * seq_len_kv * head_dim;
      uint16_t *o = out_data + h * num_tokens * head_dim;

      flash_attention_bf16(o, q, k, v, num_tokens, seq_len_kv, head_dim, scale,
                           NULL);
    }
  }
}

/* ============================================================================
 * Sample - Token sampling from logits using NEON kernel
 * ============================================================================
 */
static int neon_sample(backend_t *backend, const tensor_t *logits,
                       float temperature, int top_k, float top_p, void *rng) {
  (void)backend;

  if (!logits)
    return 0;

  int vocab_size = (int)tensor_numel(logits);

  /* Convert to F32 if needed for sampling */
  if (logits->dtype == DTYPE_F32) {
    const float *logits_data = tensor_data_f32_const(logits);
    return sampling_sample_f32_kernel(logits_data, vocab_size, temperature,
                                      top_k, top_p, 0.0f,
                                      (unsigned long long *)rng);
  } else if (logits->dtype == DTYPE_F16) {
    /* Convert F16 to F32 for sampling */
    const uint16_t *logits_f16 = tensor_data_f16_const(logits);
    float *logits_f32 = (float *)malloc(vocab_size * sizeof(float));
    if (!logits_f32)
      return 0;

    int i = 0;
    for (; i + 8 <= vocab_size; i += 8) {
      float16x8_t v = vld1q_f16((const float16_t *)(logits_f16 + i));
      float32x4_t lo = vcvt_f32_f16(vget_low_f16(v));
      float32x4_t hi = vcvt_f32_f16(vget_high_f16(v));
      vst1q_f32(logits_f32 + i, lo);
      vst1q_f32(logits_f32 + i + 4, hi);
    }
    for (; i < vocab_size; i++) {
      logits_f32[i] = f16_to_f32(logits_f16[i]);
    }

    int token =
        sampling_sample_f32_kernel(logits_f32, vocab_size, temperature, top_k,
                                   top_p, 0.0f, (unsigned long long *)rng);
    free(logits_f32);
    return token;
  } else if (logits->dtype == DTYPE_BF16) {
    /* Convert BF16 to F32 for sampling */
    const uint16_t *logits_bf16 = tensor_data_f16_const(logits);
    float *logits_f32 = (float *)malloc(vocab_size * sizeof(float));
    if (!logits_f32)
      return 0;

    for (int i = 0; i < vocab_size; i++) {
      logits_f32[i] = bf16_to_f32(logits_bf16[i]);
    }

    int token =
        sampling_sample_f32_kernel(logits_f32, vocab_size, temperature, top_k,
                                   top_p, 0.0f, (unsigned long long *)rng);
    free(logits_f32);
    return token;
  }

  return 0;
}

/* ============================================================================
 * Embedding Lookup using NEON kernel
 * ============================================================================
 */
static void neon_embedding_lookup(backend_t *backend, tensor_t *out,
                                  const tensor_t *weight,
                                  const int64_t *indices, int num_indices) {
  (void)backend;

  if (!out || !weight || !indices)
    return;

  int vocab_size = (int)tensor_dim(weight, 0);
  int embedding_dim = (int)tensor_dim(weight, 1);

  if (weight->dtype == DTYPE_F32) {
    embedding_lookup_f32_kernel(tensor_data_f32(out), indices,
                                tensor_data_f32_const(weight), num_indices,
                                vocab_size, embedding_dim, -1);
  } else if (weight->dtype == DTYPE_F16) {
    embedding_lookup_f16_kernel(tensor_data_f16(out), indices,
                                tensor_data_f16_const(weight), num_indices,
                                vocab_size, embedding_dim, -1);
  } else if (weight->dtype == DTYPE_BF16) {
    embedding_lookup_bf16_kernel(tensor_data_f16(out), indices,
                                 tensor_data_f16_const(weight), num_indices,
                                 vocab_size, embedding_dim, -1);
  }
}

/* ============================================================================
 * KV Cache Update using NEON kernel
 * ============================================================================
 */
static int neon_kv_cache_update(backend_t *backend, tensor_t *key_cache,
                                tensor_t *value_cache, const tensor_t *new_keys,
                                const tensor_t *new_values, int cache_len) {
  (void)backend;

  if (!key_cache || !value_cache || !new_keys || !new_values)
    return cache_len;

  int num_tokens = (int)tensor_dim(new_keys, 0);
  int num_heads = (int)tensor_dim(new_keys, 1);
  int head_dim = (int)tensor_dim(new_keys, 2);

  if (new_keys->dtype == DTYPE_F32) {
    kv_cache_append_f32_kernel(
        tensor_data_f32(key_cache), tensor_data_f32(value_cache),
        tensor_data_f32_const(new_keys), tensor_data_f32_const(new_values),
        cache_len, num_tokens, num_heads, head_dim);
  } else if (new_keys->dtype == DTYPE_F16) {
    kv_cache_append_f16_kernel(
        tensor_data_f16(key_cache), tensor_data_f16(value_cache),
        tensor_data_f16_const(new_keys), tensor_data_f16_const(new_values),
        cache_len, num_tokens, num_heads, head_dim);
  } else if (new_keys->dtype == DTYPE_BF16) {
    kv_cache_append_bf16_kernel(
        tensor_data_f16(key_cache), tensor_data_f16(value_cache),
        tensor_data_f16_const(new_keys), tensor_data_f16_const(new_values),
        cache_len, num_tokens, num_heads, head_dim);
  }

  return cache_len + num_tokens;
}

static const backend_ops_t neon_ops = {
    .name = "neon",
    .capability = CAP_NEON,
    .init = neon_init,
    .destroy = neon_destroy,
    .gemm = neon_gemm,
    .gemv = neon_gemv,
    .silu = neon_silu,
    .silu_mul = neon_silu_mul,
    .gelu = neon_gelu,
    .gelu_tanh = neon_gelu_tanh,
    .rms_norm = neon_rms_norm,
    .layer_norm = neon_layer_norm,
    .rope = neon_rope,
    .attention = neon_attention,
    .softmax = neon_softmax,
    .sample = neon_sample,
    .embedding_lookup = neon_embedding_lookup,
    .kv_cache_update = neon_kv_cache_update,
};

const backend_ops_t *neon_backend_ops(void) { return &neon_ops; }

__attribute__((constructor)) static void register_neon_backend(void) {
  backend_register(&neon_ops);
}

#else

const backend_ops_t *neon_backend_ops(void) { return NULL; }

#endif

#include "inference/backend/cpu/neon/neon_backend.h"
#include "inference/backend/registry.h"

#if defined(__ARM_NEON) || defined(__ARM_NEON__)

#include "inference/core/dtype.h"
#include "inference/kernels/activation/activation_kernels.h"
#include "inference/kernels/gemm/gemm_kernels.h"
#include "inference/kernels/norm/layernorm_kernels.h"
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

static const backend_ops_t neon_ops = {
    .name = "neon",
    .capability = CAP_NEON,
    .init = neon_init,
    .destroy = neon_destroy,
    .gemm = neon_gemm,
    .gemv = NULL,
    .silu = neon_silu,
    .silu_mul = neon_silu_mul,
    .gelu = neon_gelu,
    .gelu_tanh = neon_gelu_tanh,
    .rms_norm = neon_rms_norm,
    .layer_norm = NULL,
    .rope = NULL,
    .attention = NULL,
    .softmax = neon_softmax,
    .sample = NULL,
    .embedding_lookup = NULL,
    .kv_cache_update = NULL,
};

const backend_ops_t *neon_backend_ops(void) { return &neon_ops; }

__attribute__((constructor)) static void register_neon_backend(void) {
  backend_register(&neon_ops);
}

#else

const backend_ops_t *neon_backend_ops(void) { return NULL; }

#endif

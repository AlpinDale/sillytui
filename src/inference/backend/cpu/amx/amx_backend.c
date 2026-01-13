#include "inference/backend/cpu/amx/amx_backend.h"
#include "inference/backend/registry.h"

#if defined(__APPLE__) && defined(__aarch64__)

#include "inference/core/dtype.h"
#include "inference/kernels/gemm/gemm_kernels.h"
#include <stdlib.h>
#include <string.h>

static bool amx_init(backend_t *backend) {
  (void)backend;
  return true;
}

static void amx_destroy(backend_t *backend) { (void)backend; }

static void amx_gemm(backend_t *backend, const tensor_t *A, const tensor_t *B,
                     tensor_t *C, bool transpose_A, bool transpose_B) {
  if (!A || !B || !C || !A->data || !B->data || !C->data)
    return;

  if (transpose_A || transpose_B)
    return;

  int M = (int)tensor_dim(C, 0);
  int N = (int)tensor_dim(C, 1);
  int K = (int)tensor_dim(A, 1);
  int num_threads = backend ? backend->num_threads : 1;

  if (A->dtype == DTYPE_F16) {
    const uint16_t *a = tensor_data_f16_const(A);
    const uint16_t *b = tensor_data_f16_const(B);
    uint16_t *c = tensor_data_f16(C);

    if (num_threads > 1 && M >= 64) {
      gemm_f16_kernel_amx_mt(a, b, c, M, N, K, num_threads);
    } else {
      gemm_f16_kernel_amx(a, b, c, M, N, K);
    }
  } else if (A->dtype == DTYPE_BF16) {
    const uint16_t *a = tensor_data_f16_const(A);
    const uint16_t *b = tensor_data_f16_const(B);
    uint16_t *c = tensor_data_f16(C);

    if (num_threads > 1 && M >= 64) {
      gemm_bf16_kernel_amx_mt(a, b, c, M, N, K, num_threads);
    } else {
      gemm_bf16_kernel_amx(a, b, c, M, N, K);
    }
  }
}

static const backend_ops_t amx_ops = {
    .name = "amx",
    .capability = CAP_AMX,
    .init = amx_init,
    .destroy = amx_destroy,
    .gemm = amx_gemm,
    .gemv = NULL,
    .silu = NULL,
    .silu_mul = NULL,
    .gelu = NULL,
    .gelu_tanh = NULL,
    .rms_norm = NULL,
    .layer_norm = NULL,
    .rope = NULL,
    .attention = NULL,
    .softmax = NULL,
    .sample = NULL,
    .embedding_lookup = NULL,
    .kv_cache_update = NULL,
};

const backend_ops_t *amx_backend_ops(void) { return &amx_ops; }

__attribute__((constructor)) static void register_amx_backend(void) {
  backend_register(&amx_ops);
}

#else

const backend_ops_t *amx_backend_ops(void) { return NULL; }

#endif

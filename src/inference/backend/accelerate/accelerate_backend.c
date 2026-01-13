#include "inference/backend/accelerate/accelerate_backend.h"
#include "inference/backend/registry.h"

#if defined(__APPLE__)

#include "inference/core/dtype.h"
#include <Accelerate/Accelerate.h>
#include <stdlib.h>
#include <string.h>

static bool accel_init(backend_t *backend) {
  (void)backend;
  return true;
}

static void accel_destroy(backend_t *backend) { (void)backend; }

static void accel_gemm(backend_t *backend, const tensor_t *A, const tensor_t *B,
                       tensor_t *C, bool transpose_A, bool transpose_B) {
  if (!A || !B || !C || !A->data || !B->data || !C->data)
    return;

  int M = (int)tensor_dim(C, 0);
  int N = (int)tensor_dim(C, 1);
  int K = transpose_A ? (int)tensor_dim(A, 0) : (int)tensor_dim(A, 1);

  if (A->dtype == DTYPE_F32) {
    enum CBLAS_TRANSPOSE transA = transpose_A ? CblasTrans : CblasNoTrans;
    enum CBLAS_TRANSPOSE transB = transpose_B ? CblasTrans : CblasNoTrans;
    int lda = transpose_A ? M : K;
    int ldb = transpose_B ? K : N;
    int ldc = N;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    cblas_sgemm(CblasRowMajor, transA, transB, M, N, K, 1.0f,
                tensor_data_f32_const(A), lda, tensor_data_f32_const(B), ldb,
                0.0f, tensor_data_f32(C), ldc);
#pragma clang diagnostic pop
  } else if (A->dtype == DTYPE_F16) {
    (void)backend;
    BNNSDataType dt = BNNSDataTypeFloat16;

    BNNSNDArrayDescriptor descA = {
        .layout = BNNSDataLayoutRowMajorMatrix,
        .data = (void *)tensor_data_f16_const(A),
        .data_type = dt,
        .size = {(size_t)K, (size_t)M},
    };
    BNNSNDArrayDescriptor descB = {
        .layout = BNNSDataLayoutRowMajorMatrix,
        .data = (void *)tensor_data_f16_const(B),
        .data_type = dt,
        .size = {(size_t)N, (size_t)K},
    };
    BNNSNDArrayDescriptor descC = {
        .layout = BNNSDataLayoutRowMajorMatrix,
        .data = tensor_data_f16(C),
        .data_type = dt,
        .size = {(size_t)N, (size_t)M},
    };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    BNNSMatMul(transpose_A, transpose_B, 1.0f, &descA, &descB, &descC, NULL,
               NULL);
#pragma clang diagnostic pop
  }
}

static void accel_gemv(backend_t *backend, const tensor_t *A, const tensor_t *x,
                       tensor_t *y) {
  (void)backend;

  if (!A || !x || !y)
    return;

  int M = (int)tensor_dim(A, 0);
  int N = (int)tensor_dim(A, 1);

  if (A->dtype == DTYPE_F32) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    cblas_sgemv(CblasRowMajor, CblasNoTrans, M, N, 1.0f,
                tensor_data_f32_const(A), N, tensor_data_f32_const(x), 1, 0.0f,
                tensor_data_f32(y), 1);
#pragma clang diagnostic pop
  }
}

static const backend_ops_t accelerate_ops = {
    .name = "accelerate",
    .capability = CAP_ACCELERATE,
    .init = accel_init,
    .destroy = accel_destroy,
    .gemm = accel_gemm,
    .gemv = accel_gemv,
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

const backend_ops_t *accelerate_backend_ops(void) { return &accelerate_ops; }

__attribute__((constructor)) static void register_accelerate_backend(void) {
  backend_register(&accelerate_ops);
}

#else

const backend_ops_t *accelerate_backend_ops(void) { return NULL; }

#endif

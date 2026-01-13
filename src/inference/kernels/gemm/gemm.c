#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/gemm/gemm_kernels.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <Accelerate/Accelerate.h>
#define HAS_ACCELERATE 1
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

static int g_num_threads = 0;

static int get_cpu_count(void) {
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#else
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  return n > 0 ? (int)n : 1;
#endif
}

void gemm_set_num_threads(int num_threads) {
  if (num_threads <= 0) {
    g_num_threads = get_cpu_count();
  } else {
    g_num_threads = num_threads;
  }
}

int gemm_get_num_threads(void) {
  if (g_num_threads == 0) {
    g_num_threads = get_cpu_count();
  }
  return g_num_threads;
}

int gemm_get_max_threads(void) { return get_cpu_count(); }

#define MT_THRESHOLD_M 64
#define MT_THRESHOLD_FLOPS (64 * 64 * 64 * 2)

static inline uint16_t float_to_bf16_c(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t lsb = (bits >> 16) & 1;
  uint32_t rounding_bias = 0x7fff + lsb;
  bits += rounding_bias;
  return (uint16_t)(bits >> 16);
}

static inline float bf16_to_float_c(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_fp16_c(float x) {
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

static inline float fp16_to_float_c(uint16_t fp16) {
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

void bf16_array_to_f32(const uint16_t *src, float *dst, size_t count) {
  for (size_t i = 0; i < count; i++)
    dst[i] = bf16_to_float_c(src[i]);
}

void f32_array_to_bf16(const float *src, uint16_t *dst, size_t count) {
  for (size_t i = 0; i < count; i++)
    dst[i] = float_to_bf16_c(src[i]);
}

void f16_array_to_f32(const uint16_t *src, float *dst, size_t count) {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  size_t i = 0;
  for (; i + 8 <= count; i += 8) {
    float16x8_t f16 = vld1q_f16((const float16_t *)&src[i]);
    float32x4_t lo = vcvt_f32_f16(vget_low_f16(f16));
    float32x4_t hi = vcvt_f32_f16(vget_high_f16(f16));
    vst1q_f32(&dst[i], lo);
    vst1q_f32(&dst[i + 4], hi);
  }
  for (; i < count; i++)
    dst[i] = fp16_to_float_c(src[i]);
#else
  for (size_t i = 0; i < count; i++)
    dst[i] = fp16_to_float_c(src[i]);
#endif
}

void f32_array_to_f16(const float *src, uint16_t *dst, size_t count) {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  size_t i = 0;
  for (; i + 8 <= count; i += 8) {
    float32x4_t lo = vld1q_f32(&src[i]);
    float32x4_t hi = vld1q_f32(&src[i + 4]);
    float16x8_t f16 = vcombine_f16(vcvt_f16_f32(lo), vcvt_f16_f32(hi));
    vst1q_f16((float16_t *)&dst[i], f16);
  }
  for (; i < count; i++)
    dst[i] = float_to_fp16_c(src[i]);
#else
  for (size_t i = 0; i < count; i++)
    dst[i] = float_to_fp16_c(src[i]);
#endif
}

static void gemm_f32_naive(const float *A, const float *B, float *C, int M,
                           int N, int K, bool transpose_A, bool transpose_B) {
  memset(C, 0, M * N * sizeof(float));

  if (!transpose_A && !transpose_B) {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
          sum += A[i * K + k] * B[k * N + j];
        }
        C[i * N + j] = sum;
      }
    }
  } else if (transpose_A && !transpose_B) {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
          sum += A[k * M + i] * B[k * N + j];
        }
        C[i * N + j] = sum;
      }
    }
  } else if (!transpose_A && transpose_B) {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
          sum += A[i * K + k] * B[j * K + k];
        }
        C[i * N + j] = sum;
      }
    }
  } else {
    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
          sum += A[k * M + i] * B[j * K + k];
        }
        C[i * N + j] = sum;
      }
    }
  }
}

void gemm_f32(const float *A, const float *B, float *C, int M, int N, int K,
              bool transpose_A, bool transpose_B) {
  if (M <= 0 || N <= 0 || K <= 0)
    return;

#ifdef HAS_ACCELERATE
  enum CBLAS_TRANSPOSE transA = transpose_A ? CblasTrans : CblasNoTrans;
  enum CBLAS_TRANSPOSE transB = transpose_B ? CblasTrans : CblasNoTrans;
  int lda = transpose_A ? M : K;
  int ldb = transpose_B ? K : N;
  int ldc = N;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  cblas_sgemm(CblasRowMajor, transA, transB, M, N, K, 1.0f, A, lda, B, ldb,
              0.0f, C, ldc);
#pragma clang diagnostic pop
  return;
#endif

  gemm_caps_t caps = gemm_get_capabilities();

  if (caps.has_neon && !transpose_A && !transpose_B) {
    int nt = gemm_get_num_threads();
    long long flops = (long long)M * N * K * 2;
    if (nt > 1 && M >= MT_THRESHOLD_M && flops >= MT_THRESHOLD_FLOPS) {
      gemm_f32_kernel_mt(A, B, C, M, N, K, nt);
    } else {
      gemm_f32_kernel(A, B, C, M, N, K);
    }
    return;
  }

  gemm_f32_naive(A, B, C, M, N, K, transpose_A, transpose_B);
}

static void gemm_bf16_naive(const uint16_t *A, const uint16_t *B, uint16_t *C,
                            int M, int N, int K) {
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      float sum = 0.0f;
      for (int k = 0; k < K; k++) {
        sum += bf16_to_float_c(A[i * K + k]) * bf16_to_float_c(B[k * N + j]);
      }
      C[i * N + j] = float_to_bf16_c(sum);
    }
  }
}

void gemm_bf16(const uint16_t *A, const uint16_t *B, uint16_t *C, int M, int N,
               int K) {
  if (M <= 0 || N <= 0 || K <= 0)
    return;

  gemm_caps_t caps = gemm_get_capabilities();
  /* BF16: NEON is faster than AMX (AMX requires BF16->F32 conversion overhead)
   */
  if (caps.has_neon) {
    int nt = gemm_get_num_threads();
    long long flops = (long long)M * N * K * 2;
    if (nt > 1 && M >= MT_THRESHOLD_M && flops >= MT_THRESHOLD_FLOPS) {
      gemm_bf16_kernel_mt(A, B, C, M, N, K, nt);
    } else {
      gemm_bf16_kernel(A, B, C, M, N, K);
    }
    return;
  }

  gemm_bf16_naive(A, B, C, M, N, K);
}

static void gemm_f16_naive(const uint16_t *A, const uint16_t *B, uint16_t *C,
                           int M, int N, int K) {
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      float sum = 0.0f;
      for (int k = 0; k < K; k++) {
        sum += fp16_to_float_c(A[i * K + k]) * fp16_to_float_c(B[k * N + j]);
      }
      C[i * N + j] = float_to_fp16_c(sum);
    }
  }
}

void gemm_f16(const uint16_t *A, const uint16_t *B, uint16_t *C, int M, int N,
              int K) {
  if (M <= 0 || N <= 0 || K <= 0)
    return;

  gemm_caps_t caps = gemm_get_capabilities();

#ifdef HAS_ACCELERATE
  /*
   * Fast path for single-row GEMM (M=1, common in decode phase).
   * BNNS F16 MatMul is very slow for M=1 (~75ms for vocab projection).
   * Convert to F32 and use cblas_sgemv which is 6-8× faster.
   */
  if (M == 1 && N > 32000) { /* Only for vocab-scale (LM head) */
    float *A_f32 = (float *)malloc(K * sizeof(float));
    float *B_f32 = (float *)malloc((size_t)K * N * sizeof(float));
    float *C_f32 = (float *)malloc(N * sizeof(float));
    if (A_f32 && B_f32 && C_f32) {
      f16_array_to_f32(A, A_f32, K);
      f16_array_to_f32(B, B_f32, (size_t)K * N);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      /* B is K×N row-major, we want C = A × B = (1×K) × (K×N) = 1×N */
      cblas_sgemv(CblasRowMajor, CblasTrans, K, N, 1.0f, B_f32, N, A_f32, 1,
                  0.0f, C_f32, 1);
#pragma clang diagnostic pop
      f32_array_to_f16(C_f32, C, N);
      free(A_f32);
      free(B_f32);
      free(C_f32);
      return;
    }
    free(A_f32);
    free(B_f32);
    free(C_f32);
    /* Fall through to other paths if allocation failed */
  }
#endif

  if (caps.has_amx && M >= 32 && N >= 32) {
    int nt = gemm_get_num_threads();
    long long flops = (long long)M * N * K * 2;
    if (nt > 1 && M >= 64 && flops >= MT_THRESHOLD_FLOPS) {
      gemm_f16_kernel_amx_mt(A, B, C, M, N, K, nt);
    } else {
      gemm_f16_kernel_amx(A, B, C, M, N, K);
    }
    return;
  }

#ifdef HAS_ACCELERATE
  {
    BNNSNDArrayDescriptor descA = {
        .flags = BNNSNDArrayFlagBackpropSet,
        .layout = BNNSDataLayoutRowMajorMatrix,
        .size = {(size_t)K, (size_t)M, 0, 0, 0, 0, 0, 0},
        .stride = {1, (size_t)K, 0, 0, 0, 0, 0, 0},
        .data = (void *)A,
        .data_type = BNNSDataTypeFloat16,
        .table_data = NULL,
        .table_data_type = BNNSDataTypeFloat16,
        .data_scale = 1.0f,
        .data_bias = 0.0f,
    };
    BNNSNDArrayDescriptor descB = {
        .flags = BNNSNDArrayFlagBackpropSet,
        .layout = BNNSDataLayoutRowMajorMatrix,
        .size = {(size_t)N, (size_t)K, 0, 0, 0, 0, 0, 0},
        .stride = {1, (size_t)N, 0, 0, 0, 0, 0, 0},
        .data = (void *)B,
        .data_type = BNNSDataTypeFloat16,
        .table_data = NULL,
        .table_data_type = BNNSDataTypeFloat16,
        .data_scale = 1.0f,
        .data_bias = 0.0f,
    };
    BNNSNDArrayDescriptor descC = {
        .flags = BNNSNDArrayFlagBackpropSet,
        .layout = BNNSDataLayoutRowMajorMatrix,
        .size = {(size_t)N, (size_t)M, 0, 0, 0, 0, 0, 0},
        .stride = {1, (size_t)N, 0, 0, 0, 0, 0, 0},
        .data = (void *)C,
        .data_type = BNNSDataTypeFloat16,
        .table_data = NULL,
        .table_data_type = BNNSDataTypeFloat16,
        .data_scale = 1.0f,
        .data_bias = 0.0f,
    };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    int result =
        BNNSMatMul(false, false, 1.0f, &descA, &descB, &descC, NULL, NULL);
#pragma clang diagnostic pop
    if (result == 0) {
      return;
    }
  }

  long long total_elements =
      (long long)M * K + (long long)K * N + (long long)M * N;
  if (total_elements > 1024) {
    float *A_f32 = (float *)malloc(M * K * sizeof(float));
    float *B_f32 = (float *)malloc(K * N * sizeof(float));
    float *C_f32 = (float *)malloc(M * N * sizeof(float));
    if (A_f32 && B_f32 && C_f32) {
      f16_array_to_f32(A, A_f32, M * K);
      f16_array_to_f32(B, B_f32, K * N);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K, 1.0f,
                  A_f32, K, B_f32, N, 0.0f, C_f32, N);
#pragma clang diagnostic pop
      f32_array_to_f16(C_f32, C, M * N);
      free(A_f32);
      free(B_f32);
      free(C_f32);
      return;
    }
    if (A_f32)
      free(A_f32);
    if (B_f32)
      free(B_f32);
    if (C_f32)
      free(C_f32);
  }
#endif

  if (caps.has_neon) {
    int nt = gemm_get_num_threads();
    long long flops = (long long)M * N * K * 2;
    if (nt > 1 && M >= MT_THRESHOLD_M && flops >= MT_THRESHOLD_FLOPS) {
      gemm_f16_kernel_mt(A, B, C, M, N, K, nt);
    } else {
      gemm_f16_kernel(A, B, C, M, N, K);
    }
    return;
  }

  gemm_f16_naive(A, B, C, M, N, K);
}

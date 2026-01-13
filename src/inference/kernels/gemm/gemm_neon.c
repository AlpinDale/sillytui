#include "inference/backend/caps.h"
#include "inference/kernels/gemm/gemm_kernels.h"
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <pthread.h>
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

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

gemm_caps_t gemm_get_capabilities(void) {
  gemm_caps_t caps = {0};
  caps.has_neon = caps_has(CAP_NEON);
  caps.has_amx = caps_has(CAP_AMX);
  caps.has_avx2 = caps_has(CAP_AVX2);
  caps.has_avx512 = caps_has(CAP_AVX512);
  return caps;
}

#if HAS_NEON

static inline void gemv_f32_neon(const float *A, const float *B, float *C,
                                 int N, int K) {
  int j = 0;
  for (; j + 16 <= N; j += 16) {
    float32x4_t s0 = vdupq_n_f32(0.0f);
    float32x4_t s1 = vdupq_n_f32(0.0f);
    float32x4_t s2 = vdupq_n_f32(0.0f);
    float32x4_t s3 = vdupq_n_f32(0.0f);

    int k = 0;
    for (; k + 4 <= K; k += 4) {
      __builtin_prefetch(&B[(k + 8) * N + j], 0, 3);
      float32x4_t a = vld1q_f32(&A[k]);

      float32x4_t b00 = vld1q_f32(&B[k * N + j]);
      float32x4_t b01 = vld1q_f32(&B[k * N + j + 4]);
      float32x4_t b02 = vld1q_f32(&B[k * N + j + 8]);
      float32x4_t b03 = vld1q_f32(&B[k * N + j + 12]);
      s0 = vfmaq_laneq_f32(s0, b00, a, 0);
      s1 = vfmaq_laneq_f32(s1, b01, a, 0);
      s2 = vfmaq_laneq_f32(s2, b02, a, 0);
      s3 = vfmaq_laneq_f32(s3, b03, a, 0);

      b00 = vld1q_f32(&B[(k + 1) * N + j]);
      b01 = vld1q_f32(&B[(k + 1) * N + j + 4]);
      b02 = vld1q_f32(&B[(k + 1) * N + j + 8]);
      b03 = vld1q_f32(&B[(k + 1) * N + j + 12]);
      s0 = vfmaq_laneq_f32(s0, b00, a, 1);
      s1 = vfmaq_laneq_f32(s1, b01, a, 1);
      s2 = vfmaq_laneq_f32(s2, b02, a, 1);
      s3 = vfmaq_laneq_f32(s3, b03, a, 1);

      b00 = vld1q_f32(&B[(k + 2) * N + j]);
      b01 = vld1q_f32(&B[(k + 2) * N + j + 4]);
      b02 = vld1q_f32(&B[(k + 2) * N + j + 8]);
      b03 = vld1q_f32(&B[(k + 2) * N + j + 12]);
      s0 = vfmaq_laneq_f32(s0, b00, a, 2);
      s1 = vfmaq_laneq_f32(s1, b01, a, 2);
      s2 = vfmaq_laneq_f32(s2, b02, a, 2);
      s3 = vfmaq_laneq_f32(s3, b03, a, 2);

      b00 = vld1q_f32(&B[(k + 3) * N + j]);
      b01 = vld1q_f32(&B[(k + 3) * N + j + 4]);
      b02 = vld1q_f32(&B[(k + 3) * N + j + 8]);
      b03 = vld1q_f32(&B[(k + 3) * N + j + 12]);
      s0 = vfmaq_laneq_f32(s0, b00, a, 3);
      s1 = vfmaq_laneq_f32(s1, b01, a, 3);
      s2 = vfmaq_laneq_f32(s2, b02, a, 3);
      s3 = vfmaq_laneq_f32(s3, b03, a, 3);
    }

    for (; k < K; k++) {
      float av = A[k];
      s0 = vfmaq_n_f32(s0, vld1q_f32(&B[k * N + j]), av);
      s1 = vfmaq_n_f32(s1, vld1q_f32(&B[k * N + j + 4]), av);
      s2 = vfmaq_n_f32(s2, vld1q_f32(&B[k * N + j + 8]), av);
      s3 = vfmaq_n_f32(s3, vld1q_f32(&B[k * N + j + 12]), av);
    }

    vst1q_f32(&C[j], s0);
    vst1q_f32(&C[j + 4], s1);
    vst1q_f32(&C[j + 8], s2);
    vst1q_f32(&C[j + 12], s3);
  }

  for (; j < N; j++) {
    float sum = 0.0f;
    for (int k = 0; k < K; k++) {
      sum += A[k] * B[k * N + j];
    }
    C[j] = sum;
  }
}

static inline void micro_kernel_8x8_neon(const float *A, const float *B,
                                         float *C, int M, int N, int K, int lda,
                                         int ldb, int ldc, int mi, int ni) {
  float32x4_t c00 = vdupq_n_f32(0.0f);
  float32x4_t c01 = vdupq_n_f32(0.0f);
  float32x4_t c10 = vdupq_n_f32(0.0f);
  float32x4_t c11 = vdupq_n_f32(0.0f);
  float32x4_t c20 = vdupq_n_f32(0.0f);
  float32x4_t c21 = vdupq_n_f32(0.0f);
  float32x4_t c30 = vdupq_n_f32(0.0f);
  float32x4_t c31 = vdupq_n_f32(0.0f);
  float32x4_t c40 = vdupq_n_f32(0.0f);
  float32x4_t c41 = vdupq_n_f32(0.0f);
  float32x4_t c50 = vdupq_n_f32(0.0f);
  float32x4_t c51 = vdupq_n_f32(0.0f);
  float32x4_t c60 = vdupq_n_f32(0.0f);
  float32x4_t c61 = vdupq_n_f32(0.0f);
  float32x4_t c70 = vdupq_n_f32(0.0f);
  float32x4_t c71 = vdupq_n_f32(0.0f);

  int actual_m = (mi + 8 > M) ? M - mi : 8;
  int actual_n = (ni + 8 > N) ? N - ni : 8;

  if (actual_m == 8 && actual_n == 8) {
    int k = 0;
    for (; k + 4 <= K; k += 4) {
      __builtin_prefetch(&B[(k + 8) * ldb + ni], 0, 3);

#define KERNEL_ITER(kk)                                                        \
  do {                                                                         \
    float32x4_t b0 = vld1q_f32(&B[(k + kk) * ldb + ni]);                       \
    float32x4_t b1 = vld1q_f32(&B[(k + kk) * ldb + ni + 4]);                   \
    float a0 = A[(mi + 0) * lda + k + kk];                                     \
    float a1 = A[(mi + 1) * lda + k + kk];                                     \
    float a2 = A[(mi + 2) * lda + k + kk];                                     \
    float a3 = A[(mi + 3) * lda + k + kk];                                     \
    float a4 = A[(mi + 4) * lda + k + kk];                                     \
    float a5 = A[(mi + 5) * lda + k + kk];                                     \
    float a6 = A[(mi + 6) * lda + k + kk];                                     \
    float a7 = A[(mi + 7) * lda + k + kk];                                     \
    c00 = vfmaq_n_f32(c00, b0, a0);                                            \
    c01 = vfmaq_n_f32(c01, b1, a0);                                            \
    c10 = vfmaq_n_f32(c10, b0, a1);                                            \
    c11 = vfmaq_n_f32(c11, b1, a1);                                            \
    c20 = vfmaq_n_f32(c20, b0, a2);                                            \
    c21 = vfmaq_n_f32(c21, b1, a2);                                            \
    c30 = vfmaq_n_f32(c30, b0, a3);                                            \
    c31 = vfmaq_n_f32(c31, b1, a3);                                            \
    c40 = vfmaq_n_f32(c40, b0, a4);                                            \
    c41 = vfmaq_n_f32(c41, b1, a4);                                            \
    c50 = vfmaq_n_f32(c50, b0, a5);                                            \
    c51 = vfmaq_n_f32(c51, b1, a5);                                            \
    c60 = vfmaq_n_f32(c60, b0, a6);                                            \
    c61 = vfmaq_n_f32(c61, b1, a6);                                            \
    c70 = vfmaq_n_f32(c70, b0, a7);                                            \
    c71 = vfmaq_n_f32(c71, b1, a7);                                            \
  } while (0)

      KERNEL_ITER(0);
      KERNEL_ITER(1);
      KERNEL_ITER(2);
      KERNEL_ITER(3);

#undef KERNEL_ITER
    }

    for (; k < K; k++) {
      float32x4_t b0 = vld1q_f32(&B[k * ldb + ni]);
      float32x4_t b1 = vld1q_f32(&B[k * ldb + ni + 4]);
      float a0 = A[(mi + 0) * lda + k];
      float a1 = A[(mi + 1) * lda + k];
      float a2 = A[(mi + 2) * lda + k];
      float a3 = A[(mi + 3) * lda + k];
      float a4 = A[(mi + 4) * lda + k];
      float a5 = A[(mi + 5) * lda + k];
      float a6 = A[(mi + 6) * lda + k];
      float a7 = A[(mi + 7) * lda + k];
      c00 = vfmaq_n_f32(c00, b0, a0);
      c01 = vfmaq_n_f32(c01, b1, a0);
      c10 = vfmaq_n_f32(c10, b0, a1);
      c11 = vfmaq_n_f32(c11, b1, a1);
      c20 = vfmaq_n_f32(c20, b0, a2);
      c21 = vfmaq_n_f32(c21, b1, a2);
      c30 = vfmaq_n_f32(c30, b0, a3);
      c31 = vfmaq_n_f32(c31, b1, a3);
      c40 = vfmaq_n_f32(c40, b0, a4);
      c41 = vfmaq_n_f32(c41, b1, a4);
      c50 = vfmaq_n_f32(c50, b0, a5);
      c51 = vfmaq_n_f32(c51, b1, a5);
      c60 = vfmaq_n_f32(c60, b0, a6);
      c61 = vfmaq_n_f32(c61, b1, a6);
      c70 = vfmaq_n_f32(c70, b0, a7);
      c71 = vfmaq_n_f32(c71, b1, a7);
    }

    vst1q_f32(&C[(mi + 0) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 0) * ldc + ni]), c00));
    vst1q_f32(&C[(mi + 0) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 0) * ldc + ni + 4]), c01));
    vst1q_f32(&C[(mi + 1) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 1) * ldc + ni]), c10));
    vst1q_f32(&C[(mi + 1) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 1) * ldc + ni + 4]), c11));
    vst1q_f32(&C[(mi + 2) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 2) * ldc + ni]), c20));
    vst1q_f32(&C[(mi + 2) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 2) * ldc + ni + 4]), c21));
    vst1q_f32(&C[(mi + 3) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 3) * ldc + ni]), c30));
    vst1q_f32(&C[(mi + 3) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 3) * ldc + ni + 4]), c31));
    vst1q_f32(&C[(mi + 4) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 4) * ldc + ni]), c40));
    vst1q_f32(&C[(mi + 4) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 4) * ldc + ni + 4]), c41));
    vst1q_f32(&C[(mi + 5) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 5) * ldc + ni]), c50));
    vst1q_f32(&C[(mi + 5) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 5) * ldc + ni + 4]), c51));
    vst1q_f32(&C[(mi + 6) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 6) * ldc + ni]), c60));
    vst1q_f32(&C[(mi + 6) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 6) * ldc + ni + 4]), c61));
    vst1q_f32(&C[(mi + 7) * ldc + ni],
              vaddq_f32(vld1q_f32(&C[(mi + 7) * ldc + ni]), c70));
    vst1q_f32(&C[(mi + 7) * ldc + ni + 4],
              vaddq_f32(vld1q_f32(&C[(mi + 7) * ldc + ni + 4]), c71));
  } else {
    for (int i = 0; i < actual_m; i++) {
      for (int j = 0; j < actual_n; j++) {
        float sum = 0.0f;
        for (int k = 0; k < K; k++) {
          sum += A[(mi + i) * lda + k] * B[k * ldb + ni + j];
        }
        C[(mi + i) * ldc + ni + j] += sum;
      }
    }
  }
}

void gemm_f32_kernel(const float *A, const float *B, float *C, int M, int N,
                     int K) {
  memset(C, 0, M * N * sizeof(float));

  if (M == 1) {
    gemv_f32_neon(A, B, C, N, K);
    return;
  }

  for (int mi = 0; mi < M; mi += 8) {
    for (int ni = 0; ni < N; ni += 8) {
      micro_kernel_8x8_neon(A, B, C, M, N, K, K, N, N, mi, ni);
    }
  }
}

static inline void bf16x8_to_f32x8(const uint16_t *src, float32x4_t *lo,
                                   float32x4_t *hi) {
  uint16x8_t bf16 = vld1q_u16(src);
  *lo = vreinterpretq_f32_u32(vshll_n_u16(vget_low_u16(bf16), 16));
  *hi = vreinterpretq_f32_u32(vshll_n_u16(vget_high_u16(bf16), 16));
}

static inline float32x4_t bf16x4_to_f32x4(const uint16_t *src) {
  uint16x4_t bf16 = vld1_u16(src);
  return vreinterpretq_f32_u32(vshll_n_u16(bf16, 16));
}

static inline void f32x8_to_bf16x8(float32x4_t lo, float32x4_t hi,
                                   uint16_t *dst) {
  uint32x4_t lo_u32 = vreinterpretq_u32_f32(lo);
  uint32x4_t hi_u32 = vreinterpretq_u32_f32(hi);
  uint32x4_t bias = vdupq_n_u32(0x7fff);
  uint32x4_t lsb_lo = vshrq_n_u32(lo_u32, 16);
  uint32x4_t lsb_hi = vshrq_n_u32(hi_u32, 16);
  lsb_lo = vandq_u32(lsb_lo, vdupq_n_u32(1));
  lsb_hi = vandq_u32(lsb_hi, vdupq_n_u32(1));
  lo_u32 = vaddq_u32(lo_u32, vaddq_u32(bias, lsb_lo));
  hi_u32 = vaddq_u32(hi_u32, vaddq_u32(bias, lsb_hi));
  uint16x4_t lo_16 = vshrn_n_u32(lo_u32, 16);
  uint16x4_t hi_16 = vshrn_n_u32(hi_u32, 16);
  vst1_u16(&dst[0], lo_16);
  vst1_u16(&dst[4], hi_16);
}

static void micro_kernel_bf16_8x8_neon(const uint16_t *A, const uint16_t *B,
                                       uint16_t *C, int M, int N, int K, int mi,
                                       int ni) {
  float32x4_t c00 = vdupq_n_f32(0.0f), c01 = vdupq_n_f32(0.0f);
  float32x4_t c10 = vdupq_n_f32(0.0f), c11 = vdupq_n_f32(0.0f);
  float32x4_t c20 = vdupq_n_f32(0.0f), c21 = vdupq_n_f32(0.0f);
  float32x4_t c30 = vdupq_n_f32(0.0f), c31 = vdupq_n_f32(0.0f);
  float32x4_t c40 = vdupq_n_f32(0.0f), c41 = vdupq_n_f32(0.0f);
  float32x4_t c50 = vdupq_n_f32(0.0f), c51 = vdupq_n_f32(0.0f);
  float32x4_t c60 = vdupq_n_f32(0.0f), c61 = vdupq_n_f32(0.0f);
  float32x4_t c70 = vdupq_n_f32(0.0f), c71 = vdupq_n_f32(0.0f);

  int actual_m = (mi + 8 > M) ? M - mi : 8;
  int actual_n = (ni + 8 > N) ? N - ni : 8;

  if (actual_m == 8 && actual_n == 8) {
    int k = 0;
    for (; k + 4 <= K; k += 4) {
      __builtin_prefetch(&B[(k + 8) * N + ni], 0, 3);

      float32x4_t b00, b01, b10, b11, b20, b21, b30, b31;
      bf16x8_to_f32x8(&B[(k + 0) * N + ni], &b00, &b01);
      bf16x8_to_f32x8(&B[(k + 1) * N + ni], &b10, &b11);
      bf16x8_to_f32x8(&B[(k + 2) * N + ni], &b20, &b21);
      bf16x8_to_f32x8(&B[(k + 3) * N + ni], &b30, &b31);

#define BF16_ROW_ACC(row, c0, c1)                                              \
  do {                                                                         \
    float32x4_t av = bf16x4_to_f32x4(&A[(mi + row) * K + k]);                  \
    c0 = vfmaq_laneq_f32(c0, b00, av, 0);                                      \
    c1 = vfmaq_laneq_f32(c1, b01, av, 0);                                      \
    c0 = vfmaq_laneq_f32(c0, b10, av, 1);                                      \
    c1 = vfmaq_laneq_f32(c1, b11, av, 1);                                      \
    c0 = vfmaq_laneq_f32(c0, b20, av, 2);                                      \
    c1 = vfmaq_laneq_f32(c1, b21, av, 2);                                      \
    c0 = vfmaq_laneq_f32(c0, b30, av, 3);                                      \
    c1 = vfmaq_laneq_f32(c1, b31, av, 3);                                      \
  } while (0)

      BF16_ROW_ACC(0, c00, c01);
      BF16_ROW_ACC(1, c10, c11);
      BF16_ROW_ACC(2, c20, c21);
      BF16_ROW_ACC(3, c30, c31);
      BF16_ROW_ACC(4, c40, c41);
      BF16_ROW_ACC(5, c50, c51);
      BF16_ROW_ACC(6, c60, c61);
      BF16_ROW_ACC(7, c70, c71);

#undef BF16_ROW_ACC
    }

    for (; k < K; k++) {
      float32x4_t b0, b1;
      bf16x8_to_f32x8(&B[k * N + ni], &b0, &b1);
      float a0 = bf16_to_float_c(A[(mi + 0) * K + k]);
      float a1 = bf16_to_float_c(A[(mi + 1) * K + k]);
      float a2 = bf16_to_float_c(A[(mi + 2) * K + k]);
      float a3 = bf16_to_float_c(A[(mi + 3) * K + k]);
      float a4 = bf16_to_float_c(A[(mi + 4) * K + k]);
      float a5 = bf16_to_float_c(A[(mi + 5) * K + k]);
      float a6 = bf16_to_float_c(A[(mi + 6) * K + k]);
      float a7 = bf16_to_float_c(A[(mi + 7) * K + k]);
      c00 = vfmaq_n_f32(c00, b0, a0);
      c01 = vfmaq_n_f32(c01, b1, a0);
      c10 = vfmaq_n_f32(c10, b0, a1);
      c11 = vfmaq_n_f32(c11, b1, a1);
      c20 = vfmaq_n_f32(c20, b0, a2);
      c21 = vfmaq_n_f32(c21, b1, a2);
      c30 = vfmaq_n_f32(c30, b0, a3);
      c31 = vfmaq_n_f32(c31, b1, a3);
      c40 = vfmaq_n_f32(c40, b0, a4);
      c41 = vfmaq_n_f32(c41, b1, a4);
      c50 = vfmaq_n_f32(c50, b0, a5);
      c51 = vfmaq_n_f32(c51, b1, a5);
      c60 = vfmaq_n_f32(c60, b0, a6);
      c61 = vfmaq_n_f32(c61, b1, a6);
      c70 = vfmaq_n_f32(c70, b0, a7);
      c71 = vfmaq_n_f32(c71, b1, a7);
    }

    f32x8_to_bf16x8(c00, c01, &C[(mi + 0) * N + ni]);
    f32x8_to_bf16x8(c10, c11, &C[(mi + 1) * N + ni]);
    f32x8_to_bf16x8(c20, c21, &C[(mi + 2) * N + ni]);
    f32x8_to_bf16x8(c30, c31, &C[(mi + 3) * N + ni]);
    f32x8_to_bf16x8(c40, c41, &C[(mi + 4) * N + ni]);
    f32x8_to_bf16x8(c50, c51, &C[(mi + 5) * N + ni]);
    f32x8_to_bf16x8(c60, c61, &C[(mi + 6) * N + ni]);
    f32x8_to_bf16x8(c70, c71, &C[(mi + 7) * N + ni]);
  } else {
    for (int i = 0; i < actual_m; i++) {
      for (int j = 0; j < actual_n; j++) {
        float sum = 0.0f;
        for (int kk = 0; kk < K; kk++) {
          sum += bf16_to_float_c(A[(mi + i) * K + kk]) *
                 bf16_to_float_c(B[kk * N + ni + j]);
        }
        C[(mi + i) * N + ni + j] = float_to_bf16_c(sum);
      }
    }
  }
}

void gemm_bf16_kernel(const uint16_t *A, const uint16_t *B, uint16_t *C, int M,
                      int N, int K) {
  if (M == 1) {
    int j = 0;
    for (; j + 16 <= N; j += 16) {
      float32x4_t s00 = vdupq_n_f32(0.0f), s01 = vdupq_n_f32(0.0f);
      float32x4_t s10 = vdupq_n_f32(0.0f), s11 = vdupq_n_f32(0.0f);

      int k = 0;
      for (; k + 4 <= K; k += 4) {
        __builtin_prefetch(&B[(k + 8) * N + j], 0, 3);
        float32x4_t av = bf16x4_to_f32x4(&A[k]);

        float32x4_t b00, b01, b02, b03;
        bf16x8_to_f32x8(&B[k * N + j], &b00, &b01);
        bf16x8_to_f32x8(&B[k * N + j + 8], &b02, &b03);
        s00 = vfmaq_laneq_f32(s00, b00, av, 0);
        s01 = vfmaq_laneq_f32(s01, b01, av, 0);
        s10 = vfmaq_laneq_f32(s10, b02, av, 0);
        s11 = vfmaq_laneq_f32(s11, b03, av, 0);

        bf16x8_to_f32x8(&B[(k + 1) * N + j], &b00, &b01);
        bf16x8_to_f32x8(&B[(k + 1) * N + j + 8], &b02, &b03);
        s00 = vfmaq_laneq_f32(s00, b00, av, 1);
        s01 = vfmaq_laneq_f32(s01, b01, av, 1);
        s10 = vfmaq_laneq_f32(s10, b02, av, 1);
        s11 = vfmaq_laneq_f32(s11, b03, av, 1);

        bf16x8_to_f32x8(&B[(k + 2) * N + j], &b00, &b01);
        bf16x8_to_f32x8(&B[(k + 2) * N + j + 8], &b02, &b03);
        s00 = vfmaq_laneq_f32(s00, b00, av, 2);
        s01 = vfmaq_laneq_f32(s01, b01, av, 2);
        s10 = vfmaq_laneq_f32(s10, b02, av, 2);
        s11 = vfmaq_laneq_f32(s11, b03, av, 2);

        bf16x8_to_f32x8(&B[(k + 3) * N + j], &b00, &b01);
        bf16x8_to_f32x8(&B[(k + 3) * N + j + 8], &b02, &b03);
        s00 = vfmaq_laneq_f32(s00, b00, av, 3);
        s01 = vfmaq_laneq_f32(s01, b01, av, 3);
        s10 = vfmaq_laneq_f32(s10, b02, av, 3);
        s11 = vfmaq_laneq_f32(s11, b03, av, 3);
      }

      for (; k < K; k++) {
        float a_val = bf16_to_float_c(A[k]);
        float32x4_t b0, b1, b2, b3;
        bf16x8_to_f32x8(&B[k * N + j], &b0, &b1);
        bf16x8_to_f32x8(&B[k * N + j + 8], &b2, &b3);
        s00 = vfmaq_n_f32(s00, b0, a_val);
        s01 = vfmaq_n_f32(s01, b1, a_val);
        s10 = vfmaq_n_f32(s10, b2, a_val);
        s11 = vfmaq_n_f32(s11, b3, a_val);
      }

      f32x8_to_bf16x8(s00, s01, &C[j]);
      f32x8_to_bf16x8(s10, s11, &C[j + 8]);
    }

    for (; j < N; j++) {
      float sum = 0.0f;
      for (int k2 = 0; k2 < K; k2++) {
        sum += bf16_to_float_c(A[k2]) * bf16_to_float_c(B[k2 * N + j]);
      }
      C[j] = float_to_bf16_c(sum);
    }
    return;
  }

  for (int mi = 0; mi < M; mi += 8) {
    for (int ni = 0; ni < N; ni += 8) {
      micro_kernel_bf16_8x8_neon(A, B, C, M, N, K, mi, ni);
    }
  }
}

static void micro_kernel_f16_8x8_neon(const uint16_t *A, const uint16_t *B,
                                      uint16_t *C, int M, int N, int K, int mi,
                                      int ni) {
  float32x4_t c00 = vdupq_n_f32(0.0f), c01 = vdupq_n_f32(0.0f);
  float32x4_t c10 = vdupq_n_f32(0.0f), c11 = vdupq_n_f32(0.0f);
  float32x4_t c20 = vdupq_n_f32(0.0f), c21 = vdupq_n_f32(0.0f);
  float32x4_t c30 = vdupq_n_f32(0.0f), c31 = vdupq_n_f32(0.0f);
  float32x4_t c40 = vdupq_n_f32(0.0f), c41 = vdupq_n_f32(0.0f);
  float32x4_t c50 = vdupq_n_f32(0.0f), c51 = vdupq_n_f32(0.0f);
  float32x4_t c60 = vdupq_n_f32(0.0f), c61 = vdupq_n_f32(0.0f);
  float32x4_t c70 = vdupq_n_f32(0.0f), c71 = vdupq_n_f32(0.0f);

  int actual_m = (mi + 8 > M) ? M - mi : 8;
  int actual_n = (ni + 8 > N) ? N - ni : 8;

  if (actual_m == 8 && actual_n == 8) {
    int k = 0;
    for (; k + 4 <= K; k += 4) {
      __builtin_prefetch(&B[(k + 8) * N + ni], 0, 3);

      float16x8_t bf0 = vld1q_f16((const float16_t *)&B[(k + 0) * N + ni]);
      float16x8_t bf1 = vld1q_f16((const float16_t *)&B[(k + 1) * N + ni]);
      float16x8_t bf2 = vld1q_f16((const float16_t *)&B[(k + 2) * N + ni]);
      float16x8_t bf3 = vld1q_f16((const float16_t *)&B[(k + 3) * N + ni]);
      float32x4_t b00 = vcvt_f32_f16(vget_low_f16(bf0));
      float32x4_t b01 = vcvt_f32_f16(vget_high_f16(bf0));
      float32x4_t b10 = vcvt_f32_f16(vget_low_f16(bf1));
      float32x4_t b11 = vcvt_f32_f16(vget_high_f16(bf1));
      float32x4_t b20 = vcvt_f32_f16(vget_low_f16(bf2));
      float32x4_t b21 = vcvt_f32_f16(vget_high_f16(bf2));
      float32x4_t b30 = vcvt_f32_f16(vget_low_f16(bf3));
      float32x4_t b31 = vcvt_f32_f16(vget_high_f16(bf3));

#define F16_ROW_ACC(row, c0, c1)                                               \
  do {                                                                         \
    float16x4_t a_f16 = vld1_f16((const float16_t *)&A[(mi + row) * K + k]);   \
    float32x4_t av = vcvt_f32_f16(a_f16);                                      \
    c0 = vfmaq_laneq_f32(c0, b00, av, 0);                                      \
    c1 = vfmaq_laneq_f32(c1, b01, av, 0);                                      \
    c0 = vfmaq_laneq_f32(c0, b10, av, 1);                                      \
    c1 = vfmaq_laneq_f32(c1, b11, av, 1);                                      \
    c0 = vfmaq_laneq_f32(c0, b20, av, 2);                                      \
    c1 = vfmaq_laneq_f32(c1, b21, av, 2);                                      \
    c0 = vfmaq_laneq_f32(c0, b30, av, 3);                                      \
    c1 = vfmaq_laneq_f32(c1, b31, av, 3);                                      \
  } while (0)

      F16_ROW_ACC(0, c00, c01);
      F16_ROW_ACC(1, c10, c11);
      F16_ROW_ACC(2, c20, c21);
      F16_ROW_ACC(3, c30, c31);
      F16_ROW_ACC(4, c40, c41);
      F16_ROW_ACC(5, c50, c51);
      F16_ROW_ACC(6, c60, c61);
      F16_ROW_ACC(7, c70, c71);

#undef F16_ROW_ACC
    }

    for (; k < K; k++) {
      float16x8_t b_f16 = vld1q_f16((const float16_t *)&B[k * N + ni]);
      float32x4_t b0 = vcvt_f32_f16(vget_low_f16(b_f16));
      float32x4_t b1 = vcvt_f32_f16(vget_high_f16(b_f16));
      float a0 = fp16_to_float_c(A[(mi + 0) * K + k]);
      float a1 = fp16_to_float_c(A[(mi + 1) * K + k]);
      float a2 = fp16_to_float_c(A[(mi + 2) * K + k]);
      float a3 = fp16_to_float_c(A[(mi + 3) * K + k]);
      float a4 = fp16_to_float_c(A[(mi + 4) * K + k]);
      float a5 = fp16_to_float_c(A[(mi + 5) * K + k]);
      float a6 = fp16_to_float_c(A[(mi + 6) * K + k]);
      float a7 = fp16_to_float_c(A[(mi + 7) * K + k]);
      c00 = vfmaq_n_f32(c00, b0, a0);
      c01 = vfmaq_n_f32(c01, b1, a0);
      c10 = vfmaq_n_f32(c10, b0, a1);
      c11 = vfmaq_n_f32(c11, b1, a1);
      c20 = vfmaq_n_f32(c20, b0, a2);
      c21 = vfmaq_n_f32(c21, b1, a2);
      c30 = vfmaq_n_f32(c30, b0, a3);
      c31 = vfmaq_n_f32(c31, b1, a3);
      c40 = vfmaq_n_f32(c40, b0, a4);
      c41 = vfmaq_n_f32(c41, b1, a4);
      c50 = vfmaq_n_f32(c50, b0, a5);
      c51 = vfmaq_n_f32(c51, b1, a5);
      c60 = vfmaq_n_f32(c60, b0, a6);
      c61 = vfmaq_n_f32(c61, b1, a6);
      c70 = vfmaq_n_f32(c70, b0, a7);
      c71 = vfmaq_n_f32(c71, b1, a7);
    }

    vst1q_f16((float16_t *)&C[(mi + 0) * N + ni],
              vcombine_f16(vcvt_f16_f32(c00), vcvt_f16_f32(c01)));
    vst1q_f16((float16_t *)&C[(mi + 1) * N + ni],
              vcombine_f16(vcvt_f16_f32(c10), vcvt_f16_f32(c11)));
    vst1q_f16((float16_t *)&C[(mi + 2) * N + ni],
              vcombine_f16(vcvt_f16_f32(c20), vcvt_f16_f32(c21)));
    vst1q_f16((float16_t *)&C[(mi + 3) * N + ni],
              vcombine_f16(vcvt_f16_f32(c30), vcvt_f16_f32(c31)));
    vst1q_f16((float16_t *)&C[(mi + 4) * N + ni],
              vcombine_f16(vcvt_f16_f32(c40), vcvt_f16_f32(c41)));
    vst1q_f16((float16_t *)&C[(mi + 5) * N + ni],
              vcombine_f16(vcvt_f16_f32(c50), vcvt_f16_f32(c51)));
    vst1q_f16((float16_t *)&C[(mi + 6) * N + ni],
              vcombine_f16(vcvt_f16_f32(c60), vcvt_f16_f32(c61)));
    vst1q_f16((float16_t *)&C[(mi + 7) * N + ni],
              vcombine_f16(vcvt_f16_f32(c70), vcvt_f16_f32(c71)));
  } else {
    for (int i = 0; i < actual_m; i++) {
      for (int j = 0; j < actual_n; j++) {
        float sum = 0.0f;
        for (int kk = 0; kk < K; kk++) {
          sum += fp16_to_float_c(A[(mi + i) * K + kk]) *
                 fp16_to_float_c(B[kk * N + ni + j]);
        }
        C[(mi + i) * N + ni + j] = float_to_fp16_c(sum);
      }
    }
  }
}

void gemm_f16_kernel(const uint16_t *A, const uint16_t *B, uint16_t *C, int M,
                     int N, int K) {
  if (M == 1) {
    int j = 0;
    for (; j + 16 <= N; j += 16) {
      float32x4_t s00 = vdupq_n_f32(0.0f), s01 = vdupq_n_f32(0.0f);
      float32x4_t s10 = vdupq_n_f32(0.0f), s11 = vdupq_n_f32(0.0f);

      int k = 0;
      for (; k + 4 <= K; k += 4) {
        __builtin_prefetch(&B[(k + 8) * N + j], 0, 3);
        float16x4_t a_f16 = vld1_f16((const float16_t *)&A[k]);
        float32x4_t av = vcvt_f32_f16(a_f16);

        float16x8_t b0 = vld1q_f16((const float16_t *)&B[k * N + j]);
        float16x8_t b1 = vld1q_f16((const float16_t *)&B[k * N + j + 8]);
        s00 = vfmaq_laneq_f32(s00, vcvt_f32_f16(vget_low_f16(b0)), av, 0);
        s01 = vfmaq_laneq_f32(s01, vcvt_f32_f16(vget_high_f16(b0)), av, 0);
        s10 = vfmaq_laneq_f32(s10, vcvt_f32_f16(vget_low_f16(b1)), av, 0);
        s11 = vfmaq_laneq_f32(s11, vcvt_f32_f16(vget_high_f16(b1)), av, 0);

        b0 = vld1q_f16((const float16_t *)&B[(k + 1) * N + j]);
        b1 = vld1q_f16((const float16_t *)&B[(k + 1) * N + j + 8]);
        s00 = vfmaq_laneq_f32(s00, vcvt_f32_f16(vget_low_f16(b0)), av, 1);
        s01 = vfmaq_laneq_f32(s01, vcvt_f32_f16(vget_high_f16(b0)), av, 1);
        s10 = vfmaq_laneq_f32(s10, vcvt_f32_f16(vget_low_f16(b1)), av, 1);
        s11 = vfmaq_laneq_f32(s11, vcvt_f32_f16(vget_high_f16(b1)), av, 1);

        b0 = vld1q_f16((const float16_t *)&B[(k + 2) * N + j]);
        b1 = vld1q_f16((const float16_t *)&B[(k + 2) * N + j + 8]);
        s00 = vfmaq_laneq_f32(s00, vcvt_f32_f16(vget_low_f16(b0)), av, 2);
        s01 = vfmaq_laneq_f32(s01, vcvt_f32_f16(vget_high_f16(b0)), av, 2);
        s10 = vfmaq_laneq_f32(s10, vcvt_f32_f16(vget_low_f16(b1)), av, 2);
        s11 = vfmaq_laneq_f32(s11, vcvt_f32_f16(vget_high_f16(b1)), av, 2);

        b0 = vld1q_f16((const float16_t *)&B[(k + 3) * N + j]);
        b1 = vld1q_f16((const float16_t *)&B[(k + 3) * N + j + 8]);
        s00 = vfmaq_laneq_f32(s00, vcvt_f32_f16(vget_low_f16(b0)), av, 3);
        s01 = vfmaq_laneq_f32(s01, vcvt_f32_f16(vget_high_f16(b0)), av, 3);
        s10 = vfmaq_laneq_f32(s10, vcvt_f32_f16(vget_low_f16(b1)), av, 3);
        s11 = vfmaq_laneq_f32(s11, vcvt_f32_f16(vget_high_f16(b1)), av, 3);
      }

      for (; k < K; k++) {
        float a_val = fp16_to_float_c(A[k]);
        float16x8_t b0 = vld1q_f16((const float16_t *)&B[k * N + j]);
        float16x8_t b1 = vld1q_f16((const float16_t *)&B[k * N + j + 8]);
        s00 = vfmaq_n_f32(s00, vcvt_f32_f16(vget_low_f16(b0)), a_val);
        s01 = vfmaq_n_f32(s01, vcvt_f32_f16(vget_high_f16(b0)), a_val);
        s10 = vfmaq_n_f32(s10, vcvt_f32_f16(vget_low_f16(b1)), a_val);
        s11 = vfmaq_n_f32(s11, vcvt_f32_f16(vget_high_f16(b1)), a_val);
      }

      vst1q_f16((float16_t *)&C[j],
                vcombine_f16(vcvt_f16_f32(s00), vcvt_f16_f32(s01)));
      vst1q_f16((float16_t *)&C[j + 8],
                vcombine_f16(vcvt_f16_f32(s10), vcvt_f16_f32(s11)));
    }

    for (; j < N; j++) {
      float sum = 0.0f;
      for (int k2 = 0; k2 < K; k2++) {
        sum += fp16_to_float_c(A[k2]) * fp16_to_float_c(B[k2 * N + j]);
      }
      C[j] = float_to_fp16_c(sum);
    }
    return;
  }

  for (int mi = 0; mi < M; mi += 8) {
    for (int ni = 0; ni < N; ni += 8) {
      micro_kernel_f16_8x8_neon(A, B, C, M, N, K, mi, ni);
    }
  }
}

#ifndef _WIN32

typedef struct {
  const float *A;
  const float *B;
  float *C;
  int M, N, K;
  int m_start, m_end;
} gemm_f32_task_t;

static void *gemm_f32_thread_fn(void *arg) {
  gemm_f32_task_t *task = (gemm_f32_task_t *)arg;
  const float *A = task->A;
  const float *B = task->B;
  float *C = task->C;
  int N = task->N;
  int K = task->K;

  for (int mi = task->m_start; mi < task->m_end; mi += 8) {
    int mi_end = (mi + 8 > task->m_end) ? task->m_end : mi + 8;
    for (int ni = 0; ni < N; ni += 8) {
      micro_kernel_8x8_neon(A, B, C, mi_end - task->m_start, N, K, K, N, N,
                            mi - task->m_start, ni);
    }
  }
  return NULL;
}

void gemm_f32_kernel_mt(const float *A, const float *B, float *C, int M, int N,
                        int K, int num_threads) {
  memset(C, 0, M * N * sizeof(float));

  if (num_threads <= 1 || M < 16) {
    gemm_f32_kernel(A, B, C, M, N, K);
    return;
  }

  if (num_threads > M / 8)
    num_threads = M / 8;
  if (num_threads < 1)
    num_threads = 1;

  pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  gemm_f32_task_t *tasks =
      (gemm_f32_task_t *)malloc(num_threads * sizeof(gemm_f32_task_t));

  int rows_per_thread = (M + num_threads - 1) / num_threads;
  rows_per_thread = ((rows_per_thread + 7) / 8) * 8;

  for (int t = 0; t < num_threads; t++) {
    int m_start = t * rows_per_thread;
    int m_end = m_start + rows_per_thread;
    if (m_end > M)
      m_end = M;
    if (m_start >= M) {
      num_threads = t;
      break;
    }

    tasks[t].A = A + m_start * K;
    tasks[t].B = B;
    tasks[t].C = C + m_start * N;
    tasks[t].M = M;
    tasks[t].N = N;
    tasks[t].K = K;
    tasks[t].m_start = m_start;
    tasks[t].m_end = m_end;

    pthread_create(&threads[t], NULL, gemm_f32_thread_fn, &tasks[t]);
  }

  for (int t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
  }

  free(threads);
  free(tasks);
}

typedef struct {
  const uint16_t *A;
  const uint16_t *B;
  uint16_t *C;
  int M, N, K;
  int m_start, m_end;
} gemm_bf16_task_t;

static void *gemm_bf16_thread_fn(void *arg) {
  gemm_bf16_task_t *task = (gemm_bf16_task_t *)arg;
  const uint16_t *A = task->A;
  const uint16_t *B = task->B;
  uint16_t *C = task->C;
  int M_local = task->m_end - task->m_start;
  int N = task->N;
  int K = task->K;

  for (int mi = 0; mi < M_local; mi += 8) {
    for (int ni = 0; ni < N; ni += 8) {
      micro_kernel_bf16_8x8_neon(A, B, C, M_local, N, K, mi, ni);
    }
  }
  return NULL;
}

void gemm_bf16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K, int num_threads) {
  if (num_threads <= 1 || M < 16) {
    gemm_bf16_kernel(A, B, C, M, N, K);
    return;
  }

  if (num_threads > M / 8)
    num_threads = M / 8;
  if (num_threads < 1)
    num_threads = 1;

  pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  gemm_bf16_task_t *tasks =
      (gemm_bf16_task_t *)malloc(num_threads * sizeof(gemm_bf16_task_t));

  int rows_per_thread = (M + num_threads - 1) / num_threads;
  rows_per_thread = ((rows_per_thread + 7) / 8) * 8;

  for (int t = 0; t < num_threads; t++) {
    int m_start = t * rows_per_thread;
    int m_end = m_start + rows_per_thread;
    if (m_end > M)
      m_end = M;
    if (m_start >= M) {
      num_threads = t;
      break;
    }

    tasks[t].A = A + m_start * K;
    tasks[t].B = B;
    tasks[t].C = C + m_start * N;
    tasks[t].M = M;
    tasks[t].N = N;
    tasks[t].K = K;
    tasks[t].m_start = m_start;
    tasks[t].m_end = m_end;

    pthread_create(&threads[t], NULL, gemm_bf16_thread_fn, &tasks[t]);
  }

  for (int t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
  }

  free(threads);
  free(tasks);
}

typedef struct {
  const uint16_t *A;
  const uint16_t *B;
  uint16_t *C;
  int M, N, K;
  int m_start, m_end;
} gemm_f16_task_t;

static void *gemm_f16_thread_fn(void *arg) {
  gemm_f16_task_t *task = (gemm_f16_task_t *)arg;
  const uint16_t *A = task->A;
  const uint16_t *B = task->B;
  uint16_t *C = task->C;
  int M_local = task->m_end - task->m_start;
  int N = task->N;
  int K = task->K;

  for (int mi = 0; mi < M_local; mi += 8) {
    for (int ni = 0; ni < N; ni += 8) {
      micro_kernel_f16_8x8_neon(A, B, C, M_local, N, K, mi, ni);
    }
  }
  return NULL;
}

void gemm_f16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                        int M, int N, int K, int num_threads) {
  if (num_threads <= 1 || M < 16) {
    gemm_f16_kernel(A, B, C, M, N, K);
    return;
  }

  if (num_threads > M / 8)
    num_threads = M / 8;
  if (num_threads < 1)
    num_threads = 1;

  pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  gemm_f16_task_t *tasks =
      (gemm_f16_task_t *)malloc(num_threads * sizeof(gemm_f16_task_t));

  int rows_per_thread = (M + num_threads - 1) / num_threads;
  rows_per_thread = ((rows_per_thread + 7) / 8) * 8;

  for (int t = 0; t < num_threads; t++) {
    int m_start = t * rows_per_thread;
    int m_end = m_start + rows_per_thread;
    if (m_end > M)
      m_end = M;
    if (m_start >= M) {
      num_threads = t;
      break;
    }

    tasks[t].A = A + m_start * K;
    tasks[t].B = B;
    tasks[t].C = C + m_start * N;
    tasks[t].M = M;
    tasks[t].N = N;
    tasks[t].K = K;
    tasks[t].m_start = m_start;
    tasks[t].m_end = m_end;

    pthread_create(&threads[t], NULL, gemm_f16_thread_fn, &tasks[t]);
  }

  for (int t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
  }

  free(threads);
  free(tasks);
}

#else

void gemm_f32_kernel_mt(const float *A, const float *B, float *C, int M, int N,
                        int K, int num_threads) {
  (void)num_threads;
  gemm_f32_kernel(A, B, C, M, N, K);
}

void gemm_bf16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K, int num_threads) {
  (void)num_threads;
  gemm_bf16_kernel(A, B, C, M, N, K);
}

void gemm_f16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                        int M, int N, int K, int num_threads) {
  (void)num_threads;
  gemm_f16_kernel(A, B, C, M, N, K);
}

#endif

#else

void gemm_f32_kernel(const float *A, const float *B, float *C, int M, int N,
                     int K) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
}

void gemm_bf16_kernel(const uint16_t *A, const uint16_t *B, uint16_t *C, int M,
                      int N, int K) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
}

void gemm_f16_kernel(const uint16_t *A, const uint16_t *B, uint16_t *C, int M,
                     int N, int K) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
}

void gemm_f32_kernel_mt(const float *A, const float *B, float *C, int M, int N,
                        int K, int num_threads) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
  (void)num_threads;
}

void gemm_bf16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K, int num_threads) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
  (void)num_threads;
}

void gemm_f16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                        int M, int N, int K, int num_threads) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
  (void)num_threads;
}

#endif

#ifndef GEMM_KERNELS_H
#define GEMM_KERNELS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  bool has_neon;
  bool has_avx2;
  bool has_avx512;
  bool has_amx;
} gemm_caps_t;

gemm_caps_t gemm_get_capabilities(void);

void gemm_f32_kernel(const float *A, const float *B, float *C, int M, int N,
                     int K);
void gemm_f32_kernel_mt(const float *A, const float *B, float *C, int M, int N,
                        int K, int num_threads);

void gemm_bf16_kernel(const uint16_t *A, const uint16_t *B, uint16_t *C, int M,
                      int N, int K);
void gemm_bf16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K, int num_threads);

void gemm_f16_kernel(const uint16_t *A, const uint16_t *B, uint16_t *C, int M,
                     int N, int K);
void gemm_f16_kernel_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                        int M, int N, int K, int num_threads);

void gemm_f16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K);
void gemm_bf16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                          int M, int N, int K);
void gemm_f16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                            int M, int N, int K, int num_threads);
void gemm_bf16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                             int M, int N, int K, int num_threads);

#endif

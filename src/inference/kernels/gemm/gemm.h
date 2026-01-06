#ifndef GEMM_H
#define GEMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void gemm_set_num_threads(int num_threads);
int gemm_get_num_threads(void);
int gemm_get_max_threads(void);

void gemm_f32(const float *A, const float *B, float *C, int M, int N, int K,
              bool transpose_A, bool transpose_B);

void gemm_bf16(const uint16_t *A, const uint16_t *B, uint16_t *C, int M, int N,
               int K);

void gemm_f16(const uint16_t *A, const uint16_t *B, uint16_t *C, int M, int N,
              int K);

void bf16_array_to_f32(const uint16_t *src, float *dst, size_t count);
void f32_array_to_bf16(const float *src, uint16_t *dst, size_t count);

void f16_array_to_f32(const uint16_t *src, float *dst, size_t count);
void f32_array_to_f16(const float *src, uint16_t *dst, size_t count);

#endif

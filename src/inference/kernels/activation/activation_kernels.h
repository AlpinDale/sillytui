/*
 * Activation kernel interface for architecture-specific implementations
 */

#ifndef ACTIVATION_KERNELS_H
#define ACTIVATION_KERNELS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool has_neon;
  bool has_avx2;
  bool has_avx512;
} activation_caps_t;

activation_caps_t activation_get_capabilities(void);

/* SiLU kernels */
void silu_f32_kernel(float *out, const float *input, int num_tokens, int d);
void silu_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d);
void silu_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d);

/* SiLU and Multiply kernels */
void silu_and_mul_f32_kernel(float *out, const float *input, int num_tokens,
                             int d);
void silu_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                              int num_tokens, int d);
void silu_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d);

/* GELU kernels */
void gelu_f32_kernel(float *out, const float *input, int num_tokens, int d);
void gelu_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d);
void gelu_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d);

/* GELU and Multiply kernels */
void gelu_and_mul_f32_kernel(float *out, const float *input, int num_tokens,
                             int d);
void gelu_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                              int num_tokens, int d);
void gelu_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d);

/* GELU Tanh kernels */
void gelu_tanh_f32_kernel(float *out, const float *input, int num_tokens,
                          int d);
void gelu_tanh_bf16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d);
void gelu_tanh_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                          int d);

/* GELU Tanh and Multiply kernels */
void gelu_tanh_and_mul_f32_kernel(float *out, const float *input,
                                  int num_tokens, int d);
void gelu_tanh_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                                   int num_tokens, int d);
void gelu_tanh_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                                  int num_tokens, int d);

/* GELU Quick kernels */
void gelu_quick_f32_kernel(float *out, const float *input, int num_tokens,
                           int d);
void gelu_quick_bf16_kernel(uint16_t *out, const uint16_t *input,
                            int num_tokens, int d);
void gelu_quick_f16_kernel(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d);

/* GELU Quick and Multiply kernels */
void gelu_quick_and_mul_f32_kernel(float *out, const float *input,
                                   int num_tokens, int d);
void gelu_quick_and_mul_bf16_kernel(uint16_t *out, const uint16_t *input,
                                    int num_tokens, int d);
void gelu_quick_and_mul_f16_kernel(uint16_t *out, const uint16_t *input,
                                   int num_tokens, int d);

#endif /* ACTIVATION_KERNELS_H */

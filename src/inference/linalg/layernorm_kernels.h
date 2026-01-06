/*
 * Layernorm kernel interface for architecture-specific implementations
 */

#ifndef LAYERNORM_KERNELS_H
#define LAYERNORM_KERNELS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool has_neon;
  bool has_avx2;
  bool has_avx512;
} norm_caps_t;

norm_caps_t norm_get_capabilities(void);

/* FP32 kernels */
void rms_norm_f32_kernel(float *out, const float *input, const float *weight,
                         float epsilon, int num_tokens, int hidden_size);

void fused_add_rms_norm_f32_kernel(float *out, const float *input,
                                   float *residual, const float *weight,
                                   float epsilon, int num_tokens,
                                   int hidden_size);

/* BF16 kernels */
void rms_norm_bf16_kernel(uint16_t *out, const uint16_t *input,
                          const uint16_t *weight, float epsilon, int num_tokens,
                          int hidden_size);

void fused_add_rms_norm_bf16_kernel(uint16_t *out, const uint16_t *input,
                                    uint16_t *residual, const uint16_t *weight,
                                    float epsilon, int num_tokens,
                                    int hidden_size);

/* FP16 kernels */
void rms_norm_f16_kernel(uint16_t *out, const uint16_t *input,
                         const uint16_t *weight, float epsilon, int num_tokens,
                         int hidden_size);

void fused_add_rms_norm_f16_kernel(uint16_t *out, const uint16_t *input,
                                   uint16_t *residual, const uint16_t *weight,
                                   float epsilon, int num_tokens,
                                   int hidden_size);

#endif /* LAYERNORM_KERNELS_H */

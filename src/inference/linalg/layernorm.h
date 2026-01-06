/*
 * Layer Normalization Operations for Transformer Inference
 *
 * Implements RMSNorm and fused variants for FP32, BF16, and FP16.
 */

#ifndef LAYERNORM_H
#define LAYERNORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RMSNorm: Root Mean Square Layer Normalization
 *
 * Computes: out[i] = (input[i] / rms) * weight[i]
 * where rms = sqrt(mean(input^2) + epsilon)
 *
 * Args:
 *   out: Output tensor [num_tokens, hidden_size]
 *   input: Input tensor [num_tokens, hidden_size]
 *   weight: Learned scale parameters [hidden_size]
 *   epsilon: Small constant for numerical stability (typically 1e-6)
 *   num_tokens: Batch size (number of sequences)
 *   hidden_size: Feature dimension
 */
void rms_norm_f32(float *out, const float *input, const float *weight,
                  float epsilon, int num_tokens, int hidden_size);

void rms_norm_bf16(uint16_t *out, const uint16_t *input, const uint16_t *weight,
                   float epsilon, int num_tokens, int hidden_size);

void rms_norm_f16(uint16_t *out, const uint16_t *input, const uint16_t *weight,
                  float epsilon, int num_tokens, int hidden_size);

/*
 * Fused Add + RMSNorm
 *
 * Computes:
 *   residual = input + residual  (in-place residual update)
 *   out = (residual / rms) * weight
 *
 * This fuses the residual connection and normalization into one operation,
 * reducing memory bandwidth and improving cache utilization.
 *
 * Args:
 *   out: Output tensor [num_tokens, hidden_size] (normalized result)
 *   input: Input tensor [num_tokens, hidden_size] (will be destroyed/reused)
 *   residual: Residual tensor [num_tokens, hidden_size] (updated in-place)
 *   weight: Learned scale parameters [hidden_size]
 *   epsilon: Small constant for numerical stability
 *   num_tokens: Batch size
 *   hidden_size: Feature dimension
 */
void fused_add_rms_norm_f32(float *out, const float *input, float *residual,
                            const float *weight, float epsilon, int num_tokens,
                            int hidden_size);

void fused_add_rms_norm_bf16(uint16_t *out, const uint16_t *input,
                             uint16_t *residual, const uint16_t *weight,
                             float epsilon, int num_tokens, int hidden_size);

void fused_add_rms_norm_f16(uint16_t *out, const uint16_t *input,
                            uint16_t *residual, const uint16_t *weight,
                            float epsilon, int num_tokens, int hidden_size);

#ifdef __cplusplus
}
#endif

#endif /* LAYERNORM_H */

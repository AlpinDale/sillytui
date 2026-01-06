/*
 * Activation Functions for Transformer Inference
 *
 * Implements SiLU, GELU variants, and their gated counterparts (SwiGLU, GeGLU).
 * Supports FP32, BF16, and FP16 data types with NEON-optimized kernels.
 */

#ifndef ACTIVATION_H
#define ACTIVATION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SiLU (Swish) Activation: x * sigmoid(x) = x / (1 + exp(-x))
 * Used by Llama, Qwen, Mistral, and many modern LLMs.
 *
 * Args:
 *   out: Output tensor [num_tokens, d]
 *   input: Input tensor [num_tokens, d]
 *   num_tokens: Batch size
 *   d: Feature dimension
 */
void silu_f32(float *out, const float *input, int num_tokens, int d);
void silu_bf16(uint16_t *out, const uint16_t *input, int num_tokens, int d);
void silu_f16(uint16_t *out, const uint16_t *input, int num_tokens, int d);

/*
 * SiLU and Multiply (SwiGLU): silu(x) * gate
 * Used in FFN layers: SwiGLU(x, W1, W2, W3) = (silu(xW1) * xW3) @ W2
 *
 * Input layout: [x | gate] where x and gate each have dimension d
 *
 * Args:
 *   out: Output tensor [num_tokens, d]
 *   input: Input tensor [num_tokens, 2*d] - first half is x, second half is
 * gate num_tokens: Batch size d: Output feature dimension (half of input's last
 * dimension)
 */
void silu_and_mul_f32(float *out, const float *input, int num_tokens, int d);
void silu_and_mul_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                       int d);
void silu_and_mul_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d);

/*
 * GELU Activation (exact): x * 0.5 * (1 + erf(x / sqrt(2)))
 * Standard GELU as used in BERT, GPT-2, etc.
 */
void gelu_f32(float *out, const float *input, int num_tokens, int d);
void gelu_bf16(uint16_t *out, const uint16_t *input, int num_tokens, int d);
void gelu_f16(uint16_t *out, const uint16_t *input, int num_tokens, int d);

/*
 * GELU and Multiply (GeGLU): gelu(x) * gate
 */
void gelu_and_mul_f32(float *out, const float *input, int num_tokens, int d);
void gelu_and_mul_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                       int d);
void gelu_and_mul_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                      int d);

/*
 * GELU Tanh Approximation: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 *
 * x^3))) Fast approximation used by GPT-2.
 */
void gelu_tanh_f32(float *out, const float *input, int num_tokens, int d);
void gelu_tanh_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                    int d);
void gelu_tanh_f16(uint16_t *out, const uint16_t *input, int num_tokens, int d);

/*
 * GELU Tanh and Multiply
 */
void gelu_tanh_and_mul_f32(float *out, const float *input, int num_tokens,
                           int d);
void gelu_tanh_and_mul_bf16(uint16_t *out, const uint16_t *input,
                            int num_tokens, int d);
void gelu_tanh_and_mul_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                           int d);

/*
 * GELU Quick Approximation: x * sigmoid(1.702 * x) = x / (1 + exp(-1.702 * x))
 * Faster approximation with reasonable accuracy.
 */
void gelu_quick_f32(float *out, const float *input, int num_tokens, int d);
void gelu_quick_bf16(uint16_t *out, const uint16_t *input, int num_tokens,
                     int d);
void gelu_quick_f16(uint16_t *out, const uint16_t *input, int num_tokens,
                    int d);

/*
 * GELU Quick and Multiply
 */
void gelu_quick_and_mul_f32(float *out, const float *input, int num_tokens,
                            int d);
void gelu_quick_and_mul_bf16(uint16_t *out, const uint16_t *input,
                             int num_tokens, int d);
void gelu_quick_and_mul_f16(uint16_t *out, const uint16_t *input,
                            int num_tokens, int d);

#ifdef __cplusplus
}
#endif

#endif /* ACTIVATION_H */

/*
 * Sampling kernel interface for architecture-specific implementations
 */

#ifndef SAMPLING_KERNELS_H
#define SAMPLING_KERNELS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  bool has_neon;
} sampling_caps_t;

sampling_caps_t sampling_get_capabilities(void);

int sampling_sample_f32_kernel(const float *logits, int vocab_size,
                               float temperature, int top_k, float top_p,
                               float min_p, unsigned long long *rng_state);

float sampling_prob_f32_kernel(const float *logits, int vocab_size,
                               int token_id);

#ifdef __cplusplus
}
#endif

#endif // SAMPLING_KERNELS_H

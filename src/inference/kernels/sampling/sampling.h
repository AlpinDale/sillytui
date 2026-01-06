/*
 * Token Sampling - Public API
 */

#ifndef SAMPLING_H
#define SAMPLING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned long long rng_state;
} sampling_rng_t;

void sampling_rng_init(sampling_rng_t *rng, unsigned long long seed);
float sampling_rng_f32(sampling_rng_t *rng);

/*
 * Sample a token from logits using temperature, top-k, and top-p
 *
 * Parameters:
 *   logits:      [vocab_size] input logits
 *   vocab_size:  vocabulary size
 *   temperature: temperature scaling (0.0 = greedy, >0.0 = sampling)
 *   top_k:       top-k filtering (-1 = disabled, >0 = keep top k tokens)
 *   top_p:       top-p (nucleus) sampling (1.0 = disabled, <1.0 = nucleus)
 *   min_p:       minimum probability cutoff (0.0 = disabled, >0.0 = min-p)
 *   rng:         random number generator state
 *
 * Returns:
 *   sampled token ID
 */
int sampling_sample_f32(const float *logits, int vocab_size, float temperature,
                        int top_k, float top_p, float min_p,
                        sampling_rng_t *rng);

/*
 * Compute probability of a specific token from logits
 *
 * Parameters:
 *   logits:      [vocab_size] input logits
 *   vocab_size:  vocabulary size
 *   token_id:    token ID to compute probability for
 *
 * Returns:
 *   probability of the token (0.0 to 1.0)
 */
float sampling_prob_f32(const float *logits, int vocab_size, int token_id);

#ifdef __cplusplus
}
#endif

#endif // SAMPLING_H

/*
 * Token Sampling - Dispatcher and Scalar Fallback
 */

#include "inference/kernels/sampling/sampling.h"
#include "inference/kernels/sampling/sampling_kernels.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static unsigned int random_u32(unsigned long long *s) {
  *s ^= *s >> 12;
  *s ^= *s << 25;
  *s ^= *s >> 27;
  return (*s * 0x2545F4914F6CDD1DULL) >> 32;
}

static float random_f32(unsigned long long *s) {
  return (random_u32(s) >> 8) / 16777216.0f;
}

void sampling_rng_init(sampling_rng_t *rng, unsigned long long seed) {
  rng->rng_state = seed;
}

float sampling_rng_f32(sampling_rng_t *rng) {
  return random_f32(&rng->rng_state);
}

static float compute_max_logit(const float *logits, int vocab_size) {
  float max_val = -FLT_MAX;
  for (int i = 0; i < vocab_size; i++) {
    if (logits[i] > max_val) {
      max_val = logits[i];
    }
  }
  return max_val;
}

static void apply_temperature(float *probs, int vocab_size, float temperature) {
  if (temperature == 1.0f) {
    return;
  }
  float inv_temp = 1.0f / temperature;
  for (int i = 0; i < vocab_size; i++) {
    if (probs[i] > 0.0f) {
      probs[i] = powf(probs[i], inv_temp);
    }
  }
}

static int apply_top_k(float *probs, int vocab_size, int top_k) {
  if (top_k <= 0 || top_k >= vocab_size) {
    return vocab_size;
  }

  typedef struct {
    float prob;
    int idx;
  } prob_idx_t;

  prob_idx_t *candidates =
      (prob_idx_t *)malloc(vocab_size * sizeof(prob_idx_t));
  for (int i = 0; i < vocab_size; i++) {
    candidates[i].prob = probs[i];
    candidates[i].idx = i;
  }

  for (int i = 0; i < top_k; i++) {
    int max_idx = i;
    for (int j = i + 1; j < vocab_size; j++) {
      if (candidates[j].prob > candidates[max_idx].prob) {
        max_idx = j;
      }
    }
    prob_idx_t tmp = candidates[i];
    candidates[i] = candidates[max_idx];
    candidates[max_idx] = tmp;
  }

  for (int i = 0; i < vocab_size; i++) {
    probs[i] = 0.0f;
  }
  for (int i = 0; i < top_k; i++) {
    probs[candidates[i].idx] = candidates[i].prob;
  }

  free(candidates);
  return top_k;
}

static int apply_top_p(float *probs, int vocab_size, float top_p) {
  if (top_p >= 1.0f) {
    return vocab_size;
  }

  typedef struct {
    float prob;
    int idx;
  } prob_idx_t;

  prob_idx_t *sorted = (prob_idx_t *)malloc(vocab_size * sizeof(prob_idx_t));
  int count = 0;
  for (int i = 0; i < vocab_size; i++) {
    if (probs[i] > 0.0f) {
      sorted[count].prob = probs[i];
      sorted[count].idx = i;
      count++;
    }
  }

  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (sorted[j].prob > sorted[i].prob) {
        prob_idx_t tmp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = tmp;
      }
    }
  }

  float cumsum = 0.0f;
  int keep_count = 0;
  for (int i = 0; i < count; i++) {
    cumsum += sorted[i].prob;
    keep_count++;
    if (cumsum >= top_p) {
      break;
    }
  }

  for (int i = 0; i < vocab_size; i++) {
    probs[i] = 0.0f;
  }
  for (int i = 0; i < keep_count; i++) {
    probs[sorted[i].idx] = sorted[i].prob;
  }

  free(sorted);
  return keep_count;
}

static int apply_min_p(float *probs, int vocab_size, float min_p) {
  if (min_p <= 0.0f) {
    return vocab_size;
  }

  float max_prob = 0.0f;
  for (int i = 0; i < vocab_size; i++) {
    if (probs[i] > max_prob) {
      max_prob = probs[i];
    }
  }

  float threshold = max_prob * min_p;
  int keep_count = 0;
  for (int i = 0; i < vocab_size; i++) {
    if (probs[i] < threshold) {
      probs[i] = 0.0f;
    } else {
      keep_count++;
    }
  }

  return keep_count;
}

static float compute_sum(const float *probs, int vocab_size) {
  float sum = 0.0f;
  for (int i = 0; i < vocab_size; i++) {
    sum += probs[i];
  }
  return sum;
}

static void normalize_probs(float *probs, int vocab_size, float sum) {
  if (sum == 0.0f) {
    return;
  }
  float inv_sum = 1.0f / sum;
  for (int i = 0; i < vocab_size; i++) {
    probs[i] *= inv_sum;
  }
}

static int sample_from_distribution(const float *probs, int vocab_size,
                                    float random_val) {
  float cumsum = 0.0f;
  for (int i = 0; i < vocab_size; i++) {
    cumsum += probs[i];
    if (random_val < cumsum) {
      return i;
    }
  }
  return vocab_size - 1;
}

static int sample_argmax(const float *logits, int vocab_size) {
  int max_idx = 0;
  float max_val = logits[0];
  for (int i = 1; i < vocab_size; i++) {
    if (logits[i] > max_val) {
      max_val = logits[i];
      max_idx = i;
    }
  }
  return max_idx;
}

static int sampling_sample_f32_scalar(const float *logits, int vocab_size,
                                      float temperature, int top_k, float top_p,
                                      float min_p,
                                      unsigned long long *rng_state) {
  if (temperature == 0.0f) {
    return sample_argmax(logits, vocab_size);
  }

  float max_logit = compute_max_logit(logits, vocab_size);

  float *probs = (float *)malloc(vocab_size * sizeof(float));
  for (int i = 0; i < vocab_size; i++) {
    probs[i] = expf(logits[i] - max_logit);
  }

  if (min_p > 0.0f) {
    apply_min_p(probs, vocab_size, min_p);
  }

  if (top_k > 0) {
    apply_top_k(probs, vocab_size, top_k);
  }

  if (top_p < 1.0f) {
    float sum = compute_sum(probs, vocab_size);
    normalize_probs(probs, vocab_size, sum);
    apply_top_p(probs, vocab_size, top_p);
  }

  apply_temperature(probs, vocab_size, temperature);

  float sum = compute_sum(probs, vocab_size);
  normalize_probs(probs, vocab_size, sum);

  float random_val = random_f32(rng_state);
  int sampled = sample_from_distribution(probs, vocab_size, random_val);

  free(probs);
  return sampled;
}

static float sampling_prob_f32_scalar(const float *logits, int vocab_size,
                                      int token_id) {
  if (token_id < 0 || token_id >= vocab_size) {
    return 0.0f;
  }

  float max_logit = compute_max_logit(logits, vocab_size);

  float sum = 0.0f;
  for (int i = 0; i < vocab_size; i++) {
    sum += expf(logits[i] - max_logit);
  }

  if (sum == 0.0f) {
    return 0.0f;
  }

  return expf(logits[token_id] - max_logit) / sum;
}

int sampling_sample_f32(const float *logits, int vocab_size, float temperature,
                        int top_k, float top_p, float min_p,
                        sampling_rng_t *rng) {
  sampling_caps_t caps = sampling_get_capabilities();
  if (caps.has_neon) {
    return sampling_sample_f32_kernel(logits, vocab_size, temperature, top_k,
                                      top_p, min_p, &rng->rng_state);
  } else {
    return sampling_sample_f32_scalar(logits, vocab_size, temperature, top_k,
                                      top_p, min_p, &rng->rng_state);
  }
}

float sampling_prob_f32(const float *logits, int vocab_size, int token_id) {
  sampling_caps_t caps = sampling_get_capabilities();
  if (caps.has_neon) {
    return sampling_prob_f32_kernel(logits, vocab_size, token_id);
  } else {
    return sampling_prob_f32_scalar(logits, vocab_size, token_id);
  }
}

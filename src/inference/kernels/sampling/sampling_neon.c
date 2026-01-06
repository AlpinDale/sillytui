/*
 * Token Sampling - NEON Optimized Implementation
 */

#include "inference/kernels/sampling/sampling_kernels.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

sampling_caps_t sampling_get_capabilities(void) {
  sampling_caps_t caps = {0};
#if HAS_NEON
  caps.has_neon = true;
#endif
  return caps;
}

#if HAS_NEON

static unsigned int random_u32(unsigned long long *s) {
  *s ^= *s >> 12;
  *s ^= *s << 25;
  *s ^= *s >> 27;
  return (*s * 0x2545F4914F6CDD1DULL) >> 32;
}

static float random_f32(unsigned long long *s) {
  return (random_u32(s) >> 8) / 16777216.0f;
}

static inline float hmax_f32x4(float32x4_t v) {
  float32x2_t max2 = vpmax_f32(vget_low_f32(v), vget_high_f32(v));
  max2 = vpmax_f32(max2, max2);
  return vget_lane_f32(max2, 0);
}

static inline float hsum_f32x4(float32x4_t v) {
  float32x2_t sum2 = vadd_f32(vget_low_f32(v), vget_high_f32(v));
  sum2 = vpadd_f32(sum2, sum2);
  return vget_lane_f32(sum2, 0);
}

static float compute_max_logit_neon(const float *logits, int vocab_size) {
  float32x4_t max_v = vdupq_n_f32(-FLT_MAX);
  int i = 0;

  for (; i + 4 <= vocab_size; i += 4) {
    float32x4_t v = vld1q_f32(logits + i);
    max_v = vmaxq_f32(max_v, v);
  }

  float max_val = hmax_f32x4(max_v);
  for (; i < vocab_size; i++) {
    if (logits[i] > max_val) {
      max_val = logits[i];
    }
  }
  return max_val;
}

static float compute_sum_neon(const float *probs, int vocab_size) {
  float32x4_t sum_v = vdupq_n_f32(0.0f);
  int i = 0;

  for (; i + 4 <= vocab_size; i += 4) {
    float32x4_t v = vld1q_f32(probs + i);
    sum_v = vaddq_f32(sum_v, v);
  }

  float sum = hsum_f32x4(sum_v);
  for (; i < vocab_size; i++) {
    sum += probs[i];
  }
  return sum;
}

static void normalize_probs_neon(float *probs, int vocab_size, float sum) {
  if (sum == 0.0f) {
    return;
  }
  float inv_sum = 1.0f / sum;
  float32x4_t inv_sum_v = vdupq_n_f32(inv_sum);
  int i = 0;

  for (; i + 4 <= vocab_size; i += 4) {
    float32x4_t v = vld1q_f32(probs + i);
    v = vmulq_f32(v, inv_sum_v);
    vst1q_f32(probs + i, v);
  }

  for (; i < vocab_size; i++) {
    probs[i] *= inv_sum;
  }
}

static void apply_temperature_neon(float *probs, int vocab_size,
                                   float temperature) {
  if (temperature == 1.0f) {
    return;
  }
  float inv_temp = 1.0f / temperature;
  int i = 0;

  for (; i + 4 <= vocab_size; i += 4) {
    float32x4_t v = vld1q_f32(probs + i);
    float temp_vals[4];
    vst1q_f32(temp_vals, v);
    for (int j = 0; j < 4; j++) {
      if (temp_vals[j] > 0.0f) {
        temp_vals[j] = powf(temp_vals[j], inv_temp);
      }
    }
    v = vld1q_f32(temp_vals);
    vst1q_f32(probs + i, v);
  }

  for (; i < vocab_size; i++) {
    if (probs[i] > 0.0f) {
      probs[i] = powf(probs[i], inv_temp);
    }
  }
}

static int sample_argmax_neon(const float *logits, int vocab_size) {
  float32x4_t max_v = vdupq_n_f32(-FLT_MAX);
  int i = 0;
  int max_idx = 0;
  float max_val = -FLT_MAX;

  for (; i + 4 <= vocab_size; i += 4) {
    float32x4_t v = vld1q_f32(logits + i);
    max_v = vmaxq_f32(max_v, v);

    float vals[4];
    vst1q_f32(vals, v);
    for (int j = 0; j < 4; j++) {
      if (vals[j] > max_val) {
        max_val = vals[j];
        max_idx = i + j;
      }
    }
  }

  for (; i < vocab_size; i++) {
    if (logits[i] > max_val) {
      max_val = logits[i];
      max_idx = i;
    }
  }
  return max_idx;
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

static void compute_softmax_neon(float *probs, const float *logits,
                                 int vocab_size, float max_logit) {
  float32x4_t max_broadcast = vdupq_n_f32(max_logit);
  int i = 0;

  for (; i + 4 <= vocab_size; i += 4) {
    float32x4_t v = vld1q_f32(logits + i);
    v = vsubq_f32(v, max_broadcast);
    float temp[4];
    vst1q_f32(temp, v);
    for (int j = 0; j < 4; j++) {
      temp[j] = expf(temp[j]);
    }
    v = vld1q_f32(temp);
    vst1q_f32(probs + i, v);
  }

  for (; i < vocab_size; i++) {
    probs[i] = expf(logits[i] - max_logit);
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

int sampling_sample_f32_kernel(const float *logits, int vocab_size,
                               float temperature, int top_k, float top_p,
                               float min_p, unsigned long long *rng_state) {
  if (temperature == 0.0f) {
    return sample_argmax_neon(logits, vocab_size);
  }

  float max_logit = compute_max_logit_neon(logits, vocab_size);

  float *probs = (float *)malloc(vocab_size * sizeof(float));
  compute_softmax_neon(probs, logits, vocab_size, max_logit);

  if (min_p > 0.0f) {
    apply_min_p(probs, vocab_size, min_p);
  }

  if (top_k > 0) {
    apply_top_k(probs, vocab_size, top_k);
  }

  if (top_p < 1.0f) {
    float sum = compute_sum_neon(probs, vocab_size);
    normalize_probs_neon(probs, vocab_size, sum);
    apply_top_p(probs, vocab_size, top_p);
  }

  apply_temperature_neon(probs, vocab_size, temperature);

  float sum = compute_sum_neon(probs, vocab_size);
  normalize_probs_neon(probs, vocab_size, sum);

  float random_val = random_f32(rng_state);
  int sampled = sample_from_distribution(probs, vocab_size, random_val);

  free(probs);
  return sampled;
}

float sampling_prob_f32_kernel(const float *logits, int vocab_size,
                               int token_id) {
  if (token_id < 0 || token_id >= vocab_size) {
    return 0.0f;
  }

  float max_logit = compute_max_logit_neon(logits, vocab_size);

  float sum = 0.0f;
  int i = 0;
  float32x4_t sum_v = vdupq_n_f32(0.0f);
  float32x4_t max_broadcast = vdupq_n_f32(max_logit);

  for (; i + 4 <= vocab_size; i += 4) {
    float32x4_t v = vld1q_f32(logits + i);
    v = vsubq_f32(v, max_broadcast);
    float temp[4];
    vst1q_f32(temp, v);
    for (int j = 0; j < 4; j++) {
      temp[j] = expf(temp[j]);
    }
    v = vld1q_f32(temp);
    sum_v = vaddq_f32(sum_v, v);
  }

  sum = hsum_f32x4(sum_v);
  for (; i < vocab_size; i++) {
    sum += expf(logits[i] - max_logit);
  }

  if (sum == 0.0f) {
    return 0.0f;
  }

  return expf(logits[token_id] - max_logit) / sum;
}

#else // HAS_NEON

int sampling_sample_f32_kernel(const float *logits, int vocab_size,
                               float temperature, int top_k, float top_p,
                               float min_p, unsigned long long *rng_state) {
  (void)logits;
  (void)vocab_size;
  (void)temperature;
  (void)top_k;
  (void)top_p;
  (void)min_p;
  (void)rng_state;
  return 0;
}

float sampling_prob_f32_kernel(const float *logits, int vocab_size,
                               int token_id) {
  (void)logits;
  (void)vocab_size;
  (void)token_id;
  return 0.0f;
}

#endif // HAS_NEON

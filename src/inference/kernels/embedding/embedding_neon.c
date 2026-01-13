/*
 * Embedding Lookup - NEON Optimized Implementation
 */

#include "inference/backend/caps.h"
#include "inference/kernels/embedding/embedding_kernels.h"
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

embedding_caps_t embedding_get_capabilities(void) {
  embedding_caps_t caps = {0};
  caps.has_neon = caps_has(CAP_NEON);
  return caps;
}

#if HAS_NEON

void embedding_lookup_f32_kernel(float *output, const int64_t *token_ids,
                                 const float *weight, int num_tokens,
                                 int vocab_size, int embedding_dim,
                                 int64_t padding_idx) {
  for (int i = 0; i < num_tokens; i++) {
    int64_t token_id = token_ids[i];
    float *out_vec = output + i * embedding_dim;

    if (token_id == padding_idx) {
      // Padding token: output zero vector
      memset(out_vec, 0, embedding_dim * sizeof(float));
      continue;
    }

    // Clamp token_id to valid range
    if (token_id < 0) {
      token_id = 0;
    } else if (token_id >= vocab_size) {
      token_id = vocab_size - 1;
    }

    const float *weight_vec = weight + token_id * embedding_dim;

    // Vectorized copy: process 4 floats at a time
    int d = 0;
    for (; d + 4 <= embedding_dim; d += 4) {
      float32x4_t v = vld1q_f32(weight_vec + d);
      vst1q_f32(out_vec + d, v);
    }

    // Handle remaining elements
    for (; d < embedding_dim; d++) {
      out_vec[d] = weight_vec[d];
    }
  }
}

void embedding_lookup_bf16_kernel(uint16_t *output, const int64_t *token_ids,
                                  const uint16_t *weight, int num_tokens,
                                  int vocab_size, int embedding_dim,
                                  int64_t padding_idx) {
  for (int i = 0; i < num_tokens; i++) {
    int64_t token_id = token_ids[i];
    uint16_t *out_vec = output + i * embedding_dim;

    if (token_id == padding_idx) {
      // Padding token: output zero vector
      memset(out_vec, 0, embedding_dim * sizeof(uint16_t));
      continue;
    }

    // Clamp token_id to valid range
    if (token_id < 0) {
      token_id = 0;
    } else if (token_id >= vocab_size) {
      token_id = vocab_size - 1;
    }

    const uint16_t *weight_vec = weight + token_id * embedding_dim;

    // For BF16, we can copy directly (no conversion needed for memcpy)
    // But for better performance with vectorization, we could convert to FP32
    // For now, use memcpy which is still fast
    memcpy(out_vec, weight_vec, embedding_dim * sizeof(uint16_t));
  }
}

void embedding_lookup_f16_kernel(uint16_t *output, const int64_t *token_ids,
                                 const uint16_t *weight, int num_tokens,
                                 int vocab_size, int embedding_dim,
                                 int64_t padding_idx) {
  for (int i = 0; i < num_tokens; i++) {
    int64_t token_id = token_ids[i];
    uint16_t *out_vec = output + i * embedding_dim;

    if (token_id == padding_idx) {
      // Padding token: output zero vector
      memset(out_vec, 0, embedding_dim * sizeof(uint16_t));
      continue;
    }

    // Clamp token_id to valid range
    if (token_id < 0) {
      token_id = 0;
    } else if (token_id >= vocab_size) {
      token_id = vocab_size - 1;
    }

    const uint16_t *weight_vec = weight + token_id * embedding_dim;

    // For FP16, we can use NEON vectorized loads/stores
    int d = 0;
    for (; d + 8 <= embedding_dim; d += 8) {
      uint16x8_t v = vld1q_u16(weight_vec + d);
      vst1q_u16(out_vec + d, v);
    }

    // Handle remaining elements
    for (; d < embedding_dim; d++) {
      out_vec[d] = weight_vec[d];
    }
  }
}

#else // HAS_NEON

void embedding_lookup_f32_kernel(float *output, const int64_t *token_ids,
                                 const float *weight, int num_tokens,
                                 int vocab_size, int embedding_dim,
                                 int64_t padding_idx) {
  (void)output;
  (void)token_ids;
  (void)weight;
  (void)num_tokens;
  (void)vocab_size;
  (void)embedding_dim;
  (void)padding_idx;
}

void embedding_lookup_bf16_kernel(uint16_t *output, const int64_t *token_ids,
                                  const uint16_t *weight, int num_tokens,
                                  int vocab_size, int embedding_dim,
                                  int64_t padding_idx) {
  (void)output;
  (void)token_ids;
  (void)weight;
  (void)num_tokens;
  (void)vocab_size;
  (void)embedding_dim;
  (void)padding_idx;
}

void embedding_lookup_f16_kernel(uint16_t *output, const int64_t *token_ids,
                                 const uint16_t *weight, int num_tokens,
                                 int vocab_size, int embedding_dim,
                                 int64_t padding_idx) {
  (void)output;
  (void)token_ids;
  (void)weight;
  (void)num_tokens;
  (void)vocab_size;
  (void)embedding_dim;
  (void)padding_idx;
}

#endif // HAS_NEON

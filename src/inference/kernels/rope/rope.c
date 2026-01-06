/*
 * Rotary Position Embeddings - Reference/Scalar Implementations
 */

#include "inference/kernels/rope/rope.h"
#include "inference/kernels/rope/rope_kernels.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============ FP16/BF16 Conversion Helpers ============ */

static inline uint16_t float_to_bf16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t lsb = (bits >> 16) & 1;
  bits += 0x7fff + lsb;
  return (uint16_t)(bits >> 16);
}

static inline float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_fp16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t sign = bits & 0x80000000;
  uint32_t exp = (bits >> 23) & 0xFF;
  uint32_t mant = bits & 0x7FFFFF;
  if (exp == 0xFF)
    return (uint16_t)((sign >> 16) | 0x7C00);
  if (exp == 0 && mant == 0)
    return (uint16_t)(sign >> 16);
  int32_t new_exp = (int32_t)exp - 127 + 15;
  if (new_exp <= 0)
    return (uint16_t)(sign >> 16);
  if (new_exp >= 31)
    return (uint16_t)((sign >> 16) | 0x7C00);
  return (uint16_t)((sign >> 16) | (new_exp << 10) | (mant >> 13));
}

static inline float fp16_to_float(uint16_t fp16) {
  uint32_t sign = (fp16 & 0x8000) << 16;
  uint32_t exp = (fp16 >> 10) & 0x1F;
  uint32_t mant = fp16 & 0x3FF;
  uint32_t f32_bits;
  if (exp == 0) {
    f32_bits = (mant == 0) ? sign : (sign | (127 - 14) << 23 | mant << 13);
  } else if (exp == 31) {
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    f32_bits = sign | (((uint32_t)(exp - 15 + 127)) << 23) | (mant << 13);
  }
  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

/* ============ Cos/Sin Cache Computation ============ */

void rope_compute_cos_sin_cache_f32(float *cache, int max_position, int rot_dim,
                                    float base) {
  int half_dim = rot_dim / 2;

  for (int pos = 0; pos < max_position; pos++) {
    for (int i = 0; i < half_dim; i++) {
      float freq = 1.0f / powf(base, (float)(2 * i) / (float)rot_dim);
      float angle = (float)pos * freq;
      cache[pos * rot_dim + i] = cosf(angle);
      cache[pos * rot_dim + half_dim + i] = sinf(angle);
    }
  }
}

/* ============ Scalar NeoX Implementation ============ */

static void rope_neox_f32_scalar(const int64_t *positions, float *query,
                                 float *key, const float *cos_sin_cache,
                                 int num_tokens, int num_heads,
                                 int num_kv_heads, int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const float *cos_ptr = cos_sin_cache + pos * rot_dim;
    const float *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      float *q = query + t * query_stride + h * head_size;
      for (int i = 0; i < half_dim; i++) {
        float x = q[i];
        float y = q[half_dim + i];
        float cos_val = cos_ptr[i];
        float sin_val = sin_ptr[i];
        q[i] = x * cos_val - y * sin_val;
        q[half_dim + i] = y * cos_val + x * sin_val;
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        float *k = key + t * key_stride + h * head_size;
        for (int i = 0; i < half_dim; i++) {
          float x = k[i];
          float y = k[half_dim + i];
          float cos_val = cos_ptr[i];
          float sin_val = sin_ptr[i];
          k[i] = x * cos_val - y * sin_val;
          k[half_dim + i] = y * cos_val + x * sin_val;
        }
      }
    }
  }
}

static void rope_neox_bf16_scalar(const int64_t *positions, uint16_t *query,
                                  uint16_t *key, const uint16_t *cos_sin_cache,
                                  int num_tokens, int num_heads,
                                  int num_kv_heads, int head_size,
                                  int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;
      for (int i = 0; i < half_dim; i++) {
        float x = bf16_to_float(q[i]);
        float y = bf16_to_float(q[half_dim + i]);
        float cos_val = bf16_to_float(cos_ptr[i]);
        float sin_val = bf16_to_float(sin_ptr[i]);
        q[i] = float_to_bf16(x * cos_val - y * sin_val);
        q[half_dim + i] = float_to_bf16(y * cos_val + x * sin_val);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;
        for (int i = 0; i < half_dim; i++) {
          float x = bf16_to_float(k[i]);
          float y = bf16_to_float(k[half_dim + i]);
          float cos_val = bf16_to_float(cos_ptr[i]);
          float sin_val = bf16_to_float(sin_ptr[i]);
          k[i] = float_to_bf16(x * cos_val - y * sin_val);
          k[half_dim + i] = float_to_bf16(y * cos_val + x * sin_val);
        }
      }
    }
  }
}

static void rope_neox_f16_scalar(const int64_t *positions, uint16_t *query,
                                 uint16_t *key, const uint16_t *cos_sin_cache,
                                 int num_tokens, int num_heads,
                                 int num_kv_heads, int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;
      for (int i = 0; i < half_dim; i++) {
        float x = fp16_to_float(q[i]);
        float y = fp16_to_float(q[half_dim + i]);
        float cos_val = fp16_to_float(cos_ptr[i]);
        float sin_val = fp16_to_float(sin_ptr[i]);
        q[i] = float_to_fp16(x * cos_val - y * sin_val);
        q[half_dim + i] = float_to_fp16(y * cos_val + x * sin_val);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;
        for (int i = 0; i < half_dim; i++) {
          float x = fp16_to_float(k[i]);
          float y = fp16_to_float(k[half_dim + i]);
          float cos_val = fp16_to_float(cos_ptr[i]);
          float sin_val = fp16_to_float(sin_ptr[i]);
          k[i] = float_to_fp16(x * cos_val - y * sin_val);
          k[half_dim + i] = float_to_fp16(y * cos_val + x * sin_val);
        }
      }
    }
  }
}

/* ============ Scalar GPT-J Implementation ============ */

static void rope_gptj_f32_scalar(const int64_t *positions, float *query,
                                 float *key, const float *cos_sin_cache,
                                 int num_tokens, int num_heads,
                                 int num_kv_heads, int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const float *cos_ptr = cos_sin_cache + pos * rot_dim;
    const float *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      float *q = query + t * query_stride + h * head_size;
      for (int i = 0; i < half_dim; i++) {
        int x_idx = 2 * i;
        int y_idx = 2 * i + 1;
        float x = q[x_idx];
        float y = q[y_idx];
        float cos_val = cos_ptr[i];
        float sin_val = sin_ptr[i];
        q[x_idx] = x * cos_val - y * sin_val;
        q[y_idx] = y * cos_val + x * sin_val;
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        float *k = key + t * key_stride + h * head_size;
        for (int i = 0; i < half_dim; i++) {
          int x_idx = 2 * i;
          int y_idx = 2 * i + 1;
          float x = k[x_idx];
          float y = k[y_idx];
          float cos_val = cos_ptr[i];
          float sin_val = sin_ptr[i];
          k[x_idx] = x * cos_val - y * sin_val;
          k[y_idx] = y * cos_val + x * sin_val;
        }
      }
    }
  }
}

static void rope_gptj_bf16_scalar(const int64_t *positions, uint16_t *query,
                                  uint16_t *key, const uint16_t *cos_sin_cache,
                                  int num_tokens, int num_heads,
                                  int num_kv_heads, int head_size,
                                  int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;
      for (int i = 0; i < half_dim; i++) {
        int x_idx = 2 * i;
        int y_idx = 2 * i + 1;
        float x = bf16_to_float(q[x_idx]);
        float y = bf16_to_float(q[y_idx]);
        float cos_val = bf16_to_float(cos_ptr[i]);
        float sin_val = bf16_to_float(sin_ptr[i]);
        q[x_idx] = float_to_bf16(x * cos_val - y * sin_val);
        q[y_idx] = float_to_bf16(y * cos_val + x * sin_val);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;
        for (int i = 0; i < half_dim; i++) {
          int x_idx = 2 * i;
          int y_idx = 2 * i + 1;
          float x = bf16_to_float(k[x_idx]);
          float y = bf16_to_float(k[y_idx]);
          float cos_val = bf16_to_float(cos_ptr[i]);
          float sin_val = bf16_to_float(sin_ptr[i]);
          k[x_idx] = float_to_bf16(x * cos_val - y * sin_val);
          k[y_idx] = float_to_bf16(y * cos_val + x * sin_val);
        }
      }
    }
  }
}

static void rope_gptj_f16_scalar(const int64_t *positions, uint16_t *query,
                                 uint16_t *key, const uint16_t *cos_sin_cache,
                                 int num_tokens, int num_heads,
                                 int num_kv_heads, int head_size, int rot_dim) {
  int half_dim = rot_dim / 2;
  int query_stride = num_heads * head_size;
  int key_stride = num_kv_heads * head_size;

  for (int t = 0; t < num_tokens; t++) {
    int64_t pos = positions[t];
    const uint16_t *cos_ptr = cos_sin_cache + pos * rot_dim;
    const uint16_t *sin_ptr = cos_ptr + half_dim;

    for (int h = 0; h < num_heads; h++) {
      uint16_t *q = query + t * query_stride + h * head_size;
      for (int i = 0; i < half_dim; i++) {
        int x_idx = 2 * i;
        int y_idx = 2 * i + 1;
        float x = fp16_to_float(q[x_idx]);
        float y = fp16_to_float(q[y_idx]);
        float cos_val = fp16_to_float(cos_ptr[i]);
        float sin_val = fp16_to_float(sin_ptr[i]);
        q[x_idx] = float_to_fp16(x * cos_val - y * sin_val);
        q[y_idx] = float_to_fp16(y * cos_val + x * sin_val);
      }
    }

    if (key != NULL) {
      for (int h = 0; h < num_kv_heads; h++) {
        uint16_t *k = key + t * key_stride + h * head_size;
        for (int i = 0; i < half_dim; i++) {
          int x_idx = 2 * i;
          int y_idx = 2 * i + 1;
          float x = fp16_to_float(k[x_idx]);
          float y = fp16_to_float(k[y_idx]);
          float cos_val = fp16_to_float(cos_ptr[i]);
          float sin_val = fp16_to_float(sin_ptr[i]);
          k[x_idx] = float_to_fp16(x * cos_val - y * sin_val);
          k[y_idx] = float_to_fp16(y * cos_val + x * sin_val);
        }
      }
    }
  }
}

/* ============ Public API ============ */

void rope_f32(const int64_t *positions, float *query, float *key,
              const float *cos_sin_cache, int num_tokens, int num_heads,
              int num_kv_heads, int head_size, int rot_dim, bool is_neox) {
  if (num_tokens <= 0 || num_heads <= 0 || head_size <= 0 || rot_dim <= 0)
    return;

  rope_caps_t caps = rope_get_capabilities();

  if (is_neox) {
    if (caps.has_neon) {
      rope_neox_f32_kernel(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    } else {
      rope_neox_f32_scalar(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    }
  } else {
    if (caps.has_neon) {
      rope_gptj_f32_kernel(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    } else {
      rope_gptj_f32_scalar(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    }
  }
}

void rope_bf16(const int64_t *positions, uint16_t *query, uint16_t *key,
               const uint16_t *cos_sin_cache, int num_tokens, int num_heads,
               int num_kv_heads, int head_size, int rot_dim, bool is_neox) {
  if (num_tokens <= 0 || num_heads <= 0 || head_size <= 0 || rot_dim <= 0)
    return;

  rope_caps_t caps = rope_get_capabilities();

  if (is_neox) {
    if (caps.has_neon) {
      rope_neox_bf16_kernel(positions, query, key, cos_sin_cache, num_tokens,
                            num_heads, num_kv_heads, head_size, rot_dim);
    } else {
      rope_neox_bf16_scalar(positions, query, key, cos_sin_cache, num_tokens,
                            num_heads, num_kv_heads, head_size, rot_dim);
    }
  } else {
    if (caps.has_neon) {
      rope_gptj_bf16_kernel(positions, query, key, cos_sin_cache, num_tokens,
                            num_heads, num_kv_heads, head_size, rot_dim);
    } else {
      rope_gptj_bf16_scalar(positions, query, key, cos_sin_cache, num_tokens,
                            num_heads, num_kv_heads, head_size, rot_dim);
    }
  }
}

void rope_f16(const int64_t *positions, uint16_t *query, uint16_t *key,
              const uint16_t *cos_sin_cache, int num_tokens, int num_heads,
              int num_kv_heads, int head_size, int rot_dim, bool is_neox) {
  if (num_tokens <= 0 || num_heads <= 0 || head_size <= 0 || rot_dim <= 0)
    return;

  rope_caps_t caps = rope_get_capabilities();

  if (is_neox) {
    if (caps.has_neon) {
      rope_neox_f16_kernel(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    } else {
      rope_neox_f16_scalar(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    }
  } else {
    if (caps.has_neon) {
      rope_gptj_f16_kernel(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    } else {
      rope_gptj_f16_scalar(positions, query, key, cos_sin_cache, num_tokens,
                           num_heads, num_kv_heads, head_size, rot_dim);
    }
  }
}

/*
 * RoPE (Rotary Position Embeddings) Tests
 */

extern "C" {
#include "inference/kernels/rope/rope.h"
#include "test_framework.h"
}

#include <cmath>
#include <cstdlib>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void compute_rope_reference_neox(const int64_t *positions, float *query,
                                        float *key, const float *cos_sin_cache,
                                        int num_tokens, int num_heads,
                                        int num_kv_heads, int head_size,
                                        int rot_dim) {
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

static double compute_max_error(const float *a, const float *b, int n) {
  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    double diff = fabs((double)a[i] - (double)b[i]);
    double mag = fmax(fabs((double)a[i]), 1e-6);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

/* ============ Cos/Sin Cache Tests ============ */

TEST(rope_cos_sin_cache_basic) {
  const int max_pos = 128;
  const int rot_dim = 64;
  const float base = 10000.0f;
  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, base);

  for (int pos = 0; pos < 10; pos++) {
    for (int i = 0; i < rot_dim / 2; i++) {
      float freq = 1.0f / powf(base, (float)(2 * i) / (float)rot_dim);
      float angle = (float)pos * freq;
      float expected_cos = cosf(angle);
      float expected_sin = sinf(angle);

      float actual_cos = cache[pos * rot_dim + i];
      float actual_sin = cache[pos * rot_dim + rot_dim / 2 + i];

      ASSERT_LT(fabs(actual_cos - expected_cos), 1e-5);
      ASSERT_LT(fabs(actual_sin - expected_sin), 1e-5);
    }
  }

  free(cache);
}

TEST(rope_cos_sin_cache_position_zero) {
  const int max_pos = 10;
  const int rot_dim = 32;
  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, 10000.0f);

  for (int i = 0; i < rot_dim / 2; i++) {
    ASSERT_NEAR(cache[i], 1.0f, 1e-5);
    ASSERT_NEAR(cache[rot_dim / 2 + i], 0.0f, 1e-5);
  }

  free(cache);
}

/* ============ NeoX Style Tests ============ */

TEST(rope_neox_f32_single_token) {
  const int num_tokens = 1;
  const int num_heads = 4;
  const int head_size = 64;
  const int rot_dim = 64;
  const int max_pos = 128;

  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));
  float *query =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  float *query_ref =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  int64_t positions[1] = {5};

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, 10000.0f);

  for (int i = 0; i < num_tokens * num_heads * head_size; i++) {
    query[i] = query_ref[i] = ((float)(i % 100) / 50.0f) - 1.0f;
  }

  rope_f32(positions, query, NULL, cache, num_tokens, num_heads, num_heads,
           head_size, rot_dim, true);
  compute_rope_reference_neox(positions, query_ref, NULL, cache, num_tokens,
                              num_heads, num_heads, head_size, rot_dim);

  double max_err =
      compute_max_error(query, query_ref, num_tokens * num_heads * head_size);
  ASSERT_LT(max_err, 1e-5);

  free(cache);
  free(query);
  free(query_ref);
}

TEST(rope_neox_f32_batch) {
  const int num_tokens = 32;
  const int num_heads = 8;
  const int num_kv_heads = 2;
  const int head_size = 128;
  const int rot_dim = 128;
  const int max_pos = 512;

  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));
  float *query =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  float *query_ref =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  float *key =
      (float *)malloc(num_tokens * num_kv_heads * head_size * sizeof(float));
  float *key_ref =
      (float *)malloc(num_tokens * num_kv_heads * head_size * sizeof(float));
  int64_t *positions = (int64_t *)malloc(num_tokens * sizeof(int64_t));

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, 10000.0f);

  for (int i = 0; i < num_tokens; i++) {
    positions[i] = i * 3;
  }

  for (int i = 0; i < num_tokens * num_heads * head_size; i++) {
    query[i] = query_ref[i] = ((float)(i % 200) / 100.0f) - 1.0f;
  }

  for (int i = 0; i < num_tokens * num_kv_heads * head_size; i++) {
    key[i] = key_ref[i] = ((float)(i % 150) / 75.0f) - 1.0f;
  }

  rope_f32(positions, query, key, cache, num_tokens, num_heads, num_kv_heads,
           head_size, rot_dim, true);
  compute_rope_reference_neox(positions, query_ref, key_ref, cache, num_tokens,
                              num_heads, num_kv_heads, head_size, rot_dim);

  double max_err_q =
      compute_max_error(query, query_ref, num_tokens * num_heads * head_size);
  double max_err_k =
      compute_max_error(key, key_ref, num_tokens * num_kv_heads * head_size);

  ASSERT_LT(max_err_q, 1e-3);
  ASSERT_LT(max_err_k, 1e-3);

  free(cache);
  free(query);
  free(query_ref);
  free(key);
  free(key_ref);
  free(positions);
}

TEST(rope_neox_f32_partial_rot_dim) {
  const int num_tokens = 4;
  const int num_heads = 4;
  const int head_size = 128;
  const int rot_dim = 64;
  const int max_pos = 128;

  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));
  float *query =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  float *query_ref =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  int64_t positions[4] = {0, 10, 20, 30};

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, 10000.0f);

  for (int i = 0; i < num_tokens * num_heads * head_size; i++) {
    query[i] = query_ref[i] = ((float)(i % 100) / 50.0f) - 1.0f;
  }

  rope_f32(positions, query, NULL, cache, num_tokens, num_heads, num_heads,
           head_size, rot_dim, true);
  compute_rope_reference_neox(positions, query_ref, NULL, cache, num_tokens,
                              num_heads, num_heads, head_size, rot_dim);

  double max_err =
      compute_max_error(query, query_ref, num_tokens * num_heads * head_size);
  ASSERT_LT(max_err, 1e-5);

  free(cache);
  free(query);
  free(query_ref);
}

/* ============ Edge Cases ============ */

TEST(rope_position_zero) {
  const int num_tokens = 1;
  const int num_heads = 2;
  const int head_size = 32;
  const int rot_dim = 32;
  const int max_pos = 10;

  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));
  float *query =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  float *query_orig =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  int64_t positions[1] = {0};

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, 10000.0f);

  for (int i = 0; i < num_tokens * num_heads * head_size; i++) {
    query[i] = query_orig[i] = ((float)i / 10.0f);
  }

  rope_f32(positions, query, NULL, cache, num_tokens, num_heads, num_heads,
           head_size, rot_dim, true);

  double max_err =
      compute_max_error(query, query_orig, num_tokens * num_heads * head_size);
  ASSERT_LT(max_err, 1e-5);

  free(cache);
  free(query);
  free(query_orig);
}

TEST(rope_large_position) {
  const int num_tokens = 1;
  const int num_heads = 2;
  const int head_size = 64;
  const int rot_dim = 64;
  const int max_pos = 8192;

  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));
  float *query =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  float *query_ref =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  int64_t positions[1] = {4096};

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, 10000.0f);

  for (int i = 0; i < num_tokens * num_heads * head_size; i++) {
    query[i] = query_ref[i] = ((float)(i % 100) / 50.0f) - 1.0f;
  }

  rope_f32(positions, query, NULL, cache, num_tokens, num_heads, num_heads,
           head_size, rot_dim, true);
  compute_rope_reference_neox(positions, query_ref, NULL, cache, num_tokens,
                              num_heads, num_heads, head_size, rot_dim);

  double max_err =
      compute_max_error(query, query_ref, num_tokens * num_heads * head_size);
  ASSERT_LT(max_err, 1e-4);

  free(cache);
  free(query);
  free(query_ref);
}

TEST(rope_unaligned_rot_dim) {
  const int num_tokens = 2;
  const int num_heads = 2;
  const int head_size = 128;
  const int rot_dim = 66;
  const int max_pos = 64;

  float *cache = (float *)malloc(max_pos * rot_dim * sizeof(float));
  float *query =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  float *query_ref =
      (float *)malloc(num_tokens * num_heads * head_size * sizeof(float));
  int64_t positions[2] = {5, 10};

  rope_compute_cos_sin_cache_f32(cache, max_pos, rot_dim, 10000.0f);

  for (int i = 0; i < num_tokens * num_heads * head_size; i++) {
    query[i] = query_ref[i] = ((float)(i % 100) / 50.0f) - 1.0f;
  }

  rope_f32(positions, query, NULL, cache, num_tokens, num_heads, num_heads,
           head_size, rot_dim, true);
  compute_rope_reference_neox(positions, query_ref, NULL, cache, num_tokens,
                              num_heads, num_heads, head_size, rot_dim);

  double max_err =
      compute_max_error(query, query_ref, num_tokens * num_heads * head_size);
  ASSERT_LT(max_err, 1e-5);

  free(cache);
  free(query);
  free(query_ref);
}

/* ============ Test Registration ============ */

extern "C" void run_rope_tests(void) {
  TEST_SUITE("RoPE (FP32)");
  RUN_TEST(rope_cos_sin_cache_basic);
  RUN_TEST(rope_cos_sin_cache_position_zero);
  RUN_TEST(rope_neox_f32_single_token);
  RUN_TEST(rope_neox_f32_batch);
  RUN_TEST(rope_neox_f32_partial_rot_dim);
  RUN_TEST(rope_position_zero);
  RUN_TEST(rope_large_position);
  RUN_TEST(rope_unaligned_rot_dim);
}

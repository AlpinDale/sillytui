/*
 * Flash Attention Unit Tests
 */

extern "C" {
#include "inference/kernels/attention/attention.h"
#include "test_framework.h"
}

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

static void reference_attention_f32(float *output, const float *query,
                                    const float *key, const float *value,
                                    int seq_len_q, int seq_len_kv, int head_dim,
                                    float scale, const float *mask) {
  std::vector<float> scores(seq_len_q * seq_len_kv);
  std::vector<float> probs(seq_len_q * seq_len_kv);

  for (int q = 0; q < seq_len_q; q++) {
    float max_score = -INFINITY;
    for (int k = 0; k < seq_len_kv; k++) {
      float score = 0.0f;
      for (int d = 0; d < head_dim; d++) {
        score += query[q * head_dim + d] * key[k * head_dim + d];
      }
      score *= scale;
      if (mask) {
        score += mask[q * seq_len_kv + k];
      }
      scores[q * seq_len_kv + k] = score;
      if (score > max_score)
        max_score = score;
    }

    float sum_exp = 0.0f;
    for (int k = 0; k < seq_len_kv; k++) {
      float prob = expf(scores[q * seq_len_kv + k] - max_score);
      probs[q * seq_len_kv + k] = prob;
      sum_exp += prob;
    }

    for (int k = 0; k < seq_len_kv; k++) {
      probs[q * seq_len_kv + k] /= sum_exp;
    }

    for (int d = 0; d < head_dim; d++) {
      float out = 0.0f;
      for (int k = 0; k < seq_len_kv; k++) {
        out += probs[q * seq_len_kv + k] * value[k * head_dim + d];
      }
      output[q * head_dim + d] = out;
    }
  }
}

static double compute_max_rel_error(const float *a, const float *b, int n) {
  double max_err = 0.0;
  for (int i = 0; i < n; i++) {
    double diff = fabs((double)a[i] - (double)b[i]);
    double mag = fabs((double)b[i]);
    double rel_err = diff / (mag + 1e-6);
    if (rel_err > max_err)
      max_err = rel_err;
  }
  return max_err;
}

TEST(flash_attention_f32_basic) {
  const int seq_len = 4;
  const int head_dim = 8;
  const float scale = 1.0f / sqrtf((float)head_dim);

  std::vector<float> q(seq_len * head_dim);
  std::vector<float> k(seq_len * head_dim);
  std::vector<float> v(seq_len * head_dim);
  std::vector<float> out(seq_len * head_dim);
  std::vector<float> expected(seq_len * head_dim);

  for (int i = 0; i < seq_len * head_dim; i++) {
    q[i] = (float)(i % 7) / 7.0f;
    k[i] = (float)((i + 3) % 7) / 7.0f;
    v[i] = (float)((i + 5) % 7) / 7.0f;
  }

  reference_attention_f32(expected.data(), q.data(), k.data(), v.data(),
                          seq_len, seq_len, head_dim, scale, nullptr);
  flash_attention_f32(out.data(), q.data(), k.data(), v.data(), seq_len,
                      seq_len, head_dim, scale, nullptr);

  double max_err =
      compute_max_rel_error(out.data(), expected.data(), seq_len * head_dim);
  ASSERT_LT(max_err, 1e-5);
}

TEST(flash_attention_f32_larger) {
  const int seq_len = 32;
  const int head_dim = 64;
  const float scale = 1.0f / sqrtf((float)head_dim);

  std::vector<float> q(seq_len * head_dim);
  std::vector<float> k(seq_len * head_dim);
  std::vector<float> v(seq_len * head_dim);
  std::vector<float> out(seq_len * head_dim);
  std::vector<float> expected(seq_len * head_dim);

  for (int i = 0; i < seq_len * head_dim; i++) {
    q[i] = ((float)(i % 17) - 8.0f) / 8.0f;
    k[i] = ((float)((i + 5) % 17) - 8.0f) / 8.0f;
    v[i] = ((float)((i + 11) % 17) - 8.0f) / 8.0f;
  }

  reference_attention_f32(expected.data(), q.data(), k.data(), v.data(),
                          seq_len, seq_len, head_dim, scale, nullptr);
  flash_attention_f32(out.data(), q.data(), k.data(), v.data(), seq_len,
                      seq_len, head_dim, scale, nullptr);

  double max_err =
      compute_max_rel_error(out.data(), expected.data(), seq_len * head_dim);
  ASSERT_LT(max_err, 1e-4);
}

TEST(flash_attention_f32_causal_mask) {
  const int seq_len = 8;
  const int head_dim = 16;
  const float scale = 1.0f / sqrtf((float)head_dim);

  std::vector<float> q(seq_len * head_dim);
  std::vector<float> k(seq_len * head_dim);
  std::vector<float> v(seq_len * head_dim);
  std::vector<float> mask(seq_len * seq_len);
  std::vector<float> out(seq_len * head_dim);
  std::vector<float> expected(seq_len * head_dim);

  for (int i = 0; i < seq_len * head_dim; i++) {
    q[i] = (float)(i % 11) / 11.0f;
    k[i] = (float)((i + 3) % 11) / 11.0f;
    v[i] = (float)((i + 7) % 11) / 11.0f;
  }

  for (int i = 0; i < seq_len; i++) {
    for (int j = 0; j < seq_len; j++) {
      mask[i * seq_len + j] = (j > i) ? -INFINITY : 0.0f;
    }
  }

  reference_attention_f32(expected.data(), q.data(), k.data(), v.data(),
                          seq_len, seq_len, head_dim, scale, mask.data());
  flash_attention_f32(out.data(), q.data(), k.data(), v.data(), seq_len,
                      seq_len, head_dim, scale, mask.data());

  double max_err =
      compute_max_rel_error(out.data(), expected.data(), seq_len * head_dim);
  ASSERT_LT(max_err, 1e-5);
}

TEST(flash_attention_f32_output_range) {
  const int seq_len = 16;
  const int head_dim = 32;
  const float scale = 1.0f / sqrtf((float)head_dim);

  std::vector<float> q(seq_len * head_dim);
  std::vector<float> k(seq_len * head_dim);
  std::vector<float> v(seq_len * head_dim);
  std::vector<float> out(seq_len * head_dim);

  for (int i = 0; i < seq_len * head_dim; i++) {
    q[i] = (float)(rand() % 100) / 100.0f;
    k[i] = (float)(rand() % 100) / 100.0f;
    v[i] = (float)(rand() % 100) / 100.0f;
  }

  float v_min = v[0], v_max = v[0];
  for (int i = 1; i < seq_len * head_dim; i++) {
    if (v[i] < v_min)
      v_min = v[i];
    if (v[i] > v_max)
      v_max = v[i];
  }

  flash_attention_f32(out.data(), q.data(), k.data(), v.data(), seq_len,
                      seq_len, head_dim, scale, nullptr);

  for (int i = 0; i < seq_len * head_dim; i++) {
    ASSERT(out[i] >= v_min - 0.01f && out[i] <= v_max + 0.01f);
  }
}

TEST(flash_attention_mha_f32_basic) {
  const int batch = 2;
  const int num_heads = 4;
  const int seq_len = 8;
  const int head_dim = 16;
  const float scale = 1.0f / sqrtf((float)head_dim);

  int total = batch * num_heads * seq_len * head_dim;
  std::vector<float> q(total);
  std::vector<float> k(total);
  std::vector<float> v(total);
  std::vector<float> out(total);

  for (int i = 0; i < total; i++) {
    q[i] = (float)(i % 13) / 13.0f;
    k[i] = (float)((i + 5) % 13) / 13.0f;
    v[i] = (float)((i + 9) % 13) / 13.0f;
  }

  flash_attention_mha_f32(out.data(), q.data(), k.data(), v.data(), batch,
                          num_heads, num_heads, seq_len, seq_len, head_dim,
                          scale, nullptr);

  float out_min = out[0], out_max = out[0];
  for (int i = 1; i < total; i++) {
    if (out[i] < out_min)
      out_min = out[i];
    if (out[i] > out_max)
      out_max = out[i];
  }
  ASSERT(out_min >= 0.0f && out_max <= 1.0f);
}

extern "C" void run_attention_tests(void) {
  TEST_SUITE("Flash Attention");
  RUN_TEST(flash_attention_f32_basic);
  RUN_TEST(flash_attention_f32_larger);
  RUN_TEST(flash_attention_f32_causal_mask);
  RUN_TEST(flash_attention_f32_output_range);
  RUN_TEST(flash_attention_mha_f32_basic);
}

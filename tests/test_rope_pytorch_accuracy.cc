/*
 * RoPE accuracy tests against PyTorch reference data
 */

extern "C" {
#include "inference/kernels/rope/rope.h"
#include "test_framework.h"
}

#include "inference/model_loader/safetensors.hh"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

static safetensors::safetensors_t g_st;
static bool g_loaded = false;

static bool load_rope_reference() {
  if (g_loaded)
    return true;

  std::string warn, err;
  if (!safetensors::mmap_from_file("tests/rope_reference.safetensors", &g_st,
                                   &warn, &err)) {
    printf("Failed to load rope reference: %s\n", err.c_str());
    return false;
  }

  g_loaded = true;
  return true;
}

static const float *get_tensor(const char *name, size_t *count) {
  const auto &keys = g_st.tensors.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      safetensors::tensor_t tensor;
      g_st.tensors.at(i, &tensor);
      size_t data_len = tensor.data_offsets[1] - tensor.data_offsets[0];
      *count = data_len / sizeof(float);
      return reinterpret_cast<const float *>(g_st.databuffer_addr +
                                             tensor.data_offsets[0]);
    }
  }
  return nullptr;
}

static const int64_t *get_positions(const char *name, size_t *count) {
  const auto &keys = g_st.tensors.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      safetensors::tensor_t tensor;
      g_st.tensors.at(i, &tensor);
      size_t data_len = tensor.data_offsets[1] - tensor.data_offsets[0];
      *count = data_len / sizeof(int64_t);
      return reinterpret_cast<const int64_t *>(g_st.databuffer_addr +
                                               tensor.data_offsets[0]);
    }
  }
  return nullptr;
}

static int get_metadata_int(const char *name) {
  const auto &keys = g_st.metadata.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      std::string val;
      g_st.metadata.at(i, &val);
      return std::stoi(val);
    }
  }
  return 0;
}

static double compute_max_rel_error(const float *a, const float *b, size_t n) {
  double max_err = 0.0;
  for (size_t i = 0; i < n; i++) {
    double diff = fabs((double)a[i] - (double)b[i]);
    double mag = fmax(fabs((double)b[i]), 1e-6);
    double err = diff / mag;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

static const uint16_t *get_tensor_bf16(const char *name, size_t *count) {
  const auto &keys = g_st.tensors.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      safetensors::tensor_t tensor;
      g_st.tensors.at(i, &tensor);
      if (tensor.dtype != safetensors::dtype::kBFLOAT16) {
        return nullptr;
      }
      size_t data_len = tensor.data_offsets[1] - tensor.data_offsets[0];
      *count = data_len / sizeof(uint16_t);
      return reinterpret_cast<const uint16_t *>(g_st.databuffer_addr +
                                                tensor.data_offsets[0]);
    }
  }
  return nullptr;
}

static const uint16_t *get_tensor_f16(const char *name, size_t *count) {
  const auto &keys = g_st.tensors.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == name) {
      safetensors::tensor_t tensor;
      g_st.tensors.at(i, &tensor);
      if (tensor.dtype != safetensors::dtype::kFLOAT16) {
        return nullptr;
      }
      size_t data_len = tensor.data_offsets[1] - tensor.data_offsets[0];
      *count = data_len / sizeof(uint16_t);
      return reinterpret_cast<const uint16_t *>(g_st.databuffer_addr +
                                                tensor.data_offsets[0]);
    }
  }
  return nullptr;
}

static float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static float fp16_to_float(uint16_t fp16) {
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

static double compute_max_rel_error_bf16(const uint16_t *a, const uint16_t *b,
                                         size_t n, double rtol = 0.02,
                                         double atol = 0.01) {
  double max_err = 0.0;
  for (size_t i = 0; i < n; i++) {
    double a_f = bf16_to_float(a[i]);
    double b_f = bf16_to_float(b[i]);
    double diff = fabs(a_f - b_f);
    double tol = atol + rtol * fabs(b_f);
    double err = diff / tol;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

static double compute_max_rel_error_f16(const uint16_t *a, const uint16_t *b,
                                        size_t n, double rtol = 0.01,
                                        double atol = 0.005) {
  double max_err = 0.0;
  for (size_t i = 0; i < n; i++) {
    double a_f = fp16_to_float(a[i]);
    double b_f = fp16_to_float(b[i]);
    double diff = fabs(a_f - b_f);
    double tol = atol + rtol * fabs(b_f);
    double err = diff / tol;
    if (err > max_err)
      max_err = err;
  }
  return max_err;
}

static void run_rope_test(const char *test_name, double tolerance) {
  if (!load_rope_reference()) {
    FAIL("Failed to load rope reference data");
    return;
  }

  char buf[256];
  snprintf(buf, sizeof(buf), "%s_positions", test_name);
  size_t pos_count = 0;
  const int64_t *positions = get_positions(buf, &pos_count);

  snprintf(buf, sizeof(buf), "%s_query_input", test_name);
  size_t q_in_count = 0;
  const float *q_input = get_tensor(buf, &q_in_count);

  snprintf(buf, sizeof(buf), "%s_key_input", test_name);
  size_t k_in_count = 0;
  const float *k_input = get_tensor(buf, &k_in_count);

  snprintf(buf, sizeof(buf), "%s_cos_sin_cache", test_name);
  size_t cache_count = 0;
  const float *cache = get_tensor(buf, &cache_count);

  snprintf(buf, sizeof(buf), "%s_query_expected", test_name);
  size_t q_exp_count = 0;
  const float *q_expected = get_tensor(buf, &q_exp_count);

  snprintf(buf, sizeof(buf), "%s_key_expected", test_name);
  size_t k_exp_count = 0;
  const float *k_expected = get_tensor(buf, &k_exp_count);

  snprintf(buf, sizeof(buf), "%s_num_tokens", test_name);
  int num_tokens = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_num_heads", test_name);
  int num_heads = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_num_kv_heads", test_name);
  int num_kv_heads = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_head_size", test_name);
  int head_size = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_rot_dim", test_name);
  int rot_dim = get_metadata_int(buf);

  if (!positions || !q_input || !k_input || !cache || !q_expected ||
      !k_expected) {
    FAIL_FMT("Missing tensors for test case: %s (pos=%p q=%p k=%p cache=%p "
             "qe=%p ke=%p)",
             test_name, (void *)positions, (void *)q_input, (void *)k_input,
             (void *)cache, (void *)q_expected, (void *)k_expected);
    return;
  }

  if (num_tokens == 0 || num_heads == 0 || head_size == 0 || rot_dim == 0) {
    FAIL_FMT(
        "Invalid metadata for test case: %s (tok=%d heads=%d hs=%d rot=%d)",
        test_name, num_tokens, num_heads, head_size, rot_dim);
    return;
  }

  float *query = (float *)malloc(q_in_count * sizeof(float));
  float *key = (float *)malloc(k_in_count * sizeof(float));
  if (!query || !key) {
    FAIL("Memory allocation failed");
    free(query);
    free(key);
    return;
  }
  memcpy(query, q_input, q_in_count * sizeof(float));
  memcpy(key, k_input, k_in_count * sizeof(float));

  rope_f32(positions, query, key, cache, num_tokens, num_heads, num_kv_heads,
           head_size, rot_dim, true);

  double q_err = compute_max_rel_error(query, q_expected, q_in_count);
  double k_err = compute_max_rel_error(key, k_expected, k_in_count);

  if (q_err >= tolerance) {
    FAIL_FMT("Query error too high: %.6e (threshold: %.6e)", q_err, tolerance);
  } else if (k_err >= tolerance) {
    FAIL_FMT("Key error too high: %.6e (threshold: %.6e)", k_err, tolerance);
  } else {
    PASS();
  }

  free(query);
  free(key);
}

TEST(pytorch_rope_single_token_pos0) {
  run_rope_test("single_token_pos0", 1e-5);
}

TEST(pytorch_rope_single_token_pos10) {
  run_rope_test("single_token_pos10", 1e-5);
}

TEST(pytorch_rope_single_token_pos100) {
  run_rope_test("single_token_pos100", 1e-5);
}

TEST(pytorch_rope_batch_sequential) { run_rope_test("batch_sequential", 1e-4); }

TEST(pytorch_rope_batch_nonseq) { run_rope_test("batch_nonseq", 1e-4); }

TEST(pytorch_rope_gqa_decode) { run_rope_test("gqa_decode", 1e-5); }

TEST(pytorch_rope_gqa_prefill) { run_rope_test("gqa_prefill", 1e-4); }

TEST(pytorch_rope_partial_rot) { run_rope_test("partial_rot", 1e-4); }

TEST(pytorch_rope_large_batch) { run_rope_test("large_batch", 1e-3); }

TEST(pytorch_rope_large_position) { run_rope_test("large_position", 1e-3); }

/* ============ BF16 Tests ============ */

static void run_rope_test_bf16(const char *test_name, double tolerance) {
  if (!load_rope_reference()) {
    FAIL("Failed to load rope reference data");
    return;
  }

  char buf[256];
  snprintf(buf, sizeof(buf), "%s_positions", test_name);
  size_t pos_count = 0;
  const int64_t *positions = get_positions(buf, &pos_count);

  snprintf(buf, sizeof(buf), "%s_query_input", test_name);
  size_t q_in_count = 0;
  const uint16_t *q_input = get_tensor_bf16(buf, &q_in_count);

  snprintf(buf, sizeof(buf), "%s_key_input", test_name);
  size_t k_in_count = 0;
  const uint16_t *k_input = get_tensor_bf16(buf, &k_in_count);

  snprintf(buf, sizeof(buf), "%s_cos_sin_cache", test_name);
  size_t cache_count = 0;
  const uint16_t *cache = get_tensor_bf16(buf, &cache_count);

  snprintf(buf, sizeof(buf), "%s_query_expected", test_name);
  size_t q_exp_count = 0;
  const uint16_t *q_expected = get_tensor_bf16(buf, &q_exp_count);

  snprintf(buf, sizeof(buf), "%s_key_expected", test_name);
  size_t k_exp_count = 0;
  const uint16_t *k_expected = get_tensor_bf16(buf, &k_exp_count);

  snprintf(buf, sizeof(buf), "%s_num_tokens", test_name);
  int num_tokens = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_num_heads", test_name);
  int num_heads = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_num_kv_heads", test_name);
  int num_kv_heads = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_head_size", test_name);
  int head_size = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_rot_dim", test_name);
  int rot_dim = get_metadata_int(buf);

  if (!positions || !q_input || !k_input || !cache || !q_expected ||
      !k_expected) {
    FAIL_FMT("Missing tensors for BF16 test case: %s", test_name);
    return;
  }

  uint16_t *query = (uint16_t *)malloc(q_in_count * sizeof(uint16_t));
  uint16_t *key = (uint16_t *)malloc(k_in_count * sizeof(uint16_t));
  memcpy(query, q_input, q_in_count * sizeof(uint16_t));
  memcpy(key, k_input, k_in_count * sizeof(uint16_t));

  rope_bf16(positions, query, key, cache, num_tokens, num_heads, num_kv_heads,
            head_size, rot_dim, true);

  double q_err = compute_max_rel_error_bf16(query, q_expected, q_in_count);
  double k_err = compute_max_rel_error_bf16(key, k_expected, k_in_count);

  if (q_err >= tolerance) {
    FAIL_FMT("BF16 Query error too high: %.6e (threshold: %.6e)", q_err,
             tolerance);
  } else if (k_err >= tolerance) {
    FAIL_FMT("BF16 Key error too high: %.6e (threshold: %.6e)", k_err,
             tolerance);
  } else {
    PASS();
  }

  free(query);
  free(key);
}

TEST(pytorch_rope_decode_bf16) { run_rope_test_bf16("decode_bf16", 1.0); }
TEST(pytorch_rope_batch_bf16) { run_rope_test_bf16("batch_bf16", 1.0); }

/* ============ FP16 Tests ============ */

static void run_rope_test_f16(const char *test_name, double tolerance) {
  if (!load_rope_reference()) {
    FAIL("Failed to load rope reference data");
    return;
  }

  char buf[256];
  snprintf(buf, sizeof(buf), "%s_positions", test_name);
  size_t pos_count = 0;
  const int64_t *positions = get_positions(buf, &pos_count);

  snprintf(buf, sizeof(buf), "%s_query_input", test_name);
  size_t q_in_count = 0;
  const uint16_t *q_input = get_tensor_f16(buf, &q_in_count);

  snprintf(buf, sizeof(buf), "%s_key_input", test_name);
  size_t k_in_count = 0;
  const uint16_t *k_input = get_tensor_f16(buf, &k_in_count);

  snprintf(buf, sizeof(buf), "%s_cos_sin_cache", test_name);
  size_t cache_count = 0;
  const uint16_t *cache = get_tensor_f16(buf, &cache_count);

  snprintf(buf, sizeof(buf), "%s_query_expected", test_name);
  size_t q_exp_count = 0;
  const uint16_t *q_expected = get_tensor_f16(buf, &q_exp_count);

  snprintf(buf, sizeof(buf), "%s_key_expected", test_name);
  size_t k_exp_count = 0;
  const uint16_t *k_expected = get_tensor_f16(buf, &k_exp_count);

  snprintf(buf, sizeof(buf), "%s_num_tokens", test_name);
  int num_tokens = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_num_heads", test_name);
  int num_heads = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_num_kv_heads", test_name);
  int num_kv_heads = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_head_size", test_name);
  int head_size = get_metadata_int(buf);
  snprintf(buf, sizeof(buf), "%s_rot_dim", test_name);
  int rot_dim = get_metadata_int(buf);

  if (!positions || !q_input || !k_input || !cache || !q_expected ||
      !k_expected) {
    FAIL_FMT("Missing tensors for FP16 test case: %s", test_name);
    return;
  }

  uint16_t *query = (uint16_t *)malloc(q_in_count * sizeof(uint16_t));
  uint16_t *key = (uint16_t *)malloc(k_in_count * sizeof(uint16_t));
  memcpy(query, q_input, q_in_count * sizeof(uint16_t));
  memcpy(key, k_input, k_in_count * sizeof(uint16_t));

  rope_f16(positions, query, key, cache, num_tokens, num_heads, num_kv_heads,
           head_size, rot_dim, true);

  double q_err = compute_max_rel_error_f16(query, q_expected, q_in_count);
  double k_err = compute_max_rel_error_f16(key, k_expected, k_in_count);

  if (q_err >= tolerance) {
    FAIL_FMT("FP16 Query error too high: %.6e (threshold: %.6e)", q_err,
             tolerance);
  } else if (k_err >= tolerance) {
    FAIL_FMT("FP16 Key error too high: %.6e (threshold: %.6e)", k_err,
             tolerance);
  } else {
    PASS();
  }

  free(query);
  free(key);
}

TEST(pytorch_rope_decode_f16) { run_rope_test_f16("decode_f16", 1.0); }
TEST(pytorch_rope_batch_f16) { run_rope_test_f16("batch_f16", 1.0); }

extern "C" void run_rope_pytorch_tests(void) {
  TEST_SUITE("RoPE (FP32/BF16/FP16) - PyTorch Accuracy");
  RUN_TEST(pytorch_rope_single_token_pos0);
  RUN_TEST(pytorch_rope_single_token_pos10);
  RUN_TEST(pytorch_rope_single_token_pos100);
  RUN_TEST(pytorch_rope_batch_sequential);
  RUN_TEST(pytorch_rope_batch_nonseq);
  RUN_TEST(pytorch_rope_gqa_decode);
  RUN_TEST(pytorch_rope_gqa_prefill);
  RUN_TEST(pytorch_rope_partial_rot);
  RUN_TEST(pytorch_rope_large_batch);
  RUN_TEST(pytorch_rope_large_position);
  RUN_TEST(pytorch_rope_decode_bf16);
  RUN_TEST(pytorch_rope_batch_bf16);
  RUN_TEST(pytorch_rope_decode_f16);
  RUN_TEST(pytorch_rope_batch_f16);
}

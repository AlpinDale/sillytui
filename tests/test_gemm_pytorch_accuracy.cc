/*
 * GEMM Accuracy Tests vs PyTorch Reference
 *
 * Compares our GEMM implementations against PyTorch-generated reference results
 * to ensure numerical accuracy across FP32, BF16, and FP16.
 */

#include "inference/model_loader/safetensors.hh"
#include "test_framework.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include "inference/kernels/gemm/gemm.h"
}

struct TestCase {
  int M, N, K;
  float *A;
  float *B;
  float *C_ref; // PyTorch reference result
};

static safetensors::safetensors_t g_reference_data;
static bool g_reference_loaded = false;

static bool load_reference_data() {
  if (g_reference_loaded)
    return true;

  std::string warn, err;
  bool ret = safetensors::mmap_from_file("tests/gemm_reference.safetensors",
                                         &g_reference_data, &warn, &err);
  if (!ret) {
    fprintf(stderr, "Failed to load reference data: %s\n", err.c_str());
    return false;
  }

  g_reference_loaded = true;
  return true;
}

static bool load_test_case(const char *test_name, TestCase *tc) {
  if (!load_reference_data())
    return false;

  // Read metadata by iterating
  std::string m_key = std::string(test_name) + ".M";
  std::string n_key = std::string(test_name) + ".N";
  std::string k_key = std::string(test_name) + ".K";

  bool found_m = false, found_n = false, found_k = false;

  for (size_t i = 0; i < g_reference_data.metadata.size(); i++) {
    std::string key = g_reference_data.metadata.keys()[i];
    std::string val;
    g_reference_data.metadata.at(i, &val);

    if (key == m_key) {
      tc->M = std::stoi(val);
      found_m = true;
    } else if (key == n_key) {
      tc->N = std::stoi(val);
      found_n = true;
    } else if (key == k_key) {
      tc->K = std::stoi(val);
      found_k = true;
    }
  }

  if (!found_m || !found_n || !found_k) {
    fprintf(stderr, "Missing metadata for %s\n", test_name);
    return false;
  }

  // Load tensors by iterating
  std::string a_key = std::string(test_name) + ".A";
  std::string b_key = std::string(test_name) + ".B";
  std::string c_key = std::string(test_name) + ".C";

  safetensors::tensor_t tensor_a, tensor_b, tensor_c;
  bool found_a = false, found_b = false, found_c = false;

  for (size_t i = 0; i < g_reference_data.tensors.size(); i++) {
    std::string key = g_reference_data.tensors.keys()[i];
    if (key == a_key) {
      g_reference_data.tensors.at(i, &tensor_a);
      found_a = true;
    } else if (key == b_key) {
      g_reference_data.tensors.at(i, &tensor_b);
      found_b = true;
    } else if (key == c_key) {
      g_reference_data.tensors.at(i, &tensor_c);
      found_c = true;
    }
  }

  if (!found_a || !found_b || !found_c) {
    fprintf(stderr, "Missing tensors for %s\n", test_name);
    return false;
  }

  // Allocate and copy data
  tc->A = (float *)malloc(tc->M * tc->K * sizeof(float));
  tc->B = (float *)malloc(tc->K * tc->N * sizeof(float));
  tc->C_ref = (float *)malloc(tc->M * tc->N * sizeof(float));

  if (!tc->A || !tc->B || !tc->C_ref) {
    return false;
  }

  const uint8_t *data_a =
      g_reference_data.databuffer_addr + tensor_a.data_offsets[0];
  const uint8_t *data_b =
      g_reference_data.databuffer_addr + tensor_b.data_offsets[0];
  const uint8_t *data_c =
      g_reference_data.databuffer_addr + tensor_c.data_offsets[0];

  memcpy(tc->A, data_a, tc->M * tc->K * sizeof(float));
  memcpy(tc->B, data_b, tc->K * tc->N * sizeof(float));
  memcpy(tc->C_ref, data_c, tc->M * tc->N * sizeof(float));

  return true;
}

static void free_test_case(TestCase *tc) {
  free(tc->A);
  free(tc->B);
  free(tc->C_ref);
}

static double compute_combined_error(const float *a, const float *b, int n,
                                     int *worst_idx, double rtol, double atol) {
  double max_err = 0.0;
  *worst_idx = -1;
  for (int i = 0; i < n; i++) {
    double diff = fabs((double)a[i] - (double)b[i]);
    double mag = fmax(fabs((double)a[i]), fabs((double)b[i]));

    // Combined error: diff / (atol + rtol * mag)
    // Values are considered equal if diff <= atol + rtol * mag
    double threshold = atol + rtol * mag;
    double err = (threshold > 0) ? (diff / threshold) : diff;

    if (err > max_err) {
      max_err = err;
      *worst_idx = i;
    }
  }
  return max_err;
}

TEST(pytorch_accuracy_medium_fp32) {
  TestCase tc;
  ASSERT(load_test_case("medium_fp32", &tc));

  float *C_ours = (float *)malloc(tc.M * tc.N * sizeof(float));
  ASSERT(C_ours != NULL);

  gemm_f32(tc.A, tc.B, C_ours, tc.M, tc.N, tc.K, false, false);

  int worst_idx;
  double err = compute_combined_error(C_ours, tc.C_ref, tc.M * tc.N, &worst_idx,
                                      1e-5, 1e-8);

  // FP32 should be very accurate
  if (err >= 1.0) {
    fprintf(stderr, "FP32 accuracy check failed: err = %e at index %d\n", err,
            worst_idx);
  }
  ASSERT(err < 1.0);

  free(C_ours);
  free_test_case(&tc);
  PASS();
}

TEST(pytorch_accuracy_medium_bf16) {
  TestCase tc;
  ASSERT(load_test_case("medium_bf16", &tc));

  // Convert to BF16
  uint16_t *A_bf16 = (uint16_t *)malloc(tc.M * tc.K * sizeof(uint16_t));
  uint16_t *B_bf16 = (uint16_t *)malloc(tc.K * tc.N * sizeof(uint16_t));
  uint16_t *C_bf16 = (uint16_t *)malloc(tc.M * tc.N * sizeof(uint16_t));
  ASSERT(A_bf16 != NULL && B_bf16 != NULL && C_bf16 != NULL);

  f32_array_to_bf16(tc.A, A_bf16, tc.M * tc.K);
  f32_array_to_bf16(tc.B, B_bf16, tc.K * tc.N);

  gemm_bf16(A_bf16, B_bf16, C_bf16, tc.M, tc.N, tc.K);

  // Convert result back to FP32
  float *C_ours = (float *)malloc(tc.M * tc.N * sizeof(float));
  ASSERT(C_ours != NULL);
  bf16_array_to_f32(C_bf16, C_ours, tc.M * tc.N);

  int worst_idx;
  double err = compute_combined_error(C_ours, tc.C_ref, tc.M * tc.N, &worst_idx,
                                      0.05, 0.05);

  // BF16 has ~3 decimal digits of precision
  if (err >= 1.0) {
    fprintf(stderr,
            "BF16 accuracy check failed: err = %e at index %d (expected=%.6f, "
            "actual=%.6f)\n",
            err, worst_idx, tc.C_ref[worst_idx], C_ours[worst_idx]);
  }
  ASSERT(err < 1.0);

  free(C_ours);
  free(A_bf16);
  free(B_bf16);
  free(C_bf16);
  free_test_case(&tc);
  PASS();
}

TEST(pytorch_accuracy_medium_fp16) {
  TestCase tc;
  ASSERT(load_test_case("medium_fp16", &tc));

  // Convert to FP16
  uint16_t *A_fp16 = (uint16_t *)malloc(tc.M * tc.K * sizeof(uint16_t));
  uint16_t *B_fp16 = (uint16_t *)malloc(tc.K * tc.N * sizeof(uint16_t));
  uint16_t *C_fp16 = (uint16_t *)malloc(tc.M * tc.N * sizeof(uint16_t));
  ASSERT(A_fp16 != NULL && B_fp16 != NULL && C_fp16 != NULL);

  f32_array_to_f16(tc.A, A_fp16, tc.M * tc.K);
  f32_array_to_f16(tc.B, B_fp16, tc.K * tc.N);

  gemm_f16(A_fp16, B_fp16, C_fp16, tc.M, tc.N, tc.K);

  // Convert result back to FP32
  float *C_ours = (float *)malloc(tc.M * tc.N * sizeof(float));
  ASSERT(C_ours != NULL);
  f16_array_to_f32(C_fp16, C_ours, tc.M * tc.N);

  int worst_idx;
  double err = compute_combined_error(C_ours, tc.C_ref, tc.M * tc.N, &worst_idx,
                                      0.05, 0.05);

  // FP16 has ~3 decimal digits of precision
  if (err >= 1.0) {
    fprintf(stderr,
            "FP16 medium accuracy check failed: err = %e at index %d "
            "(expected=%.6f, actual=%.6f)\n",
            err, worst_idx, tc.C_ref[worst_idx], C_ours[worst_idx]);
  }
  ASSERT(err < 1.0);

  free(C_ours);
  free(A_fp16);
  free(B_fp16);
  free(C_fp16);
  free_test_case(&tc);
  PASS();
}

TEST(pytorch_accuracy_llm_fp32) {
  TestCase tc;
  ASSERT(load_test_case("llm_fp32", &tc));

  float *C_ours = (float *)malloc(tc.M * tc.N * sizeof(float));
  ASSERT(C_ours != NULL);

  gemm_f32(tc.A, tc.B, C_ours, tc.M, tc.N, tc.K, false, false);

  int worst_idx;
  double err = compute_combined_error(C_ours, tc.C_ref, tc.M * tc.N, &worst_idx,
                                      1e-5, 1e-8);

  if (err >= 1.0) {
    fprintf(stderr, "FP32 LLM accuracy check failed: err = %e at index %d\n",
            err, worst_idx);
  }
  ASSERT(err < 1.0);

  free(C_ours);
  free_test_case(&tc);
  PASS();
}

TEST(pytorch_accuracy_llm_bf16) {
  TestCase tc;
  ASSERT(load_test_case("llm_bf16", &tc));

  uint16_t *A_bf16 = (uint16_t *)malloc(tc.M * tc.K * sizeof(uint16_t));
  uint16_t *B_bf16 = (uint16_t *)malloc(tc.K * tc.N * sizeof(uint16_t));
  uint16_t *C_bf16 = (uint16_t *)malloc(tc.M * tc.N * sizeof(uint16_t));
  ASSERT(A_bf16 != NULL && B_bf16 != NULL && C_bf16 != NULL);

  f32_array_to_bf16(tc.A, A_bf16, tc.M * tc.K);
  f32_array_to_bf16(tc.B, B_bf16, tc.K * tc.N);

  gemm_bf16(A_bf16, B_bf16, C_bf16, tc.M, tc.N, tc.K);

  float *C_ours = (float *)malloc(tc.M * tc.N * sizeof(float));
  ASSERT(C_ours != NULL);
  bf16_array_to_f32(C_bf16, C_ours, tc.M * tc.N);

  int worst_idx;
  double err = compute_combined_error(C_ours, tc.C_ref, tc.M * tc.N, &worst_idx,
                                      0.05, 0.05);

  if (err >= 1.0) {
    fprintf(stderr, "BF16 LLM accuracy check failed: err = %e at index %d\n",
            err, worst_idx);
  }
  ASSERT(err < 1.0);

  free(C_ours);
  free(A_bf16);
  free(B_bf16);
  free(C_bf16);
  free_test_case(&tc);
  PASS();
}

TEST(pytorch_accuracy_llm_fp16) {
  TestCase tc;
  ASSERT(load_test_case("llm_fp16", &tc));

  uint16_t *A_fp16 = (uint16_t *)malloc(tc.M * tc.K * sizeof(uint16_t));
  uint16_t *B_fp16 = (uint16_t *)malloc(tc.K * tc.N * sizeof(uint16_t));
  uint16_t *C_fp16 = (uint16_t *)malloc(tc.M * tc.N * sizeof(uint16_t));
  ASSERT(A_fp16 != NULL && B_fp16 != NULL && C_fp16 != NULL);

  f32_array_to_f16(tc.A, A_fp16, tc.M * tc.K);
  f32_array_to_f16(tc.B, B_fp16, tc.K * tc.N);

  gemm_f16(A_fp16, B_fp16, C_fp16, tc.M, tc.N, tc.K);

  float *C_ours = (float *)malloc(tc.M * tc.N * sizeof(float));
  ASSERT(C_ours != NULL);
  f16_array_to_f32(C_fp16, C_ours, tc.M * tc.N);

  int worst_idx;
  // Large K=3072 dimension causes more accumulation error in FP16
  double err = compute_combined_error(C_ours, tc.C_ref, tc.M * tc.N, &worst_idx,
                                      0.1, 0.1);

  if (err >= 1.0) {
    fprintf(stderr, "FP16 LLM accuracy check failed: err = %e at index %d\n",
            err, worst_idx);
  }
  ASSERT(err < 1.0);

  free(C_ours);
  free(A_fp16);
  free(B_fp16);
  free(C_fp16);
  free_test_case(&tc);
  PASS();
}

extern "C" void run_gemm_pytorch_accuracy_tests(void) {
  TEST_SUITE("GEMM (FP32/FP16/BF16) - PyTorch Accuracy");
  RUN_TEST(pytorch_accuracy_medium_fp32);
  RUN_TEST(pytorch_accuracy_medium_bf16);
  RUN_TEST(pytorch_accuracy_medium_fp16);
  RUN_TEST(pytorch_accuracy_llm_fp32);
  RUN_TEST(pytorch_accuracy_llm_bf16);
  RUN_TEST(pytorch_accuracy_llm_fp16);
}

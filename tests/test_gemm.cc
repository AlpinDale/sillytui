#include "test_framework.h"
#include <cmath>
#include <cstring>

#include "inference/model_loader/safetensors.hh"

extern "C" {
#include "inference/kernels/gemm/gemm.h"
#include "inference/kernels/gemm/gemm_kernels.h"
}

static void naive_matmul_f32(const float *A, const float *B, float *C, int M,
                             int N, int K) {
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      float sum = 0.0f;
      for (int k = 0; k < K; k++) {
        sum += A[i * K + k] * B[k * N + j];
      }
      C[i * N + j] = sum;
    }
  }
}

static void extract_hadamard_tensor(const char *key, int expected_dim,
                                    float **out_data, int *out_size) {
  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::mmap_from_file("tests/hadamard.safetensors", &st,
                                         &warn, &err);
  if (!ret) {
    *out_data = nullptr;
    *out_size = 0;
    return;
  }

  for (size_t i = 0; i < st.tensors.size(); i++) {
    std::string tensor_key = st.tensors.keys()[i];
    if (tensor_key == key) {
      safetensors::tensor_t tensor;
      st.tensors.at(i, &tensor);

      if (tensor.dtype != safetensors::dtype::kFLOAT32) {
        *out_data = nullptr;
        *out_size = 0;
        return;
      }

      if (tensor.shape.size() != 2 || tensor.shape[0] != (size_t)expected_dim ||
          tensor.shape[1] != (size_t)expected_dim) {
        *out_data = nullptr;
        *out_size = 0;
        return;
      }

      size_t tensor_size = safetensors::get_shape_size(tensor);
      *out_data = (float *)malloc(tensor_size * sizeof(float));
      if (!*out_data) {
        *out_size = 0;
        return;
      }

      const uint8_t *src = st.databuffer_addr + tensor.data_offsets[0];
      memcpy(*out_data, src, tensor_size * sizeof(float));
      *out_size = expected_dim;
      return;
    }
  }

  *out_data = nullptr;
  *out_size = 0;
}

TEST(gemm_f32_identity_small) {
  float A[4] = {1, 0, 0, 1};
  float B[4] = {2, 3, 4, 5};
  float C[4] = {0};

  gemm_f32(A, B, C, 2, 2, 2, false, false);

  ASSERT_NEAR(2.0f, C[0], 1e-5);
  ASSERT_NEAR(3.0f, C[1], 1e-5);
  ASSERT_NEAR(4.0f, C[2], 1e-5);
  ASSERT_NEAR(5.0f, C[3], 1e-5);

  PASS();
}

TEST(gemm_f32_simple_multiply) {
  float A[6] = {1, 2, 3, 4, 5, 6};
  float B[6] = {7, 8, 9, 10, 11, 12};
  float C[4] = {0};

  gemm_f32(A, B, C, 2, 2, 3, false, false);

  float expected[4];
  naive_matmul_f32(A, B, expected, 2, 2, 3);

  for (int i = 0; i < 4; i++) {
    ASSERT_NEAR(expected[i], C[i], 1e-4);
  }

  PASS();
}

TEST(gemm_f32_with_hadamard_12x12) {
  float *H12 = nullptr;
  int size = 0;
  extract_hadamard_tensor("12", 12, &H12, &size);

  if (!H12 || size != 12) {
    free(H12);
    printf("(skipped - hadamard.safetensors not found) ");
    PASS();
  }

  float *A = (float *)malloc(12 * 12 * sizeof(float));
  float *C = (float *)malloc(12 * 12 * sizeof(float));
  float *expected = (float *)malloc(12 * 12 * sizeof(float));

  for (int i = 0; i < 144; i++) {
    A[i] = (float)(i % 7) * 0.1f;
  }

  gemm_f32(A, H12, C, 12, 12, 12, false, false);
  naive_matmul_f32(A, H12, expected, 12, 12, 12);

  for (int i = 0; i < 144; i++) {
    ASSERT_NEAR(expected[i], C[i], 1e-3);
  }

  free(A);
  free(C);
  free(expected);
  free(H12);

  PASS();
}

TEST(gemm_bf16_to_f32_conversion) {
  uint16_t bf16_vals[4] = {safetensors::float_to_bfloat16(1.5f),
                           safetensors::float_to_bfloat16(-2.25f),
                           safetensors::float_to_bfloat16(0.0f),
                           safetensors::float_to_bfloat16(3.14159f)};

  float f32_vals[4];
  bf16_array_to_f32(bf16_vals, f32_vals, 4);

  ASSERT_NEAR(1.5f, f32_vals[0], 0.01f);
  ASSERT_NEAR(-2.25f, f32_vals[1], 0.01f);
  ASSERT_NEAR(0.0f, f32_vals[2], 1e-6f);
  ASSERT_NEAR(3.14159f, f32_vals[3], 0.01f);

  PASS();
}

TEST(gemm_f32_to_bf16_conversion) {
  float f32_vals[4] = {1.5f, -2.25f, 0.0f, 3.14159f};
  uint16_t bf16_vals[4];

  f32_array_to_bf16(f32_vals, bf16_vals, 4);

  for (int i = 0; i < 4; i++) {
    float roundtrip = safetensors::bfloat16_to_float(bf16_vals[i]);
    ASSERT_NEAR(f32_vals[i], roundtrip, 0.01f);
  }

  PASS();
}

TEST(gemm_bf16_small_matrix) {
  float A_f32[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  float B_f32[4] = {5.0f, 6.0f, 7.0f, 8.0f};
  uint16_t A_bf16[4], B_bf16[4], C_bf16[4];
  float C_f32[4];

  f32_array_to_bf16(A_f32, A_bf16, 4);
  f32_array_to_bf16(B_f32, B_bf16, 4);

  gemm_bf16(A_bf16, B_bf16, C_bf16, 2, 2, 2);

  bf16_array_to_f32(C_bf16, C_f32, 4);

  float expected[4];
  naive_matmul_f32(A_f32, B_f32, expected, 2, 2, 2);

  for (int i = 0; i < 4; i++) {
    ASSERT_NEAR(expected[i], C_f32[i], 0.1f);
  }

  PASS();
}

TEST(gemm_f16_small_matrix) {
  float A_f32[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  float B_f32[4] = {5.0f, 6.0f, 7.0f, 8.0f};
  uint16_t A_f16[4], B_f16[4], C_f16[4];
  float C_f32[4];

  f32_array_to_f16(A_f32, A_f16, 4);
  f32_array_to_f16(B_f32, B_f16, 4);

  gemm_f16(A_f16, B_f16, C_f16, 2, 2, 2);

  f16_array_to_f32(C_f16, C_f32, 4);

  float expected[4];
  naive_matmul_f32(A_f32, B_f32, expected, 2, 2, 2);

  for (int i = 0; i < 4; i++) {
    ASSERT_NEAR(expected[i], C_f32[i], 0.1f);
  }

  PASS();
}

TEST(gemm_f32_zero_dimensions) {
  float A[1] = {0};
  float B[1] = {0};
  float C[1] = {0};

  gemm_f32(A, B, C, 0, 1, 1, false, false);
  gemm_f32(A, B, C, 1, 0, 1, false, false);
  gemm_f32(A, B, C, 1, 1, 0, false, false);

  PASS();
}

TEST(gemm_f32_m1_decode_path) {
  float A[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  float B[32];
  float C[4] = {0};

  for (int i = 0; i < 32; i++) {
    B[i] = (float)(i + 1) * 0.1f;
  }

  gemm_f32(A, B, C, 1, 4, 8, false, false);

  float expected[4];
  naive_matmul_f32(A, B, expected, 1, 4, 8);

  for (int i = 0; i < 4; i++) {
    ASSERT_NEAR(expected[i], C[i], 1e-4);
  }

  PASS();
}

TEST(gemm_f32_exact_16x16) {
  const int M = 16, N = 16, K = 32;
  float *A = (float *)malloc(M * K * sizeof(float));
  float *B = (float *)malloc(K * N * sizeof(float));
  float *C = (float *)malloc(M * N * sizeof(float));
  float *expected = (float *)malloc(M * N * sizeof(float));

  for (int i = 0; i < M * K; i++)
    A[i] = (float)(i % 17) * 0.1f - 0.8f;
  for (int i = 0; i < K * N; i++)
    B[i] = (float)(i % 13) * 0.15f - 0.9f;

  gemm_f32(A, B, C, M, N, K, false, false);
  naive_matmul_f32(A, B, expected, M, N, K);

  for (int i = 0; i < M * N; i++) {
    ASSERT_NEAR(expected[i], C[i], 1e-3);
  }

  free(A);
  free(B);
  free(C);
  free(expected);
  PASS();
}

TEST(gemm_f32_tail_m23_n19) {
  const int M = 23, N = 19, K = 64;
  float *A = (float *)malloc(M * K * sizeof(float));
  float *B = (float *)malloc(K * N * sizeof(float));
  float *C = (float *)malloc(M * N * sizeof(float));
  float *expected = (float *)malloc(M * N * sizeof(float));

  for (int i = 0; i < M * K; i++)
    A[i] = (float)(i % 11) * 0.2f - 1.0f;
  for (int i = 0; i < K * N; i++)
    B[i] = (float)(i % 7) * 0.25f - 0.8f;

  gemm_f32(A, B, C, M, N, K, false, false);
  naive_matmul_f32(A, B, expected, M, N, K);

  for (int i = 0; i < M * N; i++) {
    ASSERT_NEAR(expected[i], C[i], 1e-3);
  }

  free(A);
  free(B);
  free(C);
  free(expected);
  PASS();
}

TEST(gemm_f32_large_256x256) {
  const int M = 256, N = 256, K = 128;
  float *A = (float *)malloc(M * K * sizeof(float));
  float *B = (float *)malloc(K * N * sizeof(float));
  float *C = (float *)malloc(M * N * sizeof(float));
  float *expected = (float *)malloc(M * N * sizeof(float));

  for (int i = 0; i < M * K; i++)
    A[i] = (float)(i % 31) * 0.05f - 0.75f;
  for (int i = 0; i < K * N; i++)
    B[i] = (float)(i % 23) * 0.04f - 0.46f;

  gemm_f32(A, B, C, M, N, K, false, false);
  naive_matmul_f32(A, B, expected, M, N, K);

  float max_err = 0.0f;
  for (int i = 0; i < M * N; i++) {
    float err = fabsf(expected[i] - C[i]);
    if (err > max_err)
      max_err = err;
  }
  ASSERT_TRUE(max_err < 0.01f);

  free(A);
  free(B);
  free(C);
  free(expected);
  PASS();
}

TEST(gemm_f32_large_512x256) {
  const int M = 512, N = 256, K = 128;
  float *A = (float *)malloc(M * K * sizeof(float));
  float *B = (float *)malloc(K * N * sizeof(float));
  float *C = (float *)malloc(M * N * sizeof(float));
  float *expected = (float *)malloc(M * N * sizeof(float));

  for (int i = 0; i < M * K; i++)
    A[i] = (float)(i % 29) * 0.03f - 0.4f;
  for (int i = 0; i < K * N; i++)
    B[i] = (float)(i % 19) * 0.05f - 0.5f;

  gemm_f32(A, B, C, M, N, K, false, false);
  naive_matmul_f32(A, B, expected, M, N, K);

  float max_err = 0.0f;
  for (int i = 0; i < M * N; i++) {
    float err = fabsf(expected[i] - C[i]);
    if (err > max_err)
      max_err = err;
  }
  ASSERT_TRUE(max_err < 0.01f);

  free(A);
  free(B);
  free(C);
  free(expected);
  PASS();
}

extern "C" {
void run_gemm_tests(void) {
  TEST_SUITE("GEMM (FP32/FP16/BF16)");
  RUN_TEST(gemm_f32_identity_small);
  RUN_TEST(gemm_f32_simple_multiply);
  RUN_TEST(gemm_f32_with_hadamard_12x12);
  RUN_TEST(gemm_bf16_to_f32_conversion);
  RUN_TEST(gemm_f32_to_bf16_conversion);
  RUN_TEST(gemm_bf16_small_matrix);
  RUN_TEST(gemm_f16_small_matrix);
  RUN_TEST(gemm_f32_zero_dimensions);
  RUN_TEST(gemm_f32_m1_decode_path);
  RUN_TEST(gemm_f32_exact_16x16);
  RUN_TEST(gemm_f32_tail_m23_n19);
  RUN_TEST(gemm_f32_large_256x256);
  RUN_TEST(gemm_f32_large_512x256);
}
}

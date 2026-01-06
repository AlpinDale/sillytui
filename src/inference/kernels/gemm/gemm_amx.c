/*
 * AMX-accelerated GEMM for FP16 and BF16 on Apple Silicon
 *
 * FP16: Uses native AMX F16 instructions (AMX_FMA16) with F32 accumulation.
 * BF16: Converts to F32 on-the-fly and uses AMX F32 instructions.
 */

#include "inference/kernels/gemm/gemm_kernels.h"
#include <arm_neon.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) && defined(__aarch64__)

#include "inference/kernels/amx/aarch64.h"
#include <pthread.h>

/* AMX Operations */
#define FMA16_MATRIX_MODE 0
#define FMA16_Z_F32 (1ULL << 62)
#define FMA16_SKIP_Z (1ULL << 27)

#define FMA32_SKIP_Z (1ULL << 27)

#define AMX_PTR_ROW(ptr, row) (((uint64_t)(ptr)) | ((uint64_t)(row) << 56))

static inline uint64_t fma16_op(int z_row, int x_off, int y_off, int skip_z) {
  uint64_t op = FMA16_MATRIX_MODE | FMA16_Z_F32;
  op |= ((uint64_t)(z_row & 0x3F)) << 20;
  op |= ((uint64_t)(x_off & 0x1FF)) << 10;
  op |= ((uint64_t)(y_off & 0x1FF));
  if (skip_z)
    op |= FMA16_SKIP_Z;
  return op;
}

static inline uint64_t fma32_op(int z_row, int x_off, int y_off, int skip_z) {
  uint64_t op = 0; /* Matrix mode, F32 */
  op |= ((uint64_t)(z_row & 0x3F)) << 20;
  op |= ((uint64_t)(x_off & 0x1FF)) << 10;
  op |= ((uint64_t)(y_off & 0x1FF));
  if (skip_z)
    op |= FMA32_SKIP_Z;
  return op;
}

/* ---------------- FP16 Implementation ---------------- */

#define F16_TILE_M 32
#define F16_TILE_N 32
#define F16_K_BLOCK 32

/* Pack A (F16) -> Contiguous columns (for Y loading) */
static void pack_a_f16(const uint16_t *A, int lda, int M, int K,
                       uint16_t *packed) {
  for (int k = 0; k < K; k++) {
    for (int m = 0; m < M; m++) {
      packed[k * F16_TILE_M + m] = A[m * lda + k];
    }
    /* Pad M */
    for (int m = M; m < F16_TILE_M; m++) {
      packed[k * F16_TILE_M + m] = 0;
    }
  }
}

/* Pack B (F16) -> Contiguous rows (for X loading) */
static void pack_b_f16(const uint16_t *B, int ldb, int N, int K,
                       uint16_t *packed) {
  for (int k = 0; k < K; k++) {
    for (int n = 0; n < N; n++) {
      packed[k * F16_TILE_N + n] = B[k * ldb + n];
    }
    /* Pad N */
    for (int n = N; n < F16_TILE_N; n++) {
      packed[k * F16_TILE_N + n] = 0;
    }
  }
}

static void amx_f16_kernel(const uint16_t *pa, const uint16_t *pb, int K,
                           int first) {
  for (int k = 0; k < K; k++) {
    AMX_LDY(AMX_PTR_ROW(pa + k * 32, 0));
    AMX_LDX(AMX_PTR_ROW(pb + k * 32, 0));
    AMX_FMA16(fma16_op(0, 0, 0, (first && k == 0)));
  }
}

static void store_f16_tile(uint16_t *C, int ldc, int M, int N) {
  float z_row_even[16];
  float z_row_odd[16];
  float32x4_t v_even[4], v_odd[4];
  float16x8_t v_f16[4];

  for (int i = 0; i < M; i++) {
    /* Row i of C is stored in Z rows 2*i (even cols) and 2*i+1 (odd cols) */
    AMX_STZ(AMX_PTR_ROW(z_row_even, 2 * i));
    AMX_STZ(AMX_PTR_ROW(z_row_odd, 2 * i + 1));

    /* Load to NEON */
    for (int j = 0; j < 4; j++) {
      v_even[j] = vld1q_f32(&z_row_even[j * 4]);
      v_odd[j] = vld1q_f32(&z_row_odd[j * 4]);
    }

    /* Interleave to get sequential columns: C[i,0], C[i,1], C[i,2]... */
    for (int j = 0; j < 4; j++) {
      float32x4x2_t zipped = vzipq_f32(v_even[j], v_odd[j]);
      /* zipped.val[0] has 0,1, 4,5. zipped.val[1] has 2,3, 6,7. Wait. */
      /* vzipq:
         v_even: E0 E1 E2 E3
         v_odd:  O0 O1 O2 O3
         val[0]: E0 O0 E1 O1
         val[1]: E2 O2 E3 O3
         This matches the interleaved layout!
      */

      /* Convert to F16 */
      float16x4_t lo = vcvt_f16_f32(zipped.val[0]);
      float16x4_t hi = vcvt_f16_f32(zipped.val[1]);
      v_f16[j] = vcombine_f16(lo, hi);
    }

    /* Store to C */
    if (N == 32) {
      for (int j = 0; j < 4; j++) {
        vst1q_u16(C + i * ldc + j * 8, vreinterpretq_u16_f16(v_f16[j]));
      }
    } else {
      /* Partial store */
      uint16_t tmp[32];
      for (int j = 0; j < 4; j++) {
        vst1q_u16(tmp + j * 8, vreinterpretq_u16_f16(v_f16[j]));
      }
      for (int n = 0; n < N; n++) {
        C[i * ldc + n] = tmp[n];
      }
    }
  }
}

void gemm_f16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K) {
  /* Aligned buffers */
  uint16_t *pack_a = aligned_alloc(64, K * 32 * sizeof(uint16_t));
  uint16_t *pack_b = aligned_alloc(64, K * 32 * sizeof(uint16_t));
  if (!pack_a || !pack_b)
    return;

  AMX_SET();

  for (int m = 0; m < M; m += 32) {
    int m_len = (m + 32 > M) ? (M - m) : 32;
    pack_a_f16(A + m * K, K, m_len, K, pack_a);

    for (int n = 0; n < N; n += 32) {
      int n_len = (n + 32 > N) ? (N - n) : 32;
      pack_b_f16(B + n, N, n_len, K, pack_b);

      amx_f16_kernel(pack_a, pack_b, K, 1);
      store_f16_tile(C + m * N + n, N, m_len, n_len);
    }
  }

  AMX_CLR();
  free(pack_a);
  free(pack_b);
}

/* ---------------- BF16 Implementation ---------------- */

/* We use AMX F32 mode (16x16 tiles) */
#define BF16_TILE 16

/* Pack BF16 -> F32 (expand) */
static void pack_a_bf16_to_f32(const uint16_t *A, int lda, int M, int K,
                               float *packed) {
  for (int k = 0; k < K; k++) {
    for (int m = 0; m < M; m++) {
      uint32_t val = ((uint32_t)A[m * lda + k]) << 16;
      float f;
      memcpy(&f, &val, 4);
      packed[k * BF16_TILE + m] = f;
    }
    for (int m = M; m < BF16_TILE; m++)
      packed[k * BF16_TILE + m] = 0.0f;
  }
}

static void pack_b_bf16_to_f32(const uint16_t *B, int ldb, int N, int K,
                               float *packed) {
  for (int k = 0; k < K; k++) {
    for (int n = 0; n < N; n++) {
      uint32_t val = ((uint32_t)B[k * ldb + n]) << 16;
      float f;
      memcpy(&f, &val, 4);
      packed[k * BF16_TILE + n] = f;
    }
    for (int n = N; n < BF16_TILE; n++)
      packed[k * BF16_TILE + n] = 0.0f;
  }
}

static void amx_bf16_kernel(const float *pa, const float *pb, int K,
                            int first) {
  for (int k = 0; k < K; k++) {
    AMX_LDY(AMX_PTR_ROW(pa + k * 16, 0));
    AMX_LDX(AMX_PTR_ROW(pb + k * 16, 0));
    AMX_FMA32(fma32_op(0, 0, 0, (first && k == 0)));
  }
}

static void store_bf16_tile(uint16_t *C, int ldc, int M, int N) {
  float tile[16][16];
  for (int r = 0; r < 16; r++) {
    AMX_STZ(AMX_PTR_ROW(tile[r], r * 4));
  }

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      /* Convert F32 -> BF16 (truncation) */
      uint32_t bits;
      memcpy(&bits, &tile[i][j], 4);
      /* Rounding would be better, but truncation is fast */
      C[i * ldc + j] = (uint16_t)(bits >> 16);
    }
  }
}

void gemm_bf16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                          int M, int N, int K) {
  float *pack_a = aligned_alloc(64, K * 16 * sizeof(float));
  float *pack_b = aligned_alloc(64, K * 16 * sizeof(float));
  if (!pack_a || !pack_b)
    return;

  AMX_SET();

  for (int m = 0; m < M; m += 16) {
    int m_len = (m + 16 > M) ? (M - m) : 16;
    pack_a_bf16_to_f32(A + m * K, K, m_len, K, pack_a);

    for (int n = 0; n < N; n += 16) {
      int n_len = (n + 16 > N) ? (N - n) : 16;
      pack_b_bf16_to_f32(B + n, N, n_len, K, pack_b);

      amx_bf16_kernel(pack_a, pack_b, K, 1);
      store_bf16_tile(C + m * N + n, N, m_len, n_len);
    }
  }

  AMX_CLR();
  free(pack_a);
  free(pack_b);
}

/* Multi-threaded dispatchers */

typedef struct {
  const uint16_t *A, *B;
  uint16_t *C;
  int M, N, K, m_start, m_end;
} amx_mt_args;

static void *f16_mt_worker(void *ptr) {
  amx_mt_args *args = (amx_mt_args *)ptr;
  int K = args->K;
  int M = args->M; /* Full M for bounds check */
  int N = args->N;

  uint16_t *pack_a = aligned_alloc(64, K * 32 * sizeof(uint16_t));
  uint16_t *pack_b = aligned_alloc(64, K * 32 * sizeof(uint16_t));
  if (!pack_a || !pack_b)
    return NULL;

  AMX_SET();
  for (int m = args->m_start; m < args->m_end; m += 32) {
    int m_len = (m + 32 > M) ? (M - m) : 32;
    pack_a_f16(args->A + m * K, K, m_len, K, pack_a);
    for (int n = 0; n < N; n += 32) {
      int n_len = (n + 32 > N) ? (N - n) : 32;
      pack_b_f16(args->B + n, N, n_len, K, pack_b);
      amx_f16_kernel(pack_a, pack_b, K, 1);
      store_f16_tile(args->C + m * N + n, N, m_len, n_len);
    }
  }
  AMX_CLR();
  free(pack_a);
  free(pack_b);
  return NULL;
}

void gemm_f16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                            int M, int N, int K, int nt) {
  if (nt <= 1) {
    gemm_f16_kernel_amx(A, B, C, M, N, K);
    return;
  }

  pthread_t *threads = malloc(nt * sizeof(pthread_t));
  amx_mt_args *args = malloc(nt * sizeof(amx_mt_args));

  int rows_per = (M + nt - 1) / nt;
  /* Align to tile size 32 */
  rows_per = (rows_per + 31) & ~31;

  int active = 0;
  for (int i = 0; i < nt; i++) {
    int start = i * rows_per;
    if (start >= M)
      break;
    int end = start + rows_per;
    if (end > M)
      end = M;

    args[i] = (amx_mt_args){A, B, C, M, N, K, start, end};
    pthread_create(&threads[i], NULL, f16_mt_worker, &args[i]);
    active++;
  }
  for (int i = 0; i < active; i++)
    pthread_join(threads[i], NULL);
  free(threads);
  free(args);
}

static void *bf16_mt_worker(void *ptr) {
  amx_mt_args *args = (amx_mt_args *)ptr;
  int K = args->K;
  int M = args->M;
  int N = args->N;

  float *pack_a = aligned_alloc(64, K * 16 * sizeof(float));
  float *pack_b = aligned_alloc(64, K * 16 * sizeof(float));
  if (!pack_a || !pack_b)
    return NULL;

  AMX_SET();
  for (int m = args->m_start; m < args->m_end; m += 16) {
    int m_len = (m + 16 > M) ? (M - m) : 16;
    pack_a_bf16_to_f32(args->A + m * K, K, m_len, K, pack_a);
    for (int n = 0; n < N; n += 16) {
      int n_len = (n + 16 > N) ? (N - n) : 16;
      pack_b_bf16_to_f32(args->B + n, N, n_len, K, pack_b);
      amx_bf16_kernel(pack_a, pack_b, K, 1);
      store_bf16_tile(args->C + m * N + n, N, m_len, n_len);
    }
  }
  AMX_CLR();
  free(pack_a);
  free(pack_b);
  return NULL;
}

void gemm_bf16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                             int M, int N, int K, int nt) {
  if (nt <= 1) {
    gemm_bf16_kernel_amx(A, B, C, M, N, K);
    return;
  }

  pthread_t *threads = malloc(nt * sizeof(pthread_t));
  amx_mt_args *args = malloc(nt * sizeof(amx_mt_args));

  int rows_per = (M + nt - 1) / nt;
  rows_per = (rows_per + 15) & ~15; /* Align to 16 */

  int active = 0;
  for (int i = 0; i < nt; i++) {
    int start = i * rows_per;
    if (start >= M)
      break;
    int end = start + rows_per;
    if (end > M)
      end = M;

    args[i] = (amx_mt_args){A, B, C, M, N, K, start, end};
    pthread_create(&threads[i], NULL, bf16_mt_worker, &args[i]);
    active++;
  }
  for (int i = 0; i < active; i++)
    pthread_join(threads[i], NULL);
  free(threads);
  free(args);
}

#else
/* Stubs */
void gemm_f16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K) {}
void gemm_bf16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                          int M, int N, int K) {}
void gemm_f16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                            int M, int N, int K, int nt) {}
void gemm_bf16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                             int M, int N, int K, int nt) {}
#endif

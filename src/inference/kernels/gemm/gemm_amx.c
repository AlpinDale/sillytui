/*
 * AMX-accelerated GEMM for FP16 and BF16 on Apple Silicon
 *
 * FP16: Uses native AMX F16 instructions (AMX_FMA16) with F32 accumulation.
 *       Optimized with software pipelining and pre-packed B matrices.
 * BF16: Converts to F32 on-the-fly and uses AMX F32 instructions.
 */

#include "inference/kernels/gemm/gemm_kernels.h"
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) && defined(__aarch64__)

#include "inference/kernels/amx/aarch64.h"
#include <arm_neon.h>
#include <dispatch/dispatch.h>
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

/* ============================================================================
 * FP16 Implementation (Optimized)
 * ============================================================================
 */

#define F16_TILE_M 32
#define F16_TILE_N 32

/*
 * Pack A (F16) -> Contiguous columns (for Y loading)
 * Input: A is M×K row-major (row stride = lda)
 * Output: packed is K×32 column-major (transposed)
 *
 * Uses NEON for full tiles, scalar fallback for partial.
 */
static void pack_a_f16(const uint16_t *A, int lda, int M, int K,
                       uint16_t *packed) {
  if (M == 32) {
    /* Full tile - vectorized store */
    for (int k = 0; k < K; k++) {
      uint16_t col[32];
      for (int m = 0; m < 32; m++) {
        col[m] = A[m * lda + k];
      }
      vst1q_u16(packed + k * 32 + 0, vld1q_u16(col + 0));
      vst1q_u16(packed + k * 32 + 8, vld1q_u16(col + 8));
      vst1q_u16(packed + k * 32 + 16, vld1q_u16(col + 16));
      vst1q_u16(packed + k * 32 + 24, vld1q_u16(col + 24));
    }
  } else {
    /* Partial tile - scalar with zero padding */
    for (int k = 0; k < K; k++) {
      for (int m = 0; m < M; m++) {
        packed[k * 32 + m] = A[m * lda + k];
      }
      for (int m = M; m < 32; m++) {
        packed[k * 32 + m] = 0;
      }
    }
  }
}

/*
 * Pre-pack all B tiles for the entire matrix.
 * This is done once and reused for all M tiles.
 */
static void prepack_b_f16(const uint16_t *B, int ldb, int N, int K,
                          uint16_t *packed_B) {
  int n_tiles = (N + 31) / 32;
  for (int nt = 0; nt < n_tiles; nt++) {
    int n = nt * 32;
    int n_len = (n + 32 > N) ? (N - n) : 32;
    uint16_t *dst = packed_B + nt * K * 32;

    if (n_len == 32) {
      /* Full tile - vectorized copy */
      for (int k = 0; k < K; k++) {
        const uint16_t *src = B + k * ldb + n;
        uint16_t *d = dst + k * 32;
        vst1q_u16(d + 0, vld1q_u16(src + 0));
        vst1q_u16(d + 8, vld1q_u16(src + 8));
        vst1q_u16(d + 16, vld1q_u16(src + 16));
        vst1q_u16(d + 24, vld1q_u16(src + 24));
      }
    } else {
      /* Partial tile */
      for (int k = 0; k < K; k++) {
        for (int nn = 0; nn < n_len; nn++) {
          dst[k * 32 + nn] = B[k * ldb + n + nn];
        }
        for (int nn = n_len; nn < 32; nn++) {
          dst[k * 32 + nn] = 0;
        }
      }
    }
  }
}

/*
 * Software-pipelined AMX kernel with 4x unrolling.
 *
 * Pipeline structure:
 * - Prime: load k=0..3 into registers 0..3
 * - Main loop: FMA[k], load[k+4] interleaved
 * - Drain: FMA remaining
 *
 * This overlaps AMX loads (~4 cycles) with FMA execution.
 */
static void amx_f16_kernel_pipelined(const uint16_t *pa, const uint16_t *pb,
                                     int K, int first) {
  const int UNROLL = 4;

  if (K >= UNROLL) {
    /* Prime the pipeline */
    AMX_LDY(AMX_PTR_ROW(pa + 0 * 32, 0));
    AMX_LDX(AMX_PTR_ROW(pb + 0 * 32, 0));
    AMX_LDY(AMX_PTR_ROW(pa + 1 * 32, 1));
    AMX_LDX(AMX_PTR_ROW(pb + 1 * 32, 1));
    AMX_LDY(AMX_PTR_ROW(pa + 2 * 32, 2));
    AMX_LDX(AMX_PTR_ROW(pb + 2 * 32, 2));
    AMX_LDY(AMX_PTR_ROW(pa + 3 * 32, 3));
    AMX_LDX(AMX_PTR_ROW(pb + 3 * 32, 3));

    int k = 0;
    /* Main loop: FMA + prefetch interleaved */
    for (; k <= K - 2 * UNROLL; k += UNROLL) {
      AMX_FMA16(fma16_op(0, 0, 0, (first && k == 0)));
      AMX_LDY(AMX_PTR_ROW(pa + (k + UNROLL + 0) * 32, 0));
      AMX_LDX(AMX_PTR_ROW(pb + (k + UNROLL + 0) * 32, 0));

      AMX_FMA16(fma16_op(0, 64, 64, 0));
      AMX_LDY(AMX_PTR_ROW(pa + (k + UNROLL + 1) * 32, 1));
      AMX_LDX(AMX_PTR_ROW(pb + (k + UNROLL + 1) * 32, 1));

      AMX_FMA16(fma16_op(0, 128, 128, 0));
      AMX_LDY(AMX_PTR_ROW(pa + (k + UNROLL + 2) * 32, 2));
      AMX_LDX(AMX_PTR_ROW(pb + (k + UNROLL + 2) * 32, 2));

      AMX_FMA16(fma16_op(0, 192, 192, 0));
      AMX_LDY(AMX_PTR_ROW(pa + (k + UNROLL + 3) * 32, 3));
      AMX_LDX(AMX_PTR_ROW(pb + (k + UNROLL + 3) * 32, 3));
    }

    /* Drain: process remaining loaded data */
    AMX_FMA16(fma16_op(0, 0, 0, (first && k == 0)));
    AMX_FMA16(fma16_op(0, 64, 64, 0));
    AMX_FMA16(fma16_op(0, 128, 128, 0));
    AMX_FMA16(fma16_op(0, 192, 192, 0));
    k += UNROLL;

    /* Handle remainder (K mod UNROLL) */
    for (; k < K; k++) {
      AMX_LDY(AMX_PTR_ROW(pa + k * 32, 0));
      AMX_LDX(AMX_PTR_ROW(pb + k * 32, 0));
      AMX_FMA16(fma16_op(0, 0, 0, 0));
    }
  } else {
    /* Small K: simple loop */
    for (int k = 0; k < K; k++) {
      AMX_LDY(AMX_PTR_ROW(pa + k * 32, 0));
      AMX_LDX(AMX_PTR_ROW(pb + k * 32, 0));
      AMX_FMA16(fma16_op(0, 0, 0, (first && k == 0)));
    }
  }
}

/*
 * Store F16 tile from AMX Z registers.
 * Z stores F32 results interleaved (even/odd columns in separate rows).
 */
static void store_f16_tile(uint16_t *C, int ldc, int M, int N) {
  float z_row_even[16] __attribute__((aligned(64)));
  float z_row_odd[16] __attribute__((aligned(64)));
  float32x4_t v_even[4], v_odd[4];
  float16x8_t v_f16[4];

  for (int i = 0; i < M; i++) {
    AMX_STZ(AMX_PTR_ROW(z_row_even, 2 * i));
    AMX_STZ(AMX_PTR_ROW(z_row_odd, 2 * i + 1));

    v_even[0] = vld1q_f32(&z_row_even[0]);
    v_even[1] = vld1q_f32(&z_row_even[4]);
    v_even[2] = vld1q_f32(&z_row_even[8]);
    v_even[3] = vld1q_f32(&z_row_even[12]);
    v_odd[0] = vld1q_f32(&z_row_odd[0]);
    v_odd[1] = vld1q_f32(&z_row_odd[4]);
    v_odd[2] = vld1q_f32(&z_row_odd[8]);
    v_odd[3] = vld1q_f32(&z_row_odd[12]);

    /* Interleave and convert to F16 */
    for (int j = 0; j < 4; j++) {
      float32x4x2_t zipped = vzipq_f32(v_even[j], v_odd[j]);
      float16x4_t lo = vcvt_f16_f32(zipped.val[0]);
      float16x4_t hi = vcvt_f16_f32(zipped.val[1]);
      v_f16[j] = vcombine_f16(lo, hi);
    }

    if (N >= 32) {
      vst1q_u16(C + i * ldc + 0, vreinterpretq_u16_f16(v_f16[0]));
      vst1q_u16(C + i * ldc + 8, vreinterpretq_u16_f16(v_f16[1]));
      vst1q_u16(C + i * ldc + 16, vreinterpretq_u16_f16(v_f16[2]));
      vst1q_u16(C + i * ldc + 24, vreinterpretq_u16_f16(v_f16[3]));
    } else {
      uint16_t tmp[32] __attribute__((aligned(64)));
      vst1q_u16(tmp + 0, vreinterpretq_u16_f16(v_f16[0]));
      vst1q_u16(tmp + 8, vreinterpretq_u16_f16(v_f16[1]));
      vst1q_u16(tmp + 16, vreinterpretq_u16_f16(v_f16[2]));
      vst1q_u16(tmp + 24, vreinterpretq_u16_f16(v_f16[3]));
      for (int n = 0; n < N; n++) {
        C[i * ldc + n] = tmp[n];
      }
    }
  }
}

void gemm_f16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K) {
  int n_tiles = (N + 31) / 32;

  uint16_t *pack_a = aligned_alloc(128, K * 32 * sizeof(uint16_t));
  uint16_t *packed_B = aligned_alloc(128, n_tiles * K * 32 * sizeof(uint16_t));
  if (!pack_a || !packed_B) {
    free(pack_a);
    free(packed_B);
    return;
  }

  /* Pre-pack all B tiles once */
  prepack_b_f16(B, N, N, K, packed_B);

  AMX_SET();

  for (int m = 0; m < M; m += 32) {
    int m_len = (m + 32 > M) ? (M - m) : 32;
    pack_a_f16(A + m * K, K, m_len, K, pack_a);

    for (int nt = 0; nt < n_tiles; nt++) {
      int n = nt * 32;
      int n_len = (n + 32 > N) ? (N - n) : 32;
      uint16_t *pack_b = packed_B + nt * K * 32;

      amx_f16_kernel_pipelined(pack_a, pack_b, K, 1);
      store_f16_tile(C + m * N + n, N, m_len, n_len);
    }
  }

  AMX_CLR();
  free(pack_a);
  free(packed_B);
}

/* ============================================================================
 * BF16 Implementation
 * ============================================================================
 */

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

/* ============================================================================
 * Multi-threaded Dispatchers (GCD-based)
 * ============================================================================
 */

typedef struct {
  const uint16_t *A;
  const uint16_t *packed_B;
  uint16_t *C;
  int M, N, K;
  int n_tiles;
  int m_tiles_per_job;
  int total_m_tiles;
} gcd_gemm_context_t;

/* Thread-local storage for pack_a buffers */
static __thread uint16_t *tls_pack_a = NULL;
static __thread size_t tls_pack_a_size = 0;

static uint16_t *get_tls_pack_a(size_t size) {
  if (tls_pack_a == NULL || tls_pack_a_size < size) {
    free(tls_pack_a);
    tls_pack_a = aligned_alloc(128, size);
    tls_pack_a_size = size;
  }
  return tls_pack_a;
}

static void gcd_f16_worker(void *context, size_t job_idx) {
  gcd_gemm_context_t *ctx = (gcd_gemm_context_t *)context;
  const uint16_t *A = ctx->A;
  const uint16_t *packed_B = ctx->packed_B;
  uint16_t *C = ctx->C;
  int M = ctx->M, N = ctx->N, K = ctx->K;
  int n_tiles = ctx->n_tiles;
  int m_tiles_per_job = ctx->m_tiles_per_job;
  int total_m_tiles = ctx->total_m_tiles;

  int m_tile_start = (int)job_idx * m_tiles_per_job;
  int m_tile_end = m_tile_start + m_tiles_per_job;
  if (m_tile_end > total_m_tiles)
    m_tile_end = total_m_tiles;

  uint16_t *pack_a = get_tls_pack_a(K * 32 * sizeof(uint16_t));
  if (!pack_a)
    return;

  AMX_SET();

  for (int mt = m_tile_start; mt < m_tile_end; mt++) {
    int m = mt * 32;
    int m_len = (m + 32 > M) ? (M - m) : 32;

    pack_a_f16(A + m * K, K, m_len, K, pack_a);

    for (int nt = 0; nt < n_tiles; nt++) {
      int n = nt * 32;
      int n_len = (n + 32 > N) ? (N - n) : 32;
      const uint16_t *pack_b = packed_B + nt * K * 32;

      amx_f16_kernel_pipelined(pack_a, pack_b, K, 1);
      store_f16_tile(C + m * N + n, N, m_len, n_len);
    }
  }

  AMX_CLR();
}

void gemm_f16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                            int M, int N, int K, int nt) {
  (void)nt; /* Thread count hint ignored - GCD handles this */

  int m_tiles = (M + 31) / 32;
  int n_tiles = (N + 31) / 32;

  /* Use single-threaded for small problems */
  long long total_flops = 2LL * M * N * K;
  if (total_flops < 100000000LL || m_tiles < 4) {
    gemm_f16_kernel_amx(A, B, C, M, N, K);
    return;
  }

  /* Adaptive job sizing based on problem size */
  int m_tiles_per_job;
  if (m_tiles >= 32) {
    m_tiles_per_job = 1; /* Fine-grained for large problems */
  } else if (m_tiles >= 16) {
    m_tiles_per_job = 2;
  } else {
    m_tiles_per_job = (m_tiles + 7) / 8;
    if (m_tiles_per_job < 1)
      m_tiles_per_job = 1;
  }
  int num_jobs = (m_tiles + m_tiles_per_job - 1) / m_tiles_per_job;

  /* Pre-pack all B tiles once (shared among workers) */
  uint16_t *packed_B = aligned_alloc(128, n_tiles * K * 32 * sizeof(uint16_t));
  if (!packed_B)
    return;
  prepack_b_f16(B, N, N, K, packed_B);

  gcd_gemm_context_t ctx = {.A = A,
                            .packed_B = packed_B,
                            .C = C,
                            .M = M,
                            .N = N,
                            .K = K,
                            .n_tiles = n_tiles,
                            .m_tiles_per_job = m_tiles_per_job,
                            .total_m_tiles = m_tiles};

  dispatch_apply((size_t)num_jobs,
                 dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
                 ^(size_t idx) {
                   gcd_f16_worker((void *)&ctx, idx);
                 });

  free(packed_B);
}

/* BF16 multi-threaded (pthread-based, unchanged) */

typedef struct {
  const uint16_t *A, *B;
  uint16_t *C;
  int M, N, K, m_start, m_end;
} amx_mt_args;

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
/* Stubs for non-Apple platforms */
void gemm_f16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                         int M, int N, int K) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
}
void gemm_bf16_kernel_amx(const uint16_t *A, const uint16_t *B, uint16_t *C,
                          int M, int N, int K) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
}
void gemm_f16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                            int M, int N, int K, int nt) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
  (void)nt;
}
void gemm_bf16_kernel_amx_mt(const uint16_t *A, const uint16_t *B, uint16_t *C,
                             int M, int N, int K, int nt) {
  (void)A;
  (void)B;
  (void)C;
  (void)M;
  (void)N;
  (void)K;
  (void)nt;
}
#endif

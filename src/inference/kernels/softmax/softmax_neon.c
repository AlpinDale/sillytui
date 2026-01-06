/*
 * Softmax - NEON Optimized + Multi-threaded Implementations with Thread Pool
 */

#include "inference/kernels/softmax/softmax_kernels.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#define HAS_ACCELERATE 1
#else
#define HAS_ACCELERATE 0
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#else
#define HAS_NEON 0
#endif

#define MAX_THREADS 64

typedef void (*work_fn_t)(void *arg, int start, int end);

typedef struct {
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  pthread_cond_t done_cond;
  work_fn_t fn;
  void *arg;
  int start;
  int end;
  atomic_int busy;
  atomic_int shutdown;
} worker_t;

typedef struct {
  worker_t *workers;
  int num_workers;
  int initialized;
  pthread_mutex_t init_mutex;
} thread_pool_t;

static thread_pool_t g_pool = {NULL, 0, 0, PTHREAD_MUTEX_INITIALIZER};
static int g_num_threads = 0;

static int get_cpu_count(void) {
  int count = (int)sysconf(_SC_NPROCESSORS_ONLN);
  return count > 0 ? count : 1;
}

static void *worker_thread(void *arg) {
  worker_t *w = (worker_t *)arg;
  while (1) {
    pthread_mutex_lock(&w->mutex);
    while (!atomic_load(&w->busy) && !atomic_load(&w->shutdown)) {
      pthread_cond_wait(&w->cond, &w->mutex);
    }
    pthread_mutex_unlock(&w->mutex);

    if (atomic_load(&w->shutdown))
      break;

    if (w->fn) {
      w->fn(w->arg, w->start, w->end);
    }

    atomic_store(&w->busy, 0);
    pthread_mutex_lock(&w->mutex);
    pthread_cond_signal(&w->done_cond);
    pthread_mutex_unlock(&w->mutex);
  }
  return NULL;
}

static void init_thread_pool(int num_threads) {
  pthread_mutex_lock(&g_pool.init_mutex);
  if (g_pool.initialized) {
    pthread_mutex_unlock(&g_pool.init_mutex);
    return;
  }

  if (num_threads > MAX_THREADS)
    num_threads = MAX_THREADS;

  g_pool.workers = (worker_t *)calloc(num_threads, sizeof(worker_t));
  g_pool.num_workers = num_threads;

  for (int i = 0; i < num_threads; i++) {
    worker_t *w = &g_pool.workers[i];
    pthread_mutex_init(&w->mutex, NULL);
    pthread_cond_init(&w->cond, NULL);
    pthread_cond_init(&w->done_cond, NULL);
    atomic_store(&w->busy, 0);
    atomic_store(&w->shutdown, 0);
    pthread_create(&w->thread, NULL, worker_thread, w);
  }

  g_pool.initialized = 1;
  pthread_mutex_unlock(&g_pool.init_mutex);
}

static void parallel_for(int start, int end, work_fn_t fn, void *arg) {
  int total = end - start;
  if (total <= 0)
    return;

  int num_threads = g_num_threads > 0 ? g_num_threads : get_cpu_count();

  if (total < 8 || num_threads <= 1) {
    fn(arg, start, end);
    return;
  }

  if (!g_pool.initialized) {
    init_thread_pool(num_threads);
  }

  int workers_to_use = num_threads - 1;
  if (workers_to_use > g_pool.num_workers)
    workers_to_use = g_pool.num_workers;
  if (workers_to_use > total - 1)
    workers_to_use = total - 1;

  int chunk_size = (total + workers_to_use) / (workers_to_use + 1);
  int current = start;

  for (int i = 0; i < workers_to_use && current < end; i++) {
    worker_t *w = &g_pool.workers[i];
    int worker_end = current + chunk_size;
    if (worker_end > end)
      worker_end = end;

    pthread_mutex_lock(&w->mutex);
    w->fn = fn;
    w->arg = arg;
    w->start = current;
    w->end = worker_end;
    atomic_store(&w->busy, 1);
    pthread_cond_signal(&w->cond);
    pthread_mutex_unlock(&w->mutex);

    current = worker_end;
  }

  if (current < end) {
    fn(arg, current, end);
  }

  for (int i = 0; i < workers_to_use; i++) {
    worker_t *w = &g_pool.workers[i];
    pthread_mutex_lock(&w->mutex);
    while (atomic_load(&w->busy)) {
      pthread_cond_wait(&w->done_cond, &w->mutex);
    }
    pthread_mutex_unlock(&w->mutex);
  }
}

void softmax_set_num_threads(int num_threads) {
  if (num_threads <= 0) {
    g_num_threads = get_cpu_count();
  } else {
    g_num_threads = num_threads;
  }
}

int softmax_get_num_threads(void) {
  if (g_num_threads == 0) {
    g_num_threads = get_cpu_count();
  }
  return g_num_threads;
}

softmax_caps_t softmax_get_capabilities(void) {
  softmax_caps_t caps = {0};
#if HAS_NEON
  caps.has_neon = true;
#endif
  return caps;
}

#if HAS_NEON

static inline float bf16_to_float(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_bf16(float f) {
  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));
  return (uint16_t)(bits >> 16);
}

static inline float fp16_to_float(uint16_t fp16) {
  float16x4_t v = vreinterpret_f16_u16(vdup_n_u16(fp16));
  float32x4_t f32 = vcvt_f32_f16(v);
  return vgetq_lane_f32(f32, 0);
}

static inline uint16_t float_to_fp16(float f) {
  float32x4_t v = vdupq_n_f32(f);
  float16x4_t f16 = vcvt_f16_f32(v);
  return vget_lane_u16(vreinterpret_u16_f16(f16), 0);
}

static inline float32x4_t exp_f32x4(float32x4_t x) {
  float vals[4];
  vst1q_f32(vals, x);
  vals[0] = expf(vals[0]);
  vals[1] = expf(vals[1]);
  vals[2] = expf(vals[2]);
  vals[3] = expf(vals[3]);
  return vld1q_f32(vals);
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

static void softmax_f32_row(float *output, const float *input, int row_size,
                            float scale) {
#if HAS_ACCELERATE
  float max_val;
  if (scale == 1.0f) {
    vDSP_maxv(input, 1, &max_val, row_size);

    float neg_max = -max_val;
    vDSP_vsadd(input, 1, &neg_max, output, 1, row_size);

    int n = row_size;
    vvexpf(output, output, &n);

    float sum;
    vDSP_sve(output, 1, &sum, row_size);

    float inv_sum = 1.0f / sum;
    vDSP_vsmul(output, 1, &inv_sum, output, 1, row_size);
  } else {
    vDSP_vsmul(input, 1, &scale, output, 1, row_size);

    vDSP_maxv(output, 1, &max_val, row_size);

    float neg_max = -max_val;
    vDSP_vsadd(output, 1, &neg_max, output, 1, row_size);

    int n = row_size;
    vvexpf(output, output, &n);

    float sum;
    vDSP_sve(output, 1, &sum, row_size);

    float inv_sum = 1.0f / sum;
    vDSP_vsmul(output, 1, &inv_sum, output, 1, row_size);
  }
#else
  const float32x4_t scale_v = vdupq_n_f32(scale);

  float32x4_t max_v = vdupq_n_f32(-__builtin_inff());
  int i = 0;
  for (; i + 4 <= row_size; i += 4) {
    float32x4_t v = vld1q_f32(input + i);
    v = vmulq_f32(v, scale_v);
    max_v = vmaxq_f32(max_v, v);
  }

  float max_val = hmax_f32x4(max_v);
  for (; i < row_size; i++) {
    float val = input[i] * scale;
    if (val > max_val)
      max_val = val;
  }

  float32x4_t max_broadcast = vdupq_n_f32(max_val);
  float32x4_t sum_v = vdupq_n_f32(0.0f);

  i = 0;
  for (; i + 4 <= row_size; i += 4) {
    float32x4_t v = vld1q_f32(input + i);
    v = vmulq_f32(v, scale_v);
    v = vsubq_f32(v, max_broadcast);
    v = exp_f32x4(v);
    vst1q_f32(output + i, v);
    sum_v = vaddq_f32(sum_v, v);
  }

  float sum = hsum_f32x4(sum_v);
  for (; i < row_size; i++) {
    float val = expf(input[i] * scale - max_val);
    output[i] = val;
    sum += val;
  }

  float32x4_t inv_sum_v = vdupq_n_f32(1.0f / sum);

  i = 0;
  for (; i + 4 <= row_size; i += 4) {
    float32x4_t v = vld1q_f32(output + i);
    v = vmulq_f32(v, inv_sum_v);
    vst1q_f32(output + i, v);
  }

  float inv_sum = 1.0f / sum;
  for (; i < row_size; i++) {
    output[i] *= inv_sum;
  }
#endif
}

typedef struct {
  float *output;
  const float *input;
  int row_size;
  float scale;
} softmax_f32_ctx_t;

static void softmax_f32_work(void *arg, int start, int end) {
  softmax_f32_ctx_t *ctx = (softmax_f32_ctx_t *)arg;
  for (int row = start; row < end; row++) {
    const float *in_row = ctx->input + row * ctx->row_size;
    float *out_row = ctx->output + row * ctx->row_size;
    softmax_f32_row(out_row, in_row, ctx->row_size, ctx->scale);
  }
}

void softmax_f32_kernel(float *output, const float *input, int num_rows,
                        int row_size, float scale) {
  if (num_rows <= 4) {
    for (int row = 0; row < num_rows; row++) {
      softmax_f32_row(output + row * row_size, input + row * row_size, row_size,
                      scale);
    }
    return;
  }
  softmax_f32_ctx_t ctx = {output, input, row_size, scale};
  parallel_for(0, num_rows, softmax_f32_work, &ctx);
}

static void softmax_bf16_row(uint16_t *output, const uint16_t *input,
                             int row_size, float scale) {
  float max_val = -__builtin_inff();
  for (int i = 0; i < row_size; i++) {
    float val = bf16_to_float(input[i]) * scale;
    if (val > max_val)
      max_val = val;
  }

  const float32x4_t scale_v = vdupq_n_f32(scale);
  float32x4_t max_broadcast = vdupq_n_f32(max_val);
  float32x4_t sum_v = vdupq_n_f32(0.0f);

  int i = 0;
  for (; i + 4 <= row_size; i += 4) {
    float32x4_t v = vdupq_n_f32(0.0f);
    v = vsetq_lane_f32(bf16_to_float(input[i + 0]), v, 0);
    v = vsetq_lane_f32(bf16_to_float(input[i + 1]), v, 1);
    v = vsetq_lane_f32(bf16_to_float(input[i + 2]), v, 2);
    v = vsetq_lane_f32(bf16_to_float(input[i + 3]), v, 3);

    v = vmulq_f32(v, scale_v);
    v = vsubq_f32(v, max_broadcast);
    v = exp_f32x4(v);
    sum_v = vaddq_f32(sum_v, v);

    output[i + 0] = float_to_bf16(vgetq_lane_f32(v, 0));
    output[i + 1] = float_to_bf16(vgetq_lane_f32(v, 1));
    output[i + 2] = float_to_bf16(vgetq_lane_f32(v, 2));
    output[i + 3] = float_to_bf16(vgetq_lane_f32(v, 3));
  }

  float sum = hsum_f32x4(sum_v);
  for (; i < row_size; i++) {
    float val = expf(bf16_to_float(input[i]) * scale - max_val);
    output[i] = float_to_bf16(val);
    sum += val;
  }

  float inv_sum = 1.0f / sum;
  for (i = 0; i < row_size; i++) {
    float val = bf16_to_float(output[i]) * inv_sum;
    output[i] = float_to_bf16(val);
  }
}

typedef struct {
  uint16_t *output;
  const uint16_t *input;
  int row_size;
  float scale;
} softmax_bf16_ctx_t;

static void softmax_bf16_work(void *arg, int start, int end) {
  softmax_bf16_ctx_t *ctx = (softmax_bf16_ctx_t *)arg;
  for (int row = start; row < end; row++) {
    const uint16_t *in_row = ctx->input + row * ctx->row_size;
    uint16_t *out_row = ctx->output + row * ctx->row_size;
    softmax_bf16_row(out_row, in_row, ctx->row_size, ctx->scale);
  }
}

void softmax_bf16_kernel(uint16_t *output, const uint16_t *input, int num_rows,
                         int row_size, float scale) {
  softmax_bf16_ctx_t ctx = {output, input, row_size, scale};
  parallel_for(0, num_rows, softmax_bf16_work, &ctx);
}

static void softmax_f16_row(uint16_t *output, const uint16_t *input,
                            int row_size, float scale) {
  const float32x4_t scale_v = vdupq_n_f32(scale);

  float32x4_t max_v = vdupq_n_f32(-__builtin_inff());
  int i = 0;
  for (; i + 4 <= row_size; i += 4) {
    float16x4_t f16 = vreinterpret_f16_u16(vld1_u16(input + i));
    float32x4_t v = vcvt_f32_f16(f16);
    v = vmulq_f32(v, scale_v);
    max_v = vmaxq_f32(max_v, v);
  }

  float max_val = hmax_f32x4(max_v);
  for (; i < row_size; i++) {
    float val = fp16_to_float(input[i]) * scale;
    if (val > max_val)
      max_val = val;
  }

  float32x4_t max_broadcast = vdupq_n_f32(max_val);
  float32x4_t sum_v = vdupq_n_f32(0.0f);

  i = 0;
  for (; i + 4 <= row_size; i += 4) {
    float16x4_t f16 = vreinterpret_f16_u16(vld1_u16(input + i));
    float32x4_t v = vcvt_f32_f16(f16);
    v = vmulq_f32(v, scale_v);
    v = vsubq_f32(v, max_broadcast);
    v = exp_f32x4(v);
    sum_v = vaddq_f32(sum_v, v);

    float16x4_t out_f16 = vcvt_f16_f32(v);
    vst1_u16(output + i, vreinterpret_u16_f16(out_f16));
  }

  float sum = hsum_f32x4(sum_v);
  for (; i < row_size; i++) {
    float val = expf(fp16_to_float(input[i]) * scale - max_val);
    output[i] = float_to_fp16(val);
    sum += val;
  }

  float32x4_t inv_sum_v = vdupq_n_f32(1.0f / sum);

  i = 0;
  for (; i + 4 <= row_size; i += 4) {
    float16x4_t f16 = vreinterpret_f16_u16(vld1_u16(output + i));
    float32x4_t v = vcvt_f32_f16(f16);
    v = vmulq_f32(v, inv_sum_v);
    float16x4_t out_f16 = vcvt_f16_f32(v);
    vst1_u16(output + i, vreinterpret_u16_f16(out_f16));
  }

  float inv_sum = 1.0f / sum;
  for (; i < row_size; i++) {
    float val = fp16_to_float(output[i]) * inv_sum;
    output[i] = float_to_fp16(val);
  }
}

typedef struct {
  uint16_t *output;
  const uint16_t *input;
  int row_size;
  float scale;
} softmax_f16_ctx_t;

static void softmax_f16_work(void *arg, int start, int end) {
  softmax_f16_ctx_t *ctx = (softmax_f16_ctx_t *)arg;
  for (int row = start; row < end; row++) {
    const uint16_t *in_row = ctx->input + row * ctx->row_size;
    uint16_t *out_row = ctx->output + row * ctx->row_size;
    softmax_f16_row(out_row, in_row, ctx->row_size, ctx->scale);
  }
}

void softmax_f16_kernel(uint16_t *output, const uint16_t *input, int num_rows,
                        int row_size, float scale) {
  softmax_f16_ctx_t ctx = {output, input, row_size, scale};
  parallel_for(0, num_rows, softmax_f16_work, &ctx);
}

#else

void softmax_f32_kernel(float *output, const float *input, int num_rows,
                        int row_size, float scale) {
  (void)output;
  (void)input;
  (void)num_rows;
  (void)row_size;
  (void)scale;
}

void softmax_bf16_kernel(uint16_t *output, const uint16_t *input, int num_rows,
                         int row_size, float scale) {
  (void)output;
  (void)input;
  (void)num_rows;
  (void)row_size;
  (void)scale;
}

void softmax_f16_kernel(uint16_t *output, const uint16_t *input, int num_rows,
                        int row_size, float scale) {
  (void)output;
  (void)input;
  (void)num_rows;
  (void)row_size;
  (void)scale;
}

#endif

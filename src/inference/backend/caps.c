/**
 * @file caps.c
 * @brief Implementation of hardware capability detection.
 */

#include "inference/backend/caps.h"
#include <stdatomic.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

/* Static storage for capabilities */
static system_caps_t g_caps;
static atomic_int g_initialized = 0;
static int g_user_num_threads = 0;

/* Capability names */
static const char *cap_names[CAP_COUNT] = {
    [CAP_SCALAR] = "scalar", [CAP_NEON] = "neon",
    [CAP_AMX] = "amx",       [CAP_AVX2] = "avx2",
    [CAP_AVX512] = "avx512", [CAP_ACCELERATE] = "accelerate",
    [CAP_METAL] = "metal",   [CAP_CUDA] = "cuda",
};

/* Default priorities (higher = preferred) */
static const int default_priorities[CAP_COUNT] = {
    [CAP_SCALAR] = 0, [CAP_NEON] = 50,   [CAP_AMX] = 80,
    [CAP_AVX2] = 50,  [CAP_AVX512] = 70, [CAP_ACCELERATE] = 100,
    [CAP_METAL] = 90, [CAP_CUDA] = 90,
};

/* ============================================================================
 * Platform-specific detection
 * ============================================================================
 */

static int get_cpu_count(void) {
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#else
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  return n > 0 ? (int)n : 1;
#endif
}

#if defined(__APPLE__)
static size_t get_cache_size(const char *name) {
  size_t size = 0;
  size_t len = sizeof(size);
  if (sysctlbyname(name, &size, &len, NULL, 0) == 0) {
    return size;
  }
  return 0;
}
#endif

static void detect_capabilities(system_caps_t *caps) {
  memset(caps, 0, sizeof(*caps));

  /* Scalar is always available */
  caps->available[CAP_SCALAR] = true;
  caps->priority[CAP_SCALAR] = default_priorities[CAP_SCALAR];

  /* Detect CPU count */
  caps->num_cpus = get_cpu_count();
  caps->num_threads = caps->num_cpus;

  /* ARM NEON detection */
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  caps->available[CAP_NEON] = true;
  caps->priority[CAP_NEON] = default_priorities[CAP_NEON];
#endif

  /* Apple AMX detection (available on Apple Silicon) */
#if defined(__APPLE__) && defined(__aarch64__)
  caps->available[CAP_AMX] = true;
  caps->priority[CAP_AMX] = default_priorities[CAP_AMX];
#endif

  /* Apple Accelerate framework detection */
#if defined(__APPLE__)
  caps->available[CAP_ACCELERATE] = true;
  caps->priority[CAP_ACCELERATE] = default_priorities[CAP_ACCELERATE];

  /* Get cache sizes on Apple platforms */
  caps->l1_cache_size = get_cache_size("hw.l1dcachesize");
  caps->l2_cache_size = get_cache_size("hw.l2cachesize");
#endif

  /* x86 SIMD detection would go here */
#if defined(__x86_64__) || defined(_M_X64)
  /* TODO: Proper CPUID-based detection for AVX2/AVX512 */
  /* For now, we can use compiler-defined macros */
#ifdef __AVX2__
  caps->available[CAP_AVX2] = true;
  caps->priority[CAP_AVX2] = default_priorities[CAP_AVX2];
#endif

#ifdef __AVX512F__
  caps->available[CAP_AVX512] = true;
  caps->priority[CAP_AVX512] = default_priorities[CAP_AVX512];
#endif
#endif

  /* Metal is potentially available on Apple platforms but requires runtime
   * check */
#if defined(__APPLE__)
  /* Metal availability would require Objective-C runtime check */
  /* For now, mark as unavailable until Metal backend is implemented */
  caps->available[CAP_METAL] = false;
#endif

  /* CUDA would require runtime check */
  caps->available[CAP_CUDA] = false;
}

/* ============================================================================
 * Public API
 * ============================================================================
 */

void caps_init(void) {
  /* Double-checked locking pattern */
  if (atomic_load(&g_initialized))
    return;

  detect_capabilities(&g_caps);
  atomic_store(&g_initialized, 1);
}

const system_caps_t *caps_get(void) {
  if (!atomic_load(&g_initialized)) {
    caps_init();
  }
  return &g_caps;
}

bool caps_has(cap_t cap) {
  if (cap < 0 || cap >= CAP_COUNT)
    return false;

  const system_caps_t *caps = caps_get();
  return caps->available[cap];
}

const char *caps_name(cap_t cap) {
  if (cap < 0 || cap >= CAP_COUNT)
    return "unknown";
  return cap_names[cap];
}

int caps_num_threads(void) {
  const system_caps_t *caps = caps_get();

  if (g_user_num_threads > 0) {
    return g_user_num_threads;
  }
  return caps->num_threads;
}

void caps_set_num_threads(int num_threads) {
  const system_caps_t *caps = caps_get();

  if (num_threads <= 0) {
    g_user_num_threads = caps->num_cpus;
  } else {
    g_user_num_threads = num_threads;
  }
}

cap_t caps_best_for_gemm(void) {
  const system_caps_t *caps = caps_get();

  /* Priority for GEMM: Accelerate > AMX > NEON > AVX2 > Scalar */
  if (caps->available[CAP_ACCELERATE])
    return CAP_ACCELERATE;
  if (caps->available[CAP_AMX])
    return CAP_AMX;
  if (caps->available[CAP_NEON])
    return CAP_NEON;
  if (caps->available[CAP_AVX512])
    return CAP_AVX512;
  if (caps->available[CAP_AVX2])
    return CAP_AVX2;

  return CAP_SCALAR;
}

cap_t caps_best_for_activation(void) {
  const system_caps_t *caps = caps_get();

  /* Priority for activations: NEON > AVX2 > Scalar */
  if (caps->available[CAP_NEON])
    return CAP_NEON;
  if (caps->available[CAP_AVX512])
    return CAP_AVX512;
  if (caps->available[CAP_AVX2])
    return CAP_AVX2;

  return CAP_SCALAR;
}

cap_t caps_best_for_attention(void) {
  const system_caps_t *caps = caps_get();

  /* Priority for attention: NEON > AVX2 > Scalar */
  if (caps->available[CAP_NEON])
    return CAP_NEON;
  if (caps->available[CAP_AVX512])
    return CAP_AVX512;
  if (caps->available[CAP_AVX2])
    return CAP_AVX2;

  return CAP_SCALAR;
}

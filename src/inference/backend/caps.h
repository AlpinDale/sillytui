/**
 * @file caps.h
 * @brief Unified hardware capability detection for inference backends.
 *
 * This consolidates the capability detection that was previously
 * duplicated across gemm_kernels.h, activation_kernels.h, etc.
 */

#ifndef INFERENCE_BACKEND_CAPS_H
#define INFERENCE_BACKEND_CAPS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hardware capabilities/features that may be available.
 */
typedef enum {
  CAP_SCALAR = 0,     /**< Scalar fallback (always available) */
  CAP_NEON = 1,       /**< ARM NEON SIMD */
  CAP_AMX = 2,        /**< Apple AMX (matrix coprocessor) */
  CAP_AVX2 = 3,       /**< x86 AVX2 SIMD */
  CAP_AVX512 = 4,     /**< x86 AVX-512 SIMD */
  CAP_ACCELERATE = 5, /**< Apple Accelerate framework */
  CAP_METAL = 6,      /**< Apple Metal GPU (future) */
  CAP_CUDA = 7,       /**< NVIDIA CUDA (future) */
  CAP_COUNT
} cap_t;

/**
 * System capabilities structure.
 * Detected once at initialization and cached.
 */
typedef struct {
  bool available[CAP_COUNT]; /**< Which capabilities are available */
  int priority[CAP_COUNT];   /**< Priority for each capability (higher =
                                preferred) */
  int num_cpus;              /**< Number of CPU cores */
  int num_threads;           /**< Recommended thread count */
  size_t l1_cache_size;      /**< L1 cache size in bytes (0 if unknown) */
  size_t l2_cache_size;      /**< L2 cache size in bytes (0 if unknown) */
} system_caps_t;

/**
 * Initialize capability detection.
 * Call once at program startup. Thread-safe, can be called multiple times.
 */
void caps_init(void);

/**
 * Get the system capabilities structure.
 * Calls caps_init() if not already initialized.
 *
 * @return Pointer to system capabilities (never NULL after init)
 */
const system_caps_t *caps_get(void);

/**
 * Check if a specific capability is available.
 *
 * @param cap Capability to check
 * @return true if available, false otherwise
 */
bool caps_has(cap_t cap);

/**
 * Get the name of a capability.
 *
 * @param cap Capability
 * @return Human-readable name
 */
const char *caps_name(cap_t cap);

/**
 * Get the recommended number of threads for parallel operations.
 * Takes into account CPU count and any user-configured limits.
 *
 * @return Recommended thread count (always >= 1)
 */
int caps_num_threads(void);

/**
 * Set the thread count for parallel operations.
 * Pass 0 to use all available CPUs.
 *
 * @param num_threads Desired thread count (0 = auto)
 */
void caps_set_num_threads(int num_threads);

/**
 * Get the best available capability for GEMM operations.
 * Priority: Accelerate > AMX (for FP16) > NEON > AVX2 > Scalar
 *
 * @return Best capability for GEMM
 */
cap_t caps_best_for_gemm(void);

/**
 * Get the best available capability for activation functions.
 * Priority: NEON > AVX2 > Scalar
 *
 * @return Best capability for activations
 */
cap_t caps_best_for_activation(void);

/**
 * Get the best available capability for attention operations.
 * Priority: NEON > AVX2 > Scalar
 *
 * @return Best capability for attention
 */
cap_t caps_best_for_attention(void);

#ifdef __cplusplus
}
#endif

#endif /* INFERENCE_BACKEND_CAPS_H */

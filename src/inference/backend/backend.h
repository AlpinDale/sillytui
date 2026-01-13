/**
 * @file backend.h
 * @brief Backend abstraction layer for inference operations.
 *
 * Defines the interface that all compute backends must implement.
 * This allows swapping between NEON, AMX, Accelerate, Metal, etc.
 * without changing model code.
 */

#ifndef INFERENCE_BACKEND_BACKEND_H
#define INFERENCE_BACKEND_BACKEND_H

#include "inference/backend/caps.h"
#include "inference/core/dtype.h"
#include "inference/core/tensor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct backend backend_t;

/**
 * Backend operations vtable.
 * Each backend implements these operations.
 */
typedef struct {
  /** Backend name (e.g., "neon", "amx", "accelerate") */
  const char *name;

  /** Primary capability this backend provides */
  cap_t capability;

  /* =========================================================================
   * Lifecycle
   * =========================================================================
   */

  /**
   * Initialize the backend.
   * @param backend Backend instance to initialize
   * @return true on success, false on failure
   */
  bool (*init)(backend_t *backend);

  /**
   * Destroy the backend and free resources.
   * @param backend Backend instance to destroy
   */
  void (*destroy)(backend_t *backend);

  /* =========================================================================
   * GEMM Operations
   * =========================================================================
   */

  /**
   * General matrix multiplication: C = A @ B
   * Supports transposition of A and/or B.
   *
   * @param backend Backend instance
   * @param A Input tensor A [M x K] (or [K x M] if transposed)
   * @param B Input tensor B [K x N] (or [N x K] if transposed)
   * @param C Output tensor C [M x N]
   * @param transpose_A Whether to transpose A
   * @param transpose_B Whether to transpose B
   */
  void (*gemm)(backend_t *backend, const tensor_t *A, const tensor_t *B,
               tensor_t *C, bool transpose_A, bool transpose_B);

  /**
   * Matrix-vector multiplication: y = A @ x
   * @param backend Backend instance
   * @param A Weight matrix [M x K]
   * @param x Input vector [K]
   * @param y Output vector [M]
   */
  void (*gemv)(backend_t *backend, const tensor_t *A, const tensor_t *x,
               tensor_t *y);

  /* =========================================================================
   * Activation Functions
   * =========================================================================
   */

  /**
   * SiLU activation: out = x * sigmoid(x)
   * @param backend Backend instance
   * @param out Output tensor
   * @param in Input tensor
   */
  void (*silu)(backend_t *backend, tensor_t *out, const tensor_t *in);

  /**
   * SiLU + element-wise multiplication (for SwiGLU):
   * out = silu(gate) * up
   * @param backend Backend instance
   * @param out Output tensor
   * @param gate Gate tensor
   * @param up Up-projection tensor
   */
  void (*silu_mul)(backend_t *backend, tensor_t *out, const tensor_t *gate,
                   const tensor_t *up);

  /**
   * GELU activation
   * @param backend Backend instance
   * @param out Output tensor
   * @param in Input tensor
   */
  void (*gelu)(backend_t *backend, tensor_t *out, const tensor_t *in);

  /**
   * GELU-tanh approximation
   * @param backend Backend instance
   * @param out Output tensor
   * @param in Input tensor
   */
  void (*gelu_tanh)(backend_t *backend, tensor_t *out, const tensor_t *in);

  /* =========================================================================
   * Normalization
   * =========================================================================
   */

  /**
   * RMS normalization
   * @param backend Backend instance
   * @param out Output tensor
   * @param in Input tensor
   * @param weight Scale weights
   * @param eps Epsilon for numerical stability
   */
  void (*rms_norm)(backend_t *backend, tensor_t *out, const tensor_t *in,
                   const tensor_t *weight, float eps);

  /**
   * Layer normalization
   * @param backend Backend instance
   * @param out Output tensor
   * @param in Input tensor
   * @param weight Scale weights (can be NULL)
   * @param bias Bias (can be NULL)
   * @param eps Epsilon for numerical stability
   */
  void (*layer_norm)(backend_t *backend, tensor_t *out, const tensor_t *in,
                     const tensor_t *weight, const tensor_t *bias, float eps);

  /* =========================================================================
   * Positional Encoding
   * =========================================================================
   */

  /**
   * Apply rotary position embeddings (RoPE)
   * @param backend Backend instance
   * @param query Query tensor [batch, num_heads, seq_len, head_dim]
   * @param key Key tensor [batch, num_kv_heads, seq_len, head_dim]
   * @param cos_sin_cache Precomputed cos/sin values
   * @param positions Position indices [seq_len]
   * @param num_heads Number of query heads
   * @param num_kv_heads Number of key/value heads
   * @param head_dim Head dimension
   * @param is_neox Use NeoX-style interleaving (LLaMA) vs GPT-J style
   */
  void (*rope)(backend_t *backend, tensor_t *query, tensor_t *key,
               const tensor_t *cos_sin_cache, const int64_t *positions,
               int num_heads, int num_kv_heads, int head_dim, bool is_neox);

  /* =========================================================================
   * Attention
   * =========================================================================
   */

  /**
   * Scaled dot-product attention with KV cache
   * @param backend Backend instance
   * @param out Output tensor [batch, seq_len, num_heads, head_dim]
   * @param Q Query tensor
   * @param K Key tensor (current tokens)
   * @param V Value tensor (current tokens)
   * @param key_cache KV cache for keys
   * @param value_cache KV cache for values
   * @param cache_len Current length of cache
   * @param scale Attention scale factor
   */
  void (*attention)(backend_t *backend, tensor_t *out, const tensor_t *Q,
                    const tensor_t *K, const tensor_t *V, tensor_t *key_cache,
                    tensor_t *value_cache, int cache_len, float scale);

  /* =========================================================================
   * Softmax and Sampling
   * =========================================================================
   */

  /**
   * Softmax along a dimension
   * @param backend Backend instance
   * @param out Output tensor
   * @param in Input tensor
   * @param axis Axis to apply softmax (default: -1, last axis)
   */
  void (*softmax)(backend_t *backend, tensor_t *out, const tensor_t *in,
                  int axis);

  /**
   * Sample from a probability distribution
   * @param backend Backend instance
   * @param logits Logits tensor [vocab_size]
   * @param temperature Sampling temperature
   * @param top_k Top-k filtering (0 = disabled)
   * @param top_p Top-p (nucleus) filtering (1.0 = disabled)
   * @param rng Random state (backend-specific, can be NULL for default)
   * @return Sampled token index
   */
  int (*sample)(backend_t *backend, const tensor_t *logits, float temperature,
                int top_k, float top_p, void *rng);

  /* =========================================================================
   * Embedding
   * =========================================================================
   */

  /**
   * Embedding lookup
   * @param backend Backend instance
   * @param out Output tensor [num_tokens, embed_dim]
   * @param weight Embedding weight table [vocab_size, embed_dim]
   * @param indices Token indices [num_tokens]
   * @param num_indices Number of tokens
   */
  void (*embedding_lookup)(backend_t *backend, tensor_t *out,
                           const tensor_t *weight, const int64_t *indices,
                           int num_indices);

  /* =========================================================================
   * KV Cache Operations
   * =========================================================================
   */

  /**
   * Update KV cache with new keys/values
   * @param backend Backend instance
   * @param key_cache Key cache tensor
   * @param value_cache Value cache tensor
   * @param new_keys New keys to append
   * @param new_values New values to append
   * @param cache_len Current cache length
   * @return New cache length
   */
  int (*kv_cache_update)(backend_t *backend, tensor_t *key_cache,
                         tensor_t *value_cache, const tensor_t *new_keys,
                         const tensor_t *new_values, int cache_len);

} backend_ops_t;

/**
 * Backend instance structure.
 */
struct backend {
  const backend_ops_t *ops; /**< Operations vtable */
  void *ctx;                /**< Backend-specific context */
  int num_threads;          /**< Thread count for parallel operations */
  dtype_t preferred_dtype;  /**< Preferred dtype for this backend */
};

/* ============================================================================
 * Backend Management
 * ============================================================================
 */

/**
 * Create a backend with a specific capability.
 * Falls back to less capable backends if requested one isn't available.
 *
 * @param preferred Preferred capability (or CAP_SCALAR for any available)
 * @return New backend instance, or NULL on failure
 */
backend_t *backend_create(cap_t preferred);

/**
 * Create the best available backend for the current system.
 * @return New backend instance, or NULL on failure
 */
backend_t *backend_create_best(void);

/**
 * Destroy a backend and free resources.
 * @param backend Backend to destroy (can be NULL)
 */
void backend_destroy(backend_t *backend);

/**
 * Get the default (globally shared) backend instance.
 * Created lazily on first call, using the best available backend.
 * Thread-safe.
 *
 * @return Default backend (never NULL after initialization)
 */
backend_t *backend_default(void);

/**
 * Set the default backend.
 * Takes ownership of the backend.
 *
 * @param backend New default backend
 */
void backend_set_default(backend_t *backend);

/**
 * Set the number of threads for a backend.
 * @param backend Backend instance
 * @param num_threads Number of threads (0 = use all CPUs)
 */
void backend_set_num_threads(backend_t *backend, int num_threads);

/**
 * Get the number of threads for a backend.
 * @param backend Backend instance
 * @return Thread count
 */
int backend_get_num_threads(const backend_t *backend);

/* ============================================================================
 * Convenience Wrappers
 *
 * These use the default backend and provide simpler interfaces.
 * ============================================================================
 */

/** GEMM using default backend */
void backend_gemm(const tensor_t *A, const tensor_t *B, tensor_t *C,
                  bool transpose_A, bool transpose_B);

/** RMS norm using default backend */
void backend_rms_norm(tensor_t *out, const tensor_t *in, const tensor_t *weight,
                      float eps);

/** SiLU using default backend */
void backend_silu(tensor_t *out, const tensor_t *in);

/** Softmax using default backend */
void backend_softmax(tensor_t *out, const tensor_t *in, int axis);

#ifdef __cplusplus
}
#endif

#endif /* INFERENCE_BACKEND_BACKEND_H */

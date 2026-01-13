/**
 * @file tensor.h
 * @brief Tensor abstraction for inference operations.
 *
 * Provides a unified tensor type that encapsulates data pointer, dtype,
 * shape, and stride information. This replaces raw void* and float\* or
 * uint16_t\* pointers throughout the inference code.
 */

#ifndef INFERENCE_CORE_TENSOR_H
#define INFERENCE_CORE_TENSOR_H

#include "inference/core/dtype.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of dimensions supported */
#define TENSOR_MAX_DIMS 8

/**
 * Tensor structure containing data and metadata.
 */
typedef struct {
  void *data;                      /**< Raw data pointer */
  dtype_t dtype;                   /**< Data type of elements */
  int ndim;                        /**< Number of dimensions */
  int64_t shape[TENSOR_MAX_DIMS];  /**< Shape (size of each dimension) */
  int64_t stride[TENSOR_MAX_DIMS]; /**< Stride in elements (not bytes) */
  size_t numel;                    /**< Total number of elements */
  size_t nbytes;                   /**< Total size in bytes */
  bool owns_data; /**< If true, tensor_free will free the data pointer */
} tensor_t;

/* ============================================================================
 * Creation and Destruction
 * ============================================================================
 */

/**
 * Create a new tensor with allocated memory.
 * The tensor owns its data and will free it on tensor_free.
 *
 * @param dtype Data type for elements
 * @param ndim Number of dimensions
 * @param shape Array of dimension sizes
 * @return Pointer to new tensor, or NULL on failure
 */
tensor_t *tensor_create(dtype_t dtype, int ndim, const int64_t *shape);

/**
 * Create a tensor that wraps existing data (does not own it).
 * The tensor does NOT own the data - caller is responsible for freeing it.
 * Assumes contiguous row-major layout.
 *
 * @param data Pointer to existing data
 * @param dtype Data type of the data
 * @param ndim Number of dimensions
 * @param shape Array of dimension sizes
 * @return Pointer to new tensor, or NULL on failure
 */
tensor_t *tensor_wrap(void *data, dtype_t dtype, int ndim,
                      const int64_t *shape);

/**
 * Initialize a tensor in-place (stack-allocated or embedded in struct).
 * Does not allocate the tensor_t itself.
 *
 * @param t Tensor to initialize
 * @param data Data pointer (or NULL to allocate)
 * @param dtype Data type
 * @param ndim Number of dimensions
 * @param shape Array of dimension sizes
 * @return 0 on success, -1 on failure
 */
int tensor_init(tensor_t *t, void *data, dtype_t dtype, int ndim,
                const int64_t *shape);

/**
 * Free a tensor's resources.
 * If tensor owns its data, the data is also freed.
 * If tensor was heap-allocated (via tensor_create/wrap), the struct is freed.
 *
 * @param t Tensor to free (can be NULL)
 */
void tensor_free(tensor_t *t);

/**
 * Reset a tensor to uninitialized state without freeing the struct.
 * Frees owned data if applicable.
 *
 * @param t Tensor to reset
 */
void tensor_reset(tensor_t *t);

/* ============================================================================
 * Views and Slicing
 * ============================================================================
 */

/**
 * Create a view into a tensor starting at an offset.
 * The view does NOT own the underlying data.
 *
 * @param src Source tensor
 * @param offset Offset in elements from start
 * @param ndim Number of dimensions for the view
 * @param shape Shape of the view
 * @return Pointer to new view tensor, or NULL on failure
 */
tensor_t *tensor_view(const tensor_t *src, int64_t offset, int ndim,
                      const int64_t *shape);

/**
 * Create a view that reshapes the tensor without copying data.
 * Total element count must match.
 *
 * @param src Source tensor (must be contiguous)
 * @param ndim New number of dimensions
 * @param shape New shape
 * @return Pointer to new view tensor, or NULL if not contiguous or shape
 * mismatch
 */
tensor_t *tensor_reshape_view(const tensor_t *src, int ndim,
                              const int64_t *shape);

/* ============================================================================
 * Accessors
 * ============================================================================
 */

/**
 * Get the data pointer.
 * @param t Tensor
 * @return Data pointer (may be NULL for uninitialized tensor)
 */
static inline void *tensor_data(tensor_t *t) { return t ? t->data : NULL; }

/**
 * Get the data pointer (const version).
 * @param t Tensor
 * @return Data pointer (may be NULL for uninitialized tensor)
 */
static inline const void *tensor_data_const(const tensor_t *t) {
  return t ? t->data : NULL;
}

/**
 * Get the data pointer cast to float*.
 * Caller must ensure dtype is DTYPE_F32.
 */
static inline float *tensor_data_f32(tensor_t *t) { return (float *)t->data; }

/**
 * Get the data pointer cast to const float*.
 */
static inline const float *tensor_data_f32_const(const tensor_t *t) {
  return (const float *)t->data;
}

/**
 * Get the data pointer cast to uint16_t* (for F16/BF16).
 */
static inline uint16_t *tensor_data_f16(tensor_t *t) {
  return (uint16_t *)t->data;
}

/**
 * Get the data pointer cast to const uint16_t*.
 */
static inline const uint16_t *tensor_data_f16_const(const tensor_t *t) {
  return (const uint16_t *)t->data;
}

/**
 * Get the total number of elements.
 */
static inline size_t tensor_numel(const tensor_t *t) {
  return t ? t->numel : 0;
}

/**
 * Get the total size in bytes.
 */
static inline size_t tensor_nbytes(const tensor_t *t) {
  return t ? t->nbytes : 0;
}

/**
 * Get the data type.
 */
static inline dtype_t tensor_dtype(const tensor_t *t) {
  return t ? t->dtype : DTYPE_F32;
}

/**
 * Get the number of dimensions.
 */
static inline int tensor_ndim(const tensor_t *t) { return t ? t->ndim : 0; }

/**
 * Get a dimension size.
 * @param t Tensor
 * @param dim Dimension index (supports negative indexing: -1 = last)
 * @return Size of dimension, or -1 if invalid
 */
int64_t tensor_dim(const tensor_t *t, int dim);

/* ============================================================================
 * Properties and Checks
 * ============================================================================
 */

/**
 * Check if tensor is contiguous in memory (row-major).
 * @param t Tensor
 * @return true if contiguous, false otherwise
 */
bool tensor_is_contiguous(const tensor_t *t);

/**
 * Check if two tensors have the same shape.
 * @param a First tensor
 * @param b Second tensor
 * @return true if shapes match, false otherwise
 */
bool tensor_shape_eq(const tensor_t *a, const tensor_t *b);

/**
 * Check if tensor is valid (initialized with data).
 * @param t Tensor
 * @return true if valid, false otherwise
 */
static inline bool tensor_is_valid(const tensor_t *t) {
  return t && t->data && t->ndim > 0;
}

/* ============================================================================
 * Operations
 * ============================================================================
 */

/**
 * Create a contiguous copy of a tensor.
 * If already contiguous, may return a view (does not copy).
 *
 * @param src Source tensor
 * @return New contiguous tensor, or NULL on failure
 */
tensor_t *tensor_contiguous(const tensor_t *src);

/**
 * Copy data from one tensor to another.
 * Tensors must have the same number of elements.
 * Handles dtype conversion if needed.
 *
 * @param dst Destination tensor
 * @param src Source tensor
 * @return 0 on success, -1 on failure
 */
int tensor_copy(tensor_t *dst, const tensor_t *src);

/**
 * Cast tensor to a different dtype, creating a new tensor.
 * If dtypes match, returns a view (no copy).
 *
 * @param src Source tensor
 * @param target_dtype Target data type
 * @return New tensor with target dtype, or NULL on failure
 */
tensor_t *tensor_cast(const tensor_t *src, dtype_t target_dtype);

/**
 * Fill tensor with zeros.
 * @param t Tensor to zero
 */
void tensor_zero(tensor_t *t);

/**
 * Fill tensor with a scalar value.
 * @param t Tensor to fill
 * @param value Value to fill with (as float, will be converted)
 */
void tensor_fill(tensor_t *t, float value);

#ifdef __cplusplus
}
#endif

#endif /* INFERENCE_CORE_TENSOR_H */

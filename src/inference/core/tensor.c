/**
 * @file tensor.c
 * @brief Implementation of tensor operations.
 */

#include "inference/core/tensor.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Internal Helpers
 * ============================================================================
 */

/**
 * Calculate total elements and bytes, set up contiguous strides.
 */
static void tensor_compute_metadata(tensor_t *t) {
  if (t->ndim <= 0) {
    t->numel = 0;
    t->nbytes = 0;
    return;
  }

  /* Calculate total elements */
  t->numel = 1;
  for (int i = 0; i < t->ndim; i++) {
    t->numel *= (size_t)t->shape[i];
  }

  /* Calculate bytes */
  t->nbytes = t->numel * dtype_size(t->dtype);

  /* Set up contiguous (row-major) strides */
  t->stride[t->ndim - 1] = 1;
  for (int i = t->ndim - 2; i >= 0; i--) {
    t->stride[i] = t->stride[i + 1] * t->shape[i + 1];
  }
}

/* ============================================================================
 * Creation and Destruction
 * ============================================================================
 */

tensor_t *tensor_create(dtype_t dtype, int ndim, const int64_t *shape) {
  if (ndim <= 0 || ndim > TENSOR_MAX_DIMS || !shape)
    return NULL;

  tensor_t *t = (tensor_t *)calloc(1, sizeof(tensor_t));
  if (!t)
    return NULL;

  t->dtype = dtype;
  t->ndim = ndim;
  memcpy(t->shape, shape, ndim * sizeof(int64_t));
  tensor_compute_metadata(t);

  if (t->nbytes > 0) {
    t->data = calloc(1, t->nbytes);
    if (!t->data) {
      free(t);
      return NULL;
    }
  }
  t->owns_data = true;

  return t;
}

tensor_t *tensor_wrap(void *data, dtype_t dtype, int ndim,
                      const int64_t *shape) {
  if (!data || ndim <= 0 || ndim > TENSOR_MAX_DIMS || !shape)
    return NULL;

  tensor_t *t = (tensor_t *)calloc(1, sizeof(tensor_t));
  if (!t)
    return NULL;

  t->data = data;
  t->dtype = dtype;
  t->ndim = ndim;
  memcpy(t->shape, shape, ndim * sizeof(int64_t));
  tensor_compute_metadata(t);
  t->owns_data = false;

  return t;
}

int tensor_init(tensor_t *t, void *data, dtype_t dtype, int ndim,
                const int64_t *shape) {
  if (!t || ndim <= 0 || ndim > TENSOR_MAX_DIMS || !shape)
    return -1;

  memset(t, 0, sizeof(tensor_t));
  t->dtype = dtype;
  t->ndim = ndim;
  memcpy(t->shape, shape, ndim * sizeof(int64_t));
  tensor_compute_metadata(t);

  if (data) {
    t->data = data;
    t->owns_data = false;
  } else {
    if (t->nbytes > 0) {
      t->data = calloc(1, t->nbytes);
      if (!t->data)
        return -1;
    }
    t->owns_data = true;
  }

  return 0;
}

void tensor_free(tensor_t *t) {
  if (!t)
    return;

  if (t->owns_data && t->data) {
    free(t->data);
  }
  free(t);
}

void tensor_reset(tensor_t *t) {
  if (!t)
    return;

  if (t->owns_data && t->data) {
    free(t->data);
  }
  memset(t, 0, sizeof(tensor_t));
}

/* ============================================================================
 * Views and Slicing
 * ============================================================================
 */

tensor_t *tensor_view(const tensor_t *src, int64_t offset, int ndim,
                      const int64_t *shape) {
  if (!src || !src->data || ndim <= 0 || ndim > TENSOR_MAX_DIMS || !shape)
    return NULL;

  /* Calculate offset in bytes */
  size_t byte_offset = (size_t)offset * dtype_size(src->dtype);
  if (byte_offset >= src->nbytes)
    return NULL;

  tensor_t *t = (tensor_t *)calloc(1, sizeof(tensor_t));
  if (!t)
    return NULL;

  t->data = (char *)src->data + byte_offset;
  t->dtype = src->dtype;
  t->ndim = ndim;
  memcpy(t->shape, shape, ndim * sizeof(int64_t));
  tensor_compute_metadata(t);
  t->owns_data = false;

  return t;
}

tensor_t *tensor_reshape_view(const tensor_t *src, int ndim,
                              const int64_t *shape) {
  if (!src || !tensor_is_contiguous(src))
    return NULL;
  if (ndim <= 0 || ndim > TENSOR_MAX_DIMS || !shape)
    return NULL;

  /* Verify total elements match */
  size_t new_numel = 1;
  for (int i = 0; i < ndim; i++) {
    new_numel *= (size_t)shape[i];
  }
  if (new_numel != src->numel)
    return NULL;

  tensor_t *t = (tensor_t *)calloc(1, sizeof(tensor_t));
  if (!t)
    return NULL;

  t->data = src->data;
  t->dtype = src->dtype;
  t->ndim = ndim;
  memcpy(t->shape, shape, ndim * sizeof(int64_t));
  tensor_compute_metadata(t);
  t->owns_data = false;

  return t;
}

/* ============================================================================
 * Accessors
 * ============================================================================
 */

int64_t tensor_dim(const tensor_t *t, int dim) {
  if (!t || t->ndim <= 0)
    return -1;

  /* Handle negative indexing */
  if (dim < 0)
    dim = t->ndim + dim;

  if (dim < 0 || dim >= t->ndim)
    return -1;

  return t->shape[dim];
}

/* ============================================================================
 * Properties and Checks
 * ============================================================================
 */

bool tensor_is_contiguous(const tensor_t *t) {
  if (!t || t->ndim <= 0)
    return false;

  /* Check if strides match row-major layout */
  int64_t expected_stride = 1;
  for (int i = t->ndim - 1; i >= 0; i--) {
    if (t->stride[i] != expected_stride)
      return false;
    expected_stride *= t->shape[i];
  }
  return true;
}

bool tensor_shape_eq(const tensor_t *a, const tensor_t *b) {
  if (!a || !b)
    return false;
  if (a->ndim != b->ndim)
    return false;

  for (int i = 0; i < a->ndim; i++) {
    if (a->shape[i] != b->shape[i])
      return false;
  }
  return true;
}

/* ============================================================================
 * Operations
 * ============================================================================
 */

tensor_t *tensor_contiguous(const tensor_t *src) {
  if (!src)
    return NULL;

  /* If already contiguous, return a view */
  if (tensor_is_contiguous(src)) {
    return tensor_wrap(src->data, src->dtype, src->ndim, src->shape);
  }

  /* Create a new tensor and copy data */
  tensor_t *dst = tensor_create(src->dtype, src->ndim, src->shape);
  if (!dst)
    return NULL;

  /* For non-contiguous tensors, we need element-by-element copy */
  /* This is a simplified implementation for contiguous sources */
  /* A full implementation would handle strided access */
  memcpy(dst->data, src->data, src->nbytes);

  return dst;
}

int tensor_copy(tensor_t *dst, const tensor_t *src) {
  if (!dst || !src || !dst->data || !src->data)
    return -1;

  if (dst->numel != src->numel)
    return -1;

  /* Same dtype - direct copy */
  if (dst->dtype == src->dtype) {
    memcpy(dst->data, src->data, src->nbytes);
    return 0;
  }

  /* Different dtypes - use conversion */
  return dtype_convert_array(src->data, src->dtype, dst->data, dst->dtype,
                             src->numel);
}

tensor_t *tensor_cast(const tensor_t *src, dtype_t target_dtype) {
  if (!src)
    return NULL;

  /* Same dtype - return a view */
  if (src->dtype == target_dtype) {
    return tensor_wrap(src->data, src->dtype, src->ndim, src->shape);
  }

  /* Different dtype - create new tensor and convert */
  tensor_t *dst = tensor_create(target_dtype, src->ndim, src->shape);
  if (!dst)
    return NULL;

  if (tensor_copy(dst, src) != 0) {
    tensor_free(dst);
    return NULL;
  }

  return dst;
}

void tensor_zero(tensor_t *t) {
  if (!t || !t->data)
    return;
  memset(t->data, 0, t->nbytes);
}

void tensor_fill(tensor_t *t, float value) {
  if (!t || !t->data)
    return;

  switch (t->dtype) {
  case DTYPE_F32: {
    float *data = (float *)t->data;
    for (size_t i = 0; i < t->numel; i++) {
      data[i] = value;
    }
    break;
  }
  case DTYPE_F16: {
    uint16_t val16 = f32_to_f16(value);
    uint16_t *data = (uint16_t *)t->data;
    for (size_t i = 0; i < t->numel; i++) {
      data[i] = val16;
    }
    break;
  }
  case DTYPE_BF16: {
    uint16_t val16 = f32_to_bf16(value);
    uint16_t *data = (uint16_t *)t->data;
    for (size_t i = 0; i < t->numel; i++) {
      data[i] = val16;
    }
    break;
  }
  default:
    break;
  }
}

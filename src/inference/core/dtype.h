/**
 * @file dtype.h
 * @brief Unified data type definitions and conversion utilities for inference.
 *
 * This header consolidates all dtype-related code that was previously
 * duplicated across gemm.c, activation.c, ffn.c, and other files.
 */

#ifndef INFERENCE_CORE_DTYPE_H
#define INFERENCE_CORE_DTYPE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Supported data types for inference operations.
 */
typedef enum {
  DTYPE_F32 = 0,  /**< 32-bit floating point */
  DTYPE_F16 = 1,  /**< 16-bit floating point (IEEE 754) */
  DTYPE_BF16 = 2, /**< 16-bit brain floating point */
  DTYPE_INT8 = 3, /**< 8-bit signed integer (future quantization) */
  DTYPE_INT4 = 4, /**< 4-bit signed integer (future quantization) */
  DTYPE_COUNT
} dtype_t;

/**
 * Get the size in bytes for a given dtype.
 * @param dtype The data type
 * @return Size in bytes (0 for invalid dtype)
 */
size_t dtype_size(dtype_t dtype);

/**
 * Get a human-readable name for a dtype.
 * @param dtype The data type
 * @return String name (e.g., "f32", "f16", "bf16")
 */
const char *dtype_name(dtype_t dtype);

/**
 * Check if a dtype is a floating point type.
 * @param dtype The data type
 * @return true if floating point, false otherwise
 */
int dtype_is_float(dtype_t dtype);

/* ============================================================================
 * Scalar Conversion Functions
 * ============================================================================
 */

/**
 * Convert a single float32 value to float16.
 * @param x The float32 value
 * @return The float16 value as uint16_t
 */
uint16_t f32_to_f16(float x);

/**
 * Convert a single float16 value to float32.
 * @param fp16 The float16 value as uint16_t
 * @return The float32 value
 */
float f16_to_f32(uint16_t fp16);

/**
 * Convert a single float32 value to bfloat16.
 * Uses round-to-nearest-even.
 * @param x The float32 value
 * @return The bfloat16 value as uint16_t
 */
uint16_t f32_to_bf16(float x);

/**
 * Convert a single bfloat16 value to float32.
 * @param bf16 The bfloat16 value as uint16_t
 * @return The float32 value
 */
float bf16_to_f32(uint16_t bf16);

/* ============================================================================
 * Array Conversion Functions (with SIMD acceleration where available)
 * ============================================================================
 */

/**
 * Convert an array of float32 values to float16.
 * Uses SIMD acceleration on supported platforms (ARM NEON).
 * @param src Source float32 array
 * @param dst Destination float16 array (as uint16_t)
 * @param count Number of elements to convert
 */
void f32_to_f16_array(const float *src, uint16_t *dst, size_t count);

/**
 * Convert an array of float16 values to float32.
 * Uses SIMD acceleration on supported platforms (ARM NEON).
 * @param src Source float16 array (as uint16_t)
 * @param dst Destination float32 array
 * @param count Number of elements to convert
 */
void f16_to_f32_array(const uint16_t *src, float *dst, size_t count);

/**
 * Convert an array of float32 values to bfloat16.
 * @param src Source float32 array
 * @param dst Destination bfloat16 array (as uint16_t)
 * @param count Number of elements to convert
 */
void f32_to_bf16_array(const float *src, uint16_t *dst, size_t count);

/**
 * Convert an array of bfloat16 values to float32.
 * @param src Source bfloat16 array (as uint16_t)
 * @param dst Destination float32 array
 * @param count Number of elements to convert
 */
void bf16_to_f32_array(const uint16_t *src, float *dst, size_t count);

/**
 * Convert an array between any two floating point dtypes.
 * This is a generic conversion that handles all supported float types.
 * @param src Source array
 * @param src_dtype Source data type
 * @param dst Destination array (must be pre-allocated)
 * @param dst_dtype Destination data type
 * @param count Number of elements to convert
 * @return 0 on success, -1 on error (e.g., unsupported dtype)
 */
int dtype_convert_array(const void *src, dtype_t src_dtype, void *dst,
                        dtype_t dst_dtype, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* INFERENCE_CORE_DTYPE_H */

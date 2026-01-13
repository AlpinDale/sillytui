/**
 * @file dtype.c
 * @brief Implementation of data type utilities and conversions.
 *
 * This consolidates dtype conversion code from:
 * - kernels/gemm/gemm.c
 * - kernels/gemm/gemm_neon.c
 * - kernels/activation/activation.c
 * - model/qwen3/ffn.c
 */

#include "inference/core/dtype.h"
#include <string.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define HAS_NEON 1
#endif

/* ============================================================================
 * Dtype Metadata
 * ============================================================================
 */

static const size_t dtype_sizes[DTYPE_COUNT] = {
    [DTYPE_F32] = 4,  /* float32: 4 bytes */
    [DTYPE_F16] = 2,  /* float16: 2 bytes */
    [DTYPE_BF16] = 2, /* bfloat16: 2 bytes */
    [DTYPE_INT8] = 1, /* int8: 1 byte */
    [DTYPE_INT4] = 1, /* int4: 1 byte (packed, but minimum addressable) */
};

static const char *dtype_names[DTYPE_COUNT] = {
    [DTYPE_F32] = "f32",   [DTYPE_F16] = "f16",   [DTYPE_BF16] = "bf16",
    [DTYPE_INT8] = "int8", [DTYPE_INT4] = "int4",
};

size_t dtype_size(dtype_t dtype) {
  if (dtype < 0 || dtype >= DTYPE_COUNT)
    return 0;
  return dtype_sizes[dtype];
}

const char *dtype_name(dtype_t dtype) {
  if (dtype < 0 || dtype >= DTYPE_COUNT)
    return "unknown";
  return dtype_names[dtype];
}

int dtype_is_float(dtype_t dtype) {
  return dtype == DTYPE_F32 || dtype == DTYPE_F16 || dtype == DTYPE_BF16;
}

/* ============================================================================
 * Scalar Conversions
 * ============================================================================
 */

uint16_t f32_to_bf16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  /* Round to nearest even */
  uint32_t lsb = (bits >> 16) & 1;
  uint32_t rounding_bias = 0x7fff + lsb;
  bits += rounding_bias;
  return (uint16_t)(bits >> 16);
}

float bf16_to_f32(uint16_t bf16) {
  uint32_t bits = ((uint32_t)bf16) << 16;
  float result;
  memcpy(&result, &bits, sizeof(float));
  return result;
}

uint16_t f32_to_f16(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t sign = bits & 0x80000000;
  uint32_t exp = (bits >> 23) & 0xFF;
  uint32_t mant = bits & 0x7FFFFF;

  /* Handle special cases: infinity and NaN */
  if (exp == 0xFF) {
    if (mant != 0)
      return 0x7E00;                          /* NaN */
    return (uint16_t)((sign >> 16) | 0x7C00); /* Infinity */
  }

  /* Handle zero */
  if (exp == 0 && mant == 0)
    return (uint16_t)(sign >> 16);

  /* Compute new exponent */
  int32_t new_exp = (int32_t)exp - 127 + 15;

  /* Handle underflow */
  if (new_exp <= 0)
    return (uint16_t)(sign >> 16);

  /* Handle overflow */
  if (new_exp >= 31)
    return (uint16_t)((sign >> 16) | 0x7C00);

  uint16_t fp16_exp = (uint16_t)(new_exp << 10);
  uint16_t fp16_mant = (uint16_t)(mant >> 13);
  return (uint16_t)((sign >> 16) | fp16_exp | fp16_mant);
}

float f16_to_f32(uint16_t fp16) {
  uint32_t sign = (fp16 & 0x8000) << 16;
  uint32_t exp = (fp16 >> 10) & 0x1F;
  uint32_t mant = fp16 & 0x3FF;

  uint32_t f32_bits;
  if (exp == 0) {
    if (mant == 0) {
      /* Zero */
      f32_bits = sign;
    } else {
      /* Denormalized number - renormalize */
      int e = -14;
      while ((mant & 0x400) == 0) {
        mant <<= 1;
        e--;
      }
      mant &= 0x3FF;
      f32_bits = sign | (((uint32_t)(e + 127)) << 23) | (mant << 13);
    }
  } else if (exp == 31) {
    /* Infinity or NaN */
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    /* Normalized number */
    f32_bits = sign | (((uint32_t)(exp - 15 + 127)) << 23) | (mant << 13);
  }

  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

/* ============================================================================
 * Array Conversions
 * ============================================================================
 */

void bf16_to_f32_array(const uint16_t *src, float *dst, size_t count) {
  for (size_t i = 0; i < count; i++) {
    dst[i] = bf16_to_f32(src[i]);
  }
}

void f32_to_bf16_array(const float *src, uint16_t *dst, size_t count) {
  for (size_t i = 0; i < count; i++) {
    dst[i] = f32_to_bf16(src[i]);
  }
}

void f16_to_f32_array(const uint16_t *src, float *dst, size_t count) {
#ifdef HAS_NEON
  size_t i = 0;
  /* Process 8 elements at a time with NEON */
  for (; i + 8 <= count; i += 8) {
    float16x8_t f16 = vld1q_f16((const float16_t *)&src[i]);
    float32x4_t lo = vcvt_f32_f16(vget_low_f16(f16));
    float32x4_t hi = vcvt_f32_f16(vget_high_f16(f16));
    vst1q_f32(&dst[i], lo);
    vst1q_f32(&dst[i + 4], hi);
  }
  /* Handle remaining elements */
  for (; i < count; i++) {
    dst[i] = f16_to_f32(src[i]);
  }
#else
  for (size_t i = 0; i < count; i++) {
    dst[i] = f16_to_f32(src[i]);
  }
#endif
}

void f32_to_f16_array(const float *src, uint16_t *dst, size_t count) {
#ifdef HAS_NEON
  size_t i = 0;
  /* Process 8 elements at a time with NEON */
  for (; i + 8 <= count; i += 8) {
    float32x4_t lo = vld1q_f32(&src[i]);
    float32x4_t hi = vld1q_f32(&src[i + 4]);
    float16x8_t f16 = vcombine_f16(vcvt_f16_f32(lo), vcvt_f16_f32(hi));
    vst1q_f16((float16_t *)&dst[i], f16);
  }
  /* Handle remaining elements */
  for (; i < count; i++) {
    dst[i] = f32_to_f16(src[i]);
  }
#else
  for (size_t i = 0; i < count; i++) {
    dst[i] = f32_to_f16(src[i]);
  }
#endif
}

/* ============================================================================
 * Generic Array Conversion
 * ============================================================================
 */

int dtype_convert_array(const void *src, dtype_t src_dtype, void *dst,
                        dtype_t dst_dtype, size_t count) {
  if (!src || !dst || count == 0)
    return -1;

  /* Same type - just copy */
  if (src_dtype == dst_dtype) {
    memcpy(dst, src, count * dtype_size(src_dtype));
    return 0;
  }

  /* F32 -> F16 */
  if (src_dtype == DTYPE_F32 && dst_dtype == DTYPE_F16) {
    f32_to_f16_array((const float *)src, (uint16_t *)dst, count);
    return 0;
  }

  /* F16 -> F32 */
  if (src_dtype == DTYPE_F16 && dst_dtype == DTYPE_F32) {
    f16_to_f32_array((const uint16_t *)src, (float *)dst, count);
    return 0;
  }

  /* F32 -> BF16 */
  if (src_dtype == DTYPE_F32 && dst_dtype == DTYPE_BF16) {
    f32_to_bf16_array((const float *)src, (uint16_t *)dst, count);
    return 0;
  }

  /* BF16 -> F32 */
  if (src_dtype == DTYPE_BF16 && dst_dtype == DTYPE_F32) {
    bf16_to_f32_array((const uint16_t *)src, (float *)dst, count);
    return 0;
  }

  /* BF16 -> F16: go through F32 */
  if (src_dtype == DTYPE_BF16 && dst_dtype == DTYPE_F16) {
    for (size_t i = 0; i < count; i++) {
      float tmp = bf16_to_f32(((const uint16_t *)src)[i]);
      ((uint16_t *)dst)[i] = f32_to_f16(tmp);
    }
    return 0;
  }

  /* F16 -> BF16: go through F32 */
  if (src_dtype == DTYPE_F16 && dst_dtype == DTYPE_BF16) {
    for (size_t i = 0; i < count; i++) {
      float tmp = f16_to_f32(((const uint16_t *)src)[i]);
      ((uint16_t *)dst)[i] = f32_to_bf16(tmp);
    }
    return 0;
  }

  /* Unsupported conversion */
  return -1;
}

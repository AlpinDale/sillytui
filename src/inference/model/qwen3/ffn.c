#include "ffn.h"
#include "inference/kernels/activation/activation.h"
#include "inference/kernels/gemm/gemm.h"
#include <stdlib.h>
#include <string.h>

static inline float fp16_to_float_ffn(uint16_t fp16) {
  uint32_t sign = (fp16 & 0x8000) << 16;
  uint32_t exp = (fp16 >> 10) & 0x1F;
  uint32_t mant = fp16 & 0x3FF;

  uint32_t f32_bits;
  if (exp == 0) {
    if (mant == 0) {
      f32_bits = sign;
    } else {
      int e = -14;
      while ((mant & 0x400) == 0) {
        mant <<= 1;
        e--;
      }
      mant &= 0x3FF;
      f32_bits = sign | (((uint32_t)(e + 127)) << 23) | (mant << 13);
    }
  } else if (exp == 31) {
    f32_bits = sign | 0x7F800000 | (mant << 13);
  } else {
    f32_bits = sign | (((uint32_t)(exp - 15 + 127)) << 23) | (mant << 13);
  }

  float result;
  memcpy(&result, &f32_bits, sizeof(float));
  return result;
}

static inline uint16_t float_to_fp16_ffn(float x) {
  uint32_t bits;
  memcpy(&bits, &x, sizeof(float));
  uint32_t sign = bits & 0x80000000;
  uint32_t exp = (bits >> 23) & 0xFF;
  uint32_t mant = bits & 0x7FFFFF;

  if (exp == 0xFF) {
    if (mant != 0)
      return 0x7E00;
    return (uint16_t)((sign >> 16) | 0x7C00);
  }
  if (exp == 0 && mant == 0)
    return (uint16_t)(sign >> 16);

  int32_t new_exp = (int32_t)exp - 127 + 15;
  if (new_exp <= 0)
    return (uint16_t)(sign >> 16);
  if (new_exp >= 31)
    return (uint16_t)((sign >> 16) | 0x7C00);

  uint16_t fp16_exp = (uint16_t)(new_exp << 10);
  uint16_t fp16_mant = (uint16_t)(mant >> 13);
  return (uint16_t)((sign >> 16) | fp16_exp | fp16_mant);
}

void qwen3_ffn_f32(float *output, const float *input, const float *gate_proj,
                   const float *up_proj, const float *down_proj, int seq_len,
                   int hidden_size, int intermediate_size) {
  float *gate = (float *)malloc(seq_len * intermediate_size * sizeof(float));
  float *up = (float *)malloc(seq_len * intermediate_size * sizeof(float));
  if (!gate || !up) {
    if (gate)
      free(gate);
    if (up)
      free(up);
    return;
  }

  gemm_f32(input, gate_proj, gate, seq_len, intermediate_size, hidden_size,
           false, true);

  gemm_f32(input, up_proj, up, seq_len, intermediate_size, hidden_size, false,
           true);

  silu_f32(gate, gate, seq_len, intermediate_size);

  for (int i = 0; i < seq_len * intermediate_size; i++) {
    gate[i] = gate[i] * up[i];
  }

  gemm_f32(gate, down_proj, output, seq_len, hidden_size, intermediate_size,
           false, true);

  free(gate);
  free(up);
}

void qwen3_ffn_f16(uint16_t *output, const uint16_t *input,
                   const uint16_t *gate_proj, const uint16_t *up_proj,
                   const uint16_t *down_proj, int seq_len, int hidden_size,
                   int intermediate_size) {
  uint16_t *gate =
      (uint16_t *)malloc(seq_len * intermediate_size * sizeof(uint16_t));
  uint16_t *up =
      (uint16_t *)malloc(seq_len * intermediate_size * sizeof(uint16_t));
  uint16_t *gate_up =
      (uint16_t *)malloc(seq_len * intermediate_size * 2 * sizeof(uint16_t));
  uint16_t *gate_out =
      (uint16_t *)malloc(seq_len * intermediate_size * sizeof(uint16_t));
  if (!gate || !up || !gate_up || !gate_out) {
    if (gate)
      free(gate);
    if (up)
      free(up);
    if (gate_up)
      free(gate_up);
    if (gate_out)
      free(gate_out);
    return;
  }

  gemm_f16(input, gate_proj, gate, seq_len, intermediate_size, hidden_size);
  gemm_f16(input, up_proj, up, seq_len, intermediate_size, hidden_size);

  for (int i = 0; i < seq_len; i++) {
    memcpy(gate_up + i * 2 * intermediate_size, gate + i * intermediate_size,
           intermediate_size * sizeof(uint16_t));
    memcpy(gate_up + i * 2 * intermediate_size + intermediate_size,
           up + i * intermediate_size, intermediate_size * sizeof(uint16_t));
  }

  silu_and_mul_f16(gate_out, gate_up, seq_len, intermediate_size);

  gemm_f16(gate_out, down_proj, output, seq_len, hidden_size,
           intermediate_size);

  free(gate);
  free(up);
  free(gate_up);
  free(gate_out);
}

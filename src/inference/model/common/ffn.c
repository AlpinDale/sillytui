#include "ffn.h"
#include "inference/kernels/activation/activation.h"
#include "inference/kernels/gemm/gemm.h"
#include <stdlib.h>
#include <string.h>

void ffn_forward_f32(float *output, const float *input, const ffn_layer_t *ffn,
                     int seq_len) {
  if (!output || !input || !ffn)
    return;

  int hidden_size = ffn->hidden_size;
  int intermediate_size = ffn->intermediate_size;

  float *gate = (float *)malloc(seq_len * intermediate_size * sizeof(float));
  float *up = (float *)malloc(seq_len * intermediate_size * sizeof(float));
  if (!gate || !up) {
    free(gate);
    free(up);
    return;
  }

  const float *gate_w = tensor_data_f32_const(ffn->gate_proj);
  const float *up_w = tensor_data_f32_const(ffn->up_proj);
  const float *down_w = tensor_data_f32_const(ffn->down_proj);

  gemm_f32(input, gate_w, gate, seq_len, intermediate_size, hidden_size, false,
           true);
  gemm_f32(input, up_w, up, seq_len, intermediate_size, hidden_size, false,
           true);

  switch (ffn->activation) {
  case ACT_SILU:
    silu_f32(gate, gate, seq_len, intermediate_size);
    break;
  case ACT_GELU:
    gelu_f32(gate, gate, seq_len, intermediate_size);
    break;
  case ACT_GELU_TANH:
    gelu_tanh_f32(gate, gate, seq_len, intermediate_size);
    break;
  case ACT_GELU_QUICK:
    gelu_quick_f32(gate, gate, seq_len, intermediate_size);
    break;
  default:
    silu_f32(gate, gate, seq_len, intermediate_size);
    break;
  }

  for (int i = 0; i < seq_len * intermediate_size; i++) {
    gate[i] = gate[i] * up[i];
  }

  gemm_f32(gate, down_w, output, seq_len, hidden_size, intermediate_size, false,
           true);

  free(gate);
  free(up);
}

void ffn_forward_f16(uint16_t *output, const uint16_t *input,
                     const ffn_layer_t *ffn, int seq_len) {
  if (!output || !input || !ffn)
    return;

  int hidden_size = ffn->hidden_size;
  int intermediate_size = ffn->intermediate_size;

  uint16_t *gate =
      (uint16_t *)malloc(seq_len * intermediate_size * sizeof(uint16_t));
  uint16_t *up =
      (uint16_t *)malloc(seq_len * intermediate_size * sizeof(uint16_t));
  uint16_t *gate_up =
      (uint16_t *)malloc(seq_len * intermediate_size * 2 * sizeof(uint16_t));
  uint16_t *gate_out =
      (uint16_t *)malloc(seq_len * intermediate_size * sizeof(uint16_t));

  if (!gate || !up || !gate_up || !gate_out) {
    free(gate);
    free(up);
    free(gate_up);
    free(gate_out);
    return;
  }

  const uint16_t *gate_w = tensor_data_f16_const(ffn->gate_proj);
  const uint16_t *up_w = tensor_data_f16_const(ffn->up_proj);
  const uint16_t *down_w = tensor_data_f16_const(ffn->down_proj);

  gemm_f16(input, gate_w, gate, seq_len, intermediate_size, hidden_size);
  gemm_f16(input, up_w, up, seq_len, intermediate_size, hidden_size);

  for (int i = 0; i < seq_len; i++) {
    memcpy(gate_up + i * 2 * intermediate_size, gate + i * intermediate_size,
           intermediate_size * sizeof(uint16_t));
    memcpy(gate_up + i * 2 * intermediate_size + intermediate_size,
           up + i * intermediate_size, intermediate_size * sizeof(uint16_t));
  }

  switch (ffn->activation) {
  case ACT_SILU:
    silu_and_mul_f16(gate_out, gate_up, seq_len, intermediate_size);
    break;
  case ACT_GELU:
    gelu_and_mul_f16(gate_out, gate_up, seq_len, intermediate_size);
    break;
  case ACT_GELU_TANH:
    gelu_tanh_and_mul_f16(gate_out, gate_up, seq_len, intermediate_size);
    break;
  case ACT_GELU_QUICK:
    gelu_quick_and_mul_f16(gate_out, gate_up, seq_len, intermediate_size);
    break;
  default:
    silu_and_mul_f16(gate_out, gate_up, seq_len, intermediate_size);
    break;
  }

  gemm_f16(gate_out, down_w, output, seq_len, hidden_size, intermediate_size);

  free(gate);
  free(up);
  free(gate_up);
  free(gate_out);
}

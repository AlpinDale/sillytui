#include "inference/model/ffn.h"
#include "inference/kernels/activation/activation.h"
#include "inference/kernels/gemm/gemm.h"
#include <stdlib.h>
#include <string.h>

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

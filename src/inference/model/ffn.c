#include "inference/model/ffn.h"
#include "inference/kernels/activation/activation.h"
#include "inference/kernels/gemm/gemm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #region agent log
static void debug_log(const char *location, const char *message,
                      const char *hypothesis_id, const char *data_json) {
  FILE *f = fopen("/Users/alpindale/repos/cursed-silly/.cursor/debug.log", "a");
  if (f) {
    fprintf(f,
            "{\"sessionId\":\"debug-session\",\"runId\":\"run1\","
            "\"hypothesisId\":\"%s\",\"location\":\"%s\",\"message\":\"%s\","
            "\"data\":%s,\"timestamp\":%ld}\n",
            hypothesis_id, location, message, data_json, time(NULL));
    fclose(f);
  }
}
// #endregion

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

  // #region agent log
  if (seq_len == 1) {
    char ffn_pre_buf[256];
    snprintf(ffn_pre_buf, sizeof(ffn_pre_buf),
             "{\"ffn_input_first3\":[%.6f,%.6f,%.6f],\"gate_proj_first3\":[%."
             "6f,%.6f,%.6f],\"up_proj_first3\":[%.6f,%.6f,%.6f]}",
             input[0], input[1], input[2], gate_proj[0], gate_proj[1],
             gate_proj[2], up_proj[0], up_proj[1], up_proj[2]);
    debug_log("ffn.c:20", "Before FFN projections", "G", ffn_pre_buf);
  }
  // #endregion

  gemm_f32(input, gate_proj, gate, seq_len, intermediate_size, hidden_size,
           false, true);

  gemm_f32(input, up_proj, up, seq_len, intermediate_size, hidden_size, false,
           true);

  // #region agent log
  if (seq_len == 1) {
    char ffn_mid_buf[256];
    snprintf(ffn_mid_buf, sizeof(ffn_mid_buf),
             "{\"gate_after_proj_first3\":[%.6f,%.6f,%.6f],\"up_after_proj_"
             "first3\":[%.6f,%.6f,%.6f]}",
             gate[0], gate[1], gate[2], up[0], up[1], up[2]);
    debug_log("ffn.c:28", "After gate/up projections", "G", ffn_mid_buf);
  }
  // #endregion

  silu_f32(gate, gate, seq_len, intermediate_size);

  // #region agent log
  if (seq_len == 1) {
    char ffn_silu_buf[256];
    snprintf(ffn_silu_buf, sizeof(ffn_silu_buf),
             "{\"gate_after_silu_first3\":[%.6f,%.6f,%.6f]}", gate[0], gate[1],
             gate[2]);
    debug_log("ffn.c:32", "After SiLU", "G", ffn_silu_buf);
  }
  // #endregion

  for (int i = 0; i < seq_len * intermediate_size; i++) {
    gate[i] = gate[i] * up[i];
  }

  // #region agent log
  if (seq_len == 1) {
    char ffn_mult_buf[256];
    snprintf(ffn_mult_buf, sizeof(ffn_mult_buf),
             "{\"gate_after_mult_first3\":[%.6f,%.6f,%.6f]}", gate[0], gate[1],
             gate[2]);
    debug_log("ffn.c:36", "After gate*up", "G", ffn_mult_buf);
  }
  // #endregion

  gemm_f32(gate, down_proj, output, seq_len, hidden_size, intermediate_size,
           false, true);

  // #region agent log
  if (seq_len == 1) {
    char ffn_out_buf[256];
    snprintf(ffn_out_buf, sizeof(ffn_out_buf),
             "{\"ffn_output_first3\":[%.6f,%.6f,%.6f]}", output[0], output[1],
             output[2]);
    debug_log("ffn.c:40", "After down projection", "G", ffn_out_buf);
  }
  // #endregion

  free(gate);
  free(up);
}

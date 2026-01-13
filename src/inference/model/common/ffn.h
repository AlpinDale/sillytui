#ifndef INFERENCE_MODEL_COMMON_FFN_H
#define INFERENCE_MODEL_COMMON_FFN_H

#include "inference/core/tensor.h"
#include "inference/model/config.h"
#include <stdint.h>

typedef struct {
  tensor_t *gate_proj;
  tensor_t *up_proj;
  tensor_t *down_proj;
  int hidden_size;
  int intermediate_size;
  activation_type_t activation;
  bool has_bias;
  tensor_t *gate_bias;
  tensor_t *up_bias;
  tensor_t *down_bias;
} ffn_layer_t;

void ffn_forward_f32(float *output, const float *input, const ffn_layer_t *ffn,
                     int seq_len);
void ffn_forward_f16(uint16_t *output, const uint16_t *input,
                     const ffn_layer_t *ffn, int seq_len);

#endif

#ifndef QWEN3_FFN_H
#define QWEN3_FFN_H

#include <stddef.h>
#include <stdint.h>

void qwen3_ffn_f32(float *output, const float *input, const float *gate_proj,
                   const float *up_proj, const float *down_proj, int seq_len,
                   int hidden_size, int intermediate_size);

void qwen3_ffn_f16(uint16_t *output, const uint16_t *input,
                   const uint16_t *gate_proj, const uint16_t *up_proj,
                   const uint16_t *down_proj, int seq_len, int hidden_size,
                   int intermediate_size);

#endif

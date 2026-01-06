#ifndef SOFTMAX_H
#define SOFTMAX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void softmax_f32(float *output, const float *input, int num_rows, int row_size);
void softmax_bf16(uint16_t *output, const uint16_t *input, int num_rows,
                  int row_size);
void softmax_f16(uint16_t *output, const uint16_t *input, int num_rows,
                 int row_size);

void softmax_f32_inplace(float *data, int num_rows, int row_size);
void softmax_bf16_inplace(uint16_t *data, int num_rows, int row_size);
void softmax_f16_inplace(uint16_t *data, int num_rows, int row_size);

void softmax_f32_scaled(float *output, const float *input, int num_rows,
                        int row_size, float scale);
void softmax_bf16_scaled(uint16_t *output, const uint16_t *input, int num_rows,
                         int row_size, float scale);
void softmax_f16_scaled(uint16_t *output, const uint16_t *input, int num_rows,
                        int row_size, float scale);

#ifdef __cplusplus
}
#endif

#endif

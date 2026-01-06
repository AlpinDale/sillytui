#ifndef SOFTMAX_KERNELS_H
#define SOFTMAX_KERNELS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool has_neon;
  bool has_avx2;
  bool has_avx512;
} softmax_caps_t;

softmax_caps_t softmax_get_capabilities(void);

void softmax_set_num_threads(int num_threads);
int softmax_get_num_threads(void);

void softmax_f32_kernel(float *output, const float *input, int num_rows,
                        int row_size, float scale);
void softmax_bf16_kernel(uint16_t *output, const uint16_t *input, int num_rows,
                         int row_size, float scale);
void softmax_f16_kernel(uint16_t *output, const uint16_t *input, int num_rows,
                        int row_size, float scale);

#endif

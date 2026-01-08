#ifndef INFERENCE_MODEL_BASE_H
#define INFERENCE_MODEL_BASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  INFERENCE_DTYPE_F32 = 0,
  INFERENCE_DTYPE_F16 = 1,
} inference_dtype_t;

typedef struct inference_model inference_model_t;

typedef struct {
  bool (*load)(inference_model_t *model, const char *model_dir,
               inference_dtype_t dtype);
  void (*free)(inference_model_t *model);
  void (*reset_cache)(inference_model_t *model);
  bool (*forward)(inference_model_t *model, float *logits, const int *token_ids,
                  int num_tokens);
  int (*generate)(inference_model_t *model, int *output_tokens, int max_tokens,
                  const int *input_tokens, int num_input_tokens,
                  float temperature, int top_k, float top_p);
} inference_model_ops_t;

struct inference_model {
  const inference_model_ops_t *ops;
  void *impl;
  inference_dtype_t dtype;
};

bool inference_model_load(inference_model_t *model, const char *model_type,
                          const char *model_dir, inference_dtype_t dtype);
void inference_model_free(inference_model_t *model);
void inference_model_reset_cache(inference_model_t *model);
bool inference_model_forward(inference_model_t *model, float *logits,
                             const int *token_ids, int num_tokens);
int inference_model_generate(inference_model_t *model, int *output_tokens,
                             int max_tokens, const int *input_tokens,
                             int num_input_tokens, float temperature, int top_k,
                             float top_p);

#endif

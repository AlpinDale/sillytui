#ifndef INFERENCE_MODEL_REGISTRY_H
#define INFERENCE_MODEL_REGISTRY_H

#include "inference/model/base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODEL_REGISTRY_MAX 16

typedef struct {
  const char *name;
  const inference_model_ops_t *ops;
} model_entry_t;

int model_register(const char *name, const inference_model_ops_t *ops);
const inference_model_ops_t *model_get_by_name(const char *name);
const char *model_detect_arch(const char *model_dir);
int model_registry_count(void);
int model_registry_list(const model_entry_t **out_list);

#ifdef __cplusplus
}
#endif

#endif

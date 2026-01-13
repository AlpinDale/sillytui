#ifndef INFERENCE_BACKEND_ACCELERATE_H
#define INFERENCE_BACKEND_ACCELERATE_H

#include "inference/backend/backend.h"

#ifdef __cplusplus
extern "C" {
#endif

const backend_ops_t *accelerate_backend_ops(void);

#ifdef __cplusplus
}
#endif

#endif

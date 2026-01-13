#ifndef INFERENCE_BACKEND_CPU_SCALAR_H
#define INFERENCE_BACKEND_CPU_SCALAR_H

#include "inference/backend/backend.h"

#ifdef __cplusplus
extern "C" {
#endif

const backend_ops_t *scalar_backend_ops(void);

#ifdef __cplusplus
}
#endif

#endif

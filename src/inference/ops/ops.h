/**
 * @file ops/ops.h
 * @brief Convenience header that includes all operation interfaces.
 *
 * Model implementations should include this header instead of directly
 * including kernel headers. This provides a clean abstraction layer.
 */

#ifndef INFERENCE_OPS_OPS_H
#define INFERENCE_OPS_OPS_H

#include "inference/ops/activation.h"
#include "inference/ops/embedding.h"
#include "inference/ops/gemm.h"
#include "inference/ops/kv_cache.h"
#include "inference/ops/norm.h"
#include "inference/ops/rope.h"
#include "inference/ops/sampling.h"

#endif /* INFERENCE_OPS_OPS_H */

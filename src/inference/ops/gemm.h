/**
 * @file ops/gemm.h
 * @brief High-level GEMM operations interface.
 *
 * This header provides a clean interface to matrix multiplication operations,
 * abstracting away the backend selection and kernel routing.
 *
 * Note: This header re-exports from the underlying kernel implementation.
 */

#ifndef INFERENCE_OPS_GEMM_H
#define INFERENCE_OPS_GEMM_H

/* Re-export from kernel implementation */
#include "inference/kernels/gemm/gemm.h"

#endif /* INFERENCE_OPS_GEMM_H */

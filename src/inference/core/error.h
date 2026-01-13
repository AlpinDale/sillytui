/**
 * @file error.h
 * @brief Error codes and handling for inference operations.
 */

#ifndef INFERENCE_CORE_ERROR_H
#define INFERENCE_CORE_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error codes for inference operations.
 */
typedef enum {
  INF_OK = 0,                 /**< Success */
  INF_ERR_INVALID_ARG = 1,    /**< Invalid argument */
  INF_ERR_NULL_PTR = 2,       /**< Null pointer */
  INF_ERR_OUT_OF_MEMORY = 3,  /**< Memory allocation failed */
  INF_ERR_SHAPE_MISMATCH = 4, /**< Tensor shape mismatch */
  INF_ERR_DTYPE_MISMATCH = 5, /**< Data type mismatch */
  INF_ERR_UNSUPPORTED = 6,    /**< Unsupported operation or dtype */
  INF_ERR_IO = 7,             /**< I/O error (file read/write) */
  INF_ERR_PARSE = 8,          /**< Parse error (config, weights) */
  INF_ERR_NOT_FOUND = 9,      /**< Resource not found (model, backend) */
  INF_ERR_BACKEND = 10,       /**< Backend error */
  INF_ERR_MODEL = 11,         /**< Model error */
  INF_ERR_INTERNAL = 99,      /**< Internal error */
} inf_error_t;

/**
 * Get a human-readable error message.
 * @param err Error code
 * @return Error message string
 */
const char *inf_error_str(inf_error_t err);

/**
 * Thread-local last error storage.
 * Allows operations to set an error code that callers can retrieve.
 */

/**
 * Set the last error.
 * @param err Error code to set
 */
void inf_set_error(inf_error_t err);

/**
 * Get the last error.
 * @return Last error code (or INF_OK if no error)
 */
inf_error_t inf_get_error(void);

/**
 * Clear the last error.
 */
void inf_clear_error(void);

#ifdef __cplusplus
}
#endif

#endif /* INFERENCE_CORE_ERROR_H */

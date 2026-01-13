/**
 * @file error.c
 * @brief Implementation of error handling.
 */

#include "inference/core/error.h"

/* Thread-local storage for last error */
#if defined(_MSC_VER)
static __declspec(thread) inf_error_t g_last_error = INF_OK;
#else
static _Thread_local inf_error_t g_last_error = INF_OK;
#endif

static const char *error_messages[] = {
    [INF_OK] = "success",
    [INF_ERR_INVALID_ARG] = "invalid argument",
    [INF_ERR_NULL_PTR] = "null pointer",
    [INF_ERR_OUT_OF_MEMORY] = "out of memory",
    [INF_ERR_SHAPE_MISMATCH] = "tensor shape mismatch",
    [INF_ERR_DTYPE_MISMATCH] = "data type mismatch",
    [INF_ERR_UNSUPPORTED] = "unsupported operation",
    [INF_ERR_IO] = "I/O error",
    [INF_ERR_PARSE] = "parse error",
    [INF_ERR_NOT_FOUND] = "resource not found",
    [INF_ERR_BACKEND] = "backend error",
    [INF_ERR_MODEL] = "model error",
    [INF_ERR_INTERNAL] = "internal error",
};

const char *inf_error_str(inf_error_t err) {
  if (err >= 0 && err <= INF_ERR_MODEL) {
    return error_messages[err];
  }
  if (err == INF_ERR_INTERNAL) {
    return error_messages[INF_ERR_INTERNAL];
  }
  return "unknown error";
}

void inf_set_error(inf_error_t err) { g_last_error = err; }

inf_error_t inf_get_error(void) { return g_last_error; }

void inf_clear_error(void) { g_last_error = INF_OK; }

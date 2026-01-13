/**
 * @file registry.c
 * @brief Implementation of backend registration and discovery.
 */

#include "inference/backend/registry.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

/* Registry storage */
static const backend_ops_t *g_registry[BACKEND_REGISTRY_MAX];
static int g_registry_count = 0;
static atomic_int g_initialized = 0;

/* Default backend instance */
static backend_t *g_default_backend = NULL;

/* ============================================================================
 * Registry Functions
 * ============================================================================
 */

void backend_registry_init(void) {
  if (atomic_load(&g_initialized))
    return;

  /* Initialize capabilities */
  caps_init();

  atomic_store(&g_initialized, 1);
}

int backend_register(const backend_ops_t *ops) {
  if (!ops || !ops->name)
    return -1;

  /* Ensure registry is initialized */
  backend_registry_init();

  if (g_registry_count >= BACKEND_REGISTRY_MAX)
    return -1;

  /* Check for duplicates */
  for (int i = 0; i < g_registry_count; i++) {
    if (g_registry[i] == ops || strcmp(g_registry[i]->name, ops->name) == 0) {
      return 0; /* Already registered */
    }
  }

  g_registry[g_registry_count++] = ops;
  return 0;
}

const backend_ops_t *backend_get_by_cap(cap_t cap) {
  backend_registry_init();

  for (int i = 0; i < g_registry_count; i++) {
    if (g_registry[i]->capability == cap) {
      return g_registry[i];
    }
  }
  return NULL;
}

const backend_ops_t *backend_get_by_name(const char *name) {
  if (!name)
    return NULL;

  backend_registry_init();

  for (int i = 0; i < g_registry_count; i++) {
    if (strcmp(g_registry[i]->name, name) == 0) {
      return g_registry[i];
    }
  }
  return NULL;
}

int backend_registry_count(void) {
  backend_registry_init();
  return g_registry_count;
}

int backend_registry_list(const backend_ops_t ***out_list) {
  if (!out_list)
    return 0;

  backend_registry_init();
  *out_list = g_registry;
  return g_registry_count;
}

const backend_ops_t *backend_find_best(void) {
  backend_registry_init();

  const system_caps_t *caps = caps_get();
  const backend_ops_t *best = NULL;
  int best_priority = -1;

  for (int i = 0; i < g_registry_count; i++) {
    cap_t cap = g_registry[i]->capability;

    /* Skip if capability not available */
    if (!caps->available[cap])
      continue;

    /* Check priority */
    int priority = caps->priority[cap];
    if (priority > best_priority) {
      best = g_registry[i];
      best_priority = priority;
    }
  }

  return best;
}

/* ============================================================================
 * Backend Creation
 * ============================================================================
 */

backend_t *backend_create(cap_t preferred) {
  backend_registry_init();

  const backend_ops_t *ops = NULL;

  /* Try to get requested capability */
  if (preferred != CAP_SCALAR && caps_has(preferred)) {
    ops = backend_get_by_cap(preferred);
  }

  /* Fall back to best available */
  if (!ops) {
    ops = backend_find_best();
  }

  /* Fall back to scalar */
  if (!ops) {
    ops = backend_get_by_cap(CAP_SCALAR);
  }

  if (!ops)
    return NULL;

  /* Create backend instance */
  backend_t *backend = (backend_t *)calloc(1, sizeof(backend_t));
  if (!backend)
    return NULL;

  backend->ops = ops;
  backend->num_threads = caps_num_threads();
  backend->preferred_dtype = DTYPE_F32;

  /* Initialize if ops has init function */
  if (ops->init && !ops->init(backend)) {
    free(backend);
    return NULL;
  }

  return backend;
}

backend_t *backend_create_best(void) {
  return backend_create(
      CAP_SCALAR); /* CAP_SCALAR triggers find_best fallback */
}

void backend_destroy(backend_t *backend) {
  if (!backend)
    return;

  if (backend->ops && backend->ops->destroy) {
    backend->ops->destroy(backend);
  }

  if (backend->ctx) {
    free(backend->ctx);
  }

  free(backend);
}

backend_t *backend_default(void) {
  if (!g_default_backend) {
    g_default_backend = backend_create_best();
  }
  return g_default_backend;
}

void backend_set_default(backend_t *backend) {
  if (g_default_backend && g_default_backend != backend) {
    backend_destroy(g_default_backend);
  }
  g_default_backend = backend;
}

void backend_set_num_threads(backend_t *backend, int num_threads) {
  if (!backend)
    return;

  if (num_threads <= 0) {
    backend->num_threads = caps_num_threads();
  } else {
    backend->num_threads = num_threads;
  }
}

int backend_get_num_threads(const backend_t *backend) {
  if (!backend)
    return 1;
  return backend->num_threads;
}

/* ============================================================================
 * Convenience Wrappers
 * ============================================================================
 */

void backend_gemm(const tensor_t *A, const tensor_t *B, tensor_t *C,
                  bool transpose_A, bool transpose_B) {
  backend_t *backend = backend_default();
  if (backend && backend->ops && backend->ops->gemm) {
    backend->ops->gemm(backend, A, B, C, transpose_A, transpose_B);
  }
}

void backend_rms_norm(tensor_t *out, const tensor_t *in, const tensor_t *weight,
                      float eps) {
  backend_t *backend = backend_default();
  if (backend && backend->ops && backend->ops->rms_norm) {
    backend->ops->rms_norm(backend, out, in, weight, eps);
  }
}

void backend_silu(tensor_t *out, const tensor_t *in) {
  backend_t *backend = backend_default();
  if (backend && backend->ops && backend->ops->silu) {
    backend->ops->silu(backend, out, in);
  }
}

void backend_softmax(tensor_t *out, const tensor_t *in, int axis) {
  backend_t *backend = backend_default();
  if (backend && backend->ops && backend->ops->softmax) {
    backend->ops->softmax(backend, out, in, axis);
  }
}

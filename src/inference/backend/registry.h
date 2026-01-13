/**
 * @file registry.h
 * @brief Backend registration and discovery.
 *
 * Provides a registry for backend implementations that allows
 * dynamic registration (e.g., via constructors) and lookup.
 */

#ifndef INFERENCE_BACKEND_REGISTRY_H
#define INFERENCE_BACKEND_REGISTRY_H

#include "inference/backend/backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of registered backends */
#define BACKEND_REGISTRY_MAX 16

/**
 * Register a backend implementation.
 * Called by backend implementations at startup (via
 * __attribute__((constructor))).
 *
 * @param ops Backend operations vtable
 * @return 0 on success, -1 if registry is full
 */
int backend_register(const backend_ops_t *ops);

/**
 * Get a backend by capability.
 *
 * @param cap Capability to look up
 * @return Backend ops, or NULL if not found
 */
const backend_ops_t *backend_get_by_cap(cap_t cap);

/**
 * Get a backend by name.
 *
 * @param name Backend name
 * @return Backend ops, or NULL if not found
 */
const backend_ops_t *backend_get_by_name(const char *name);

/**
 * Get the number of registered backends.
 *
 * @return Number of backends
 */
int backend_registry_count(void);

/**
 * Get all registered backends.
 *
 * @param out_list Output pointer to array of backend ops
 * @return Number of backends
 */
int backend_registry_list(const backend_ops_t ***out_list);

/**
 * Find the best available backend.
 * Considers capabilities and priorities.
 *
 * @return Best backend ops, or NULL if none registered
 */
const backend_ops_t *backend_find_best(void);

/**
 * Initialize the backend registry.
 * Called automatically on first use.
 */
void backend_registry_init(void);

#ifdef __cplusplus
}
#endif

#endif /* INFERENCE_BACKEND_REGISTRY_H */

#include "core/fault.h"

#ifdef SILLYTUI_FAULT_INJECTION

#include <stdlib.h>
#include <string.h>

FaultConfig g_malloc_fault = {0};
FaultConfig g_io_fault = {0};

#if defined(SILLYTUI_COVERAGE_TEST) || defined(SILLYTUI_DEBUG)
unsigned int g_coverage_counter = 0;
#endif

void fault_init(void) {
  memset(&g_malloc_fault, 0, sizeof(g_malloc_fault));
  memset(&g_io_fault, 0, sizeof(g_io_fault));
}

static bool fault_step(FaultConfig *cfg) {
  if (!cfg->enabled) {
    cfg->ok_after++;
    return false;
  }
  if (cfg->countdown > 0) {
    cfg->countdown--;
    cfg->ok_before++;
    return false;
  }
  cfg->fail_count++;
  cfg->repeat--;
  if (cfg->repeat <= 0) {
    cfg->enabled = false;
  }
  return true;
}

void fault_enable_malloc(int countdown, int repeat) {
  g_malloc_fault.countdown = countdown;
  g_malloc_fault.repeat = repeat;
  g_malloc_fault.enabled = true;
  g_malloc_fault.fail_count = 0;
  g_malloc_fault.ok_before = 0;
  g_malloc_fault.ok_after = 0;
}

void fault_disable_malloc(void) { g_malloc_fault.enabled = false; }

bool fault_should_fail_malloc(void) { return fault_step(&g_malloc_fault); }

int fault_get_malloc_fail_count(void) { return g_malloc_fault.fail_count; }

void fault_enable_io(int countdown, int repeat) {
  g_io_fault.countdown = countdown;
  g_io_fault.repeat = repeat;
  g_io_fault.enabled = true;
  g_io_fault.fail_count = 0;
  g_io_fault.ok_before = 0;
  g_io_fault.ok_after = 0;
}

void fault_disable_io(void) { g_io_fault.enabled = false; }

bool fault_should_fail_io(void) { return fault_step(&g_io_fault); }

int fault_get_io_fail_count(void) { return g_io_fault.fail_count; }

void fault_begin_benign(void) {
  g_malloc_fault.is_benign = true;
  g_io_fault.is_benign = true;
}

void fault_end_benign(void) {
  g_malloc_fault.is_benign = false;
  g_io_fault.is_benign = false;
}

void *fault_malloc(size_t size) {
  if (fault_should_fail_malloc()) {
    return NULL;
  }
  return malloc(size);
}

void *fault_realloc(void *ptr, size_t size) {
  if (fault_should_fail_malloc()) {
    return NULL;
  }
  return realloc(ptr, size);
}

void *fault_calloc(size_t count, size_t size) {
  if (fault_should_fail_malloc()) {
    return NULL;
  }
  return calloc(count, size);
}

char *fault_strdup(const char *s) {
  if (fault_should_fail_malloc()) {
    return NULL;
  }
  return strdup(s);
}

void fault_free(void *ptr) { free(ptr); }

#endif

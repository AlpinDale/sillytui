#ifndef FAULT_H
#define FAULT_H

#include <stdbool.h>
#include <stddef.h>

#ifdef SILLYTUI_FAULT_INJECTION

typedef struct {
  int countdown;
  int repeat;
  int fail_count;
  int ok_before;
  int ok_after;
  bool enabled;
  bool is_benign;
} FaultConfig;

extern FaultConfig g_malloc_fault;
extern FaultConfig g_io_fault;

void fault_init(void);

void fault_enable_malloc(int countdown, int repeat);
void fault_disable_malloc(void);
bool fault_should_fail_malloc(void);
int fault_get_malloc_fail_count(void);

void fault_enable_io(int countdown, int repeat);
void fault_disable_io(void);
bool fault_should_fail_io(void);
int fault_get_io_fail_count(void);

void fault_begin_benign(void);
void fault_end_benign(void);

void *fault_malloc(size_t size);
void *fault_realloc(void *ptr, size_t size);
void *fault_calloc(size_t count, size_t size);
char *fault_strdup(const char *s);
void fault_free(void *ptr);

#define MALLOC(size) fault_malloc(size)
#define REALLOC(ptr, size) fault_realloc(ptr, size)
#define CALLOC(count, size) fault_calloc(count, size)
#define STRDUP(s) fault_strdup(s)
#define FREE(ptr) fault_free(ptr)

#else

#include <stdlib.h>
#include <string.h>

#define fault_init()
#define fault_enable_malloc(c, r)
#define fault_disable_malloc()
#define fault_should_fail_malloc() false
#define fault_get_malloc_fail_count() 0
#define fault_enable_io(c, r)
#define fault_disable_io()
#define fault_should_fail_io() false
#define fault_get_io_fail_count() 0
#define fault_begin_benign()
#define fault_end_benign()

#define MALLOC(size) malloc(size)
#define REALLOC(ptr, size) realloc(ptr, size)
#define CALLOC(count, size) calloc(count, size)
#define STRDUP(s) strdup(s)
#define FREE(ptr) free(ptr)

#endif

#endif

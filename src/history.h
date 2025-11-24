#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

typedef struct {
  char **items;
  size_t count;
  size_t capacity;
} ChatHistory;

void history_init(ChatHistory *history);
void history_free(ChatHistory *history);
size_t history_add(ChatHistory *history, const char *message);
void history_update(ChatHistory *history, size_t index, const char *message);

#endif

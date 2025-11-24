#include "history.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char *dup_string(const char *source) {
  size_t len = strlen(source) + 1;
  char *copy = malloc(len);
  if (copy) {
    memcpy(copy, source, len);
  }
  return copy;
}

void history_init(ChatHistory *history) {
  history->items = NULL;
  history->count = 0;
  history->capacity = 0;
}

void history_free(ChatHistory *history) {
  if (!history) {
    return;
  }
  for (size_t i = 0; i < history->count; ++i) {
    free(history->items[i]);
  }
  free(history->items);
  history->items = NULL;
  history->count = 0;
  history->capacity = 0;
}

size_t history_add(ChatHistory *history, const char *message) {
  if (history->count == history->capacity) {
    size_t new_capacity = history->capacity == 0 ? 8 : history->capacity * 2;
    char **tmp = realloc(history->items, new_capacity * sizeof(char *));
    if (!tmp) {
      return SIZE_MAX;
    }
    history->items = tmp;
    history->capacity = new_capacity;
  }
  history->items[history->count] = dup_string(message);
  if (!history->items[history->count]) {
    return SIZE_MAX;
  }
  history->count += 1;
  return history->count - 1;
}

void history_update(ChatHistory *history, size_t index, const char *message) {
  if (index >= history->count) {
    return;
  }
  char *copy = dup_string(message);
  if (!copy) {
    return;
  }
  free(history->items[index]);
  history->items[index] = copy;
}

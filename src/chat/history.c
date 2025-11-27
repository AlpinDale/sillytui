#include "chat/history.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char *dup_string(const char *source) {
  if (!source)
    return NULL;
  size_t len = strlen(source) + 1;
  char *copy = malloc(len);
  if (copy) {
    memcpy(copy, source, len);
  }
  return copy;
}

static void message_init(ChatMessage *msg) {
  msg->swipes = NULL;
  msg->swipe_count = 0;
  msg->active_swipe = 0;
  msg->token_counts = NULL;
  msg->gen_times = NULL;
  msg->output_tps = NULL;
}

static void message_free(ChatMessage *msg) {
  if (!msg)
    return;
  for (size_t i = 0; i < msg->swipe_count; i++) {
    free(msg->swipes[i]);
  }
  free(msg->swipes);
  free(msg->token_counts);
  free(msg->gen_times);
  free(msg->output_tps);
  msg->swipes = NULL;
  msg->token_counts = NULL;
  msg->gen_times = NULL;
  msg->output_tps = NULL;
  msg->swipe_count = 0;
  msg->active_swipe = 0;
}

void history_init(ChatHistory *history) {
  if (!history)
    return;
  history->messages = NULL;
  history->count = 0;
  history->capacity = 0;
}

void history_free(ChatHistory *history) {
  if (!history)
    return;
  for (size_t i = 0; i < history->count; i++) {
    message_free(&history->messages[i]);
  }
  free(history->messages);
  history->messages = NULL;
  history->count = 0;
  history->capacity = 0;
}

size_t history_add(ChatHistory *history, const char *message) {
  if (!history)
    return SIZE_MAX;
  if (history->count == history->capacity) {
    size_t new_capacity = history->capacity == 0 ? 8 : history->capacity * 2;
    ChatMessage *tmp =
        realloc(history->messages, new_capacity * sizeof(ChatMessage));
    if (!tmp)
      return SIZE_MAX;
    history->messages = tmp;
    history->capacity = new_capacity;
  }

  ChatMessage *msg = &history->messages[history->count];
  message_init(msg);

  msg->swipes = malloc(sizeof(char *));
  msg->token_counts = malloc(sizeof(int));
  msg->gen_times = malloc(sizeof(double));
  msg->output_tps = malloc(sizeof(double));
  if (!msg->swipes || !msg->token_counts || !msg->gen_times ||
      !msg->output_tps) {
    free(msg->swipes);
    free(msg->token_counts);
    free(msg->gen_times);
    free(msg->output_tps);
    return SIZE_MAX;
  }

  msg->swipes[0] = dup_string(message);
  if (!msg->swipes[0]) {
    free(msg->swipes);
    free(msg->token_counts);
    free(msg->gen_times);
    free(msg->output_tps);
    return SIZE_MAX;
  }
  msg->token_counts[0] = 0;
  msg->gen_times[0] = 0.0;
  msg->output_tps[0] = 0.0;
  msg->swipe_count = 1;
  msg->active_swipe = 0;

  history->count++;
  return history->count - 1;
}

void history_update(ChatHistory *history, size_t index, const char *message) {
  if (!history || index >= history->count)
    return;

  ChatMessage *msg = &history->messages[index];
  if (msg->swipe_count == 0)
    return;

  char *copy = dup_string(message);
  if (!copy)
    return;

  free(msg->swipes[msg->active_swipe]);
  msg->swipes[msg->active_swipe] = copy;
}

const char *history_get(const ChatHistory *history, size_t index) {
  if (!history || index >= history->count)
    return NULL;

  ChatMessage *msg = &history->messages[index];
  if (msg->swipe_count == 0 || msg->active_swipe >= msg->swipe_count)
    return NULL;

  return msg->swipes[msg->active_swipe];
}

const char *history_get_swipe(const ChatHistory *history, size_t msg_index,
                              size_t swipe_index) {
  if (!history || msg_index >= history->count)
    return NULL;

  ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count)
    return NULL;

  return msg->swipes[swipe_index];
}

void history_update_swipe(ChatHistory *history, size_t msg_index,
                          size_t swipe_index, const char *message) {
  if (!history || msg_index >= history->count)
    return;

  ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count)
    return;

  char *copy = dup_string(message);
  if (!copy)
    return;

  free(msg->swipes[swipe_index]);
  msg->swipes[swipe_index] = copy;
}

size_t history_add_swipe(ChatHistory *history, size_t msg_index,
                         const char *content) {
  if (!history || msg_index >= history->count)
    return SIZE_MAX;

  ChatMessage *msg = &history->messages[msg_index];
  if (msg->swipe_count >= MAX_SWIPES)
    return SIZE_MAX;

  char **new_swipes =
      realloc(msg->swipes, (msg->swipe_count + 1) * sizeof(char *));
  int *new_tokens =
      realloc(msg->token_counts, (msg->swipe_count + 1) * sizeof(int));
  double *new_times =
      realloc(msg->gen_times, (msg->swipe_count + 1) * sizeof(double));
  double *new_tps =
      realloc(msg->output_tps, (msg->swipe_count + 1) * sizeof(double));
  if (!new_swipes || !new_tokens || !new_times || !new_tps) {
    if (new_swipes)
      msg->swipes = new_swipes;
    if (new_tokens)
      msg->token_counts = new_tokens;
    if (new_times)
      msg->gen_times = new_times;
    if (new_tps)
      msg->output_tps = new_tps;
    return SIZE_MAX;
  }
  msg->swipes = new_swipes;
  msg->token_counts = new_tokens;
  msg->gen_times = new_times;
  msg->output_tps = new_tps;

  msg->swipes[msg->swipe_count] = dup_string(content);
  if (!msg->swipes[msg->swipe_count])
    return SIZE_MAX;

  msg->token_counts[msg->swipe_count] = 0;
  msg->gen_times[msg->swipe_count] = 0.0;
  msg->output_tps[msg->swipe_count] = 0.0;
  msg->swipe_count++;
  msg->active_swipe = msg->swipe_count - 1;
  return msg->active_swipe;
}

bool history_set_active_swipe(ChatHistory *history, size_t msg_index,
                              size_t swipe_index) {
  if (!history || msg_index >= history->count)
    return false;

  ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count)
    return false;

  msg->active_swipe = swipe_index;
  return true;
}

size_t history_get_swipe_count(const ChatHistory *history, size_t msg_index) {
  if (!history || msg_index >= history->count)
    return 0;
  return history->messages[msg_index].swipe_count;
}

size_t history_get_active_swipe(const ChatHistory *history, size_t msg_index) {
  if (!history || msg_index >= history->count)
    return 0;
  return history->messages[msg_index].active_swipe;
}

bool history_delete(ChatHistory *history, size_t index) {
  if (!history || index >= history->count)
    return false;

  ChatMessage *msg = &history->messages[index];
  for (size_t i = 0; i < msg->swipe_count; i++) {
    free(msg->swipes[i]);
  }
  free(msg->swipes);
  free(msg->token_counts);
  free(msg->gen_times);
  free(msg->output_tps);

  for (size_t i = index; i < history->count - 1; i++) {
    history->messages[i] = history->messages[i + 1];
  }
  history->count--;

  return true;
}

void history_set_token_count(ChatHistory *history, size_t msg_index,
                             size_t swipe_index, int tokens) {
  if (!history || msg_index >= history->count)
    return;
  ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count || !msg->token_counts)
    return;
  msg->token_counts[swipe_index] = tokens;
}

int history_get_token_count(const ChatHistory *history, size_t msg_index,
                            size_t swipe_index) {
  if (!history || msg_index >= history->count)
    return 0;
  const ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count || !msg->token_counts)
    return 0;
  return msg->token_counts[swipe_index];
}

void history_set_gen_time(ChatHistory *history, size_t msg_index,
                          size_t swipe_index, double time_ms) {
  if (!history || msg_index >= history->count)
    return;
  ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count || !msg->gen_times)
    return;
  msg->gen_times[swipe_index] = time_ms;
}

double history_get_gen_time(const ChatHistory *history, size_t msg_index,
                            size_t swipe_index) {
  if (!history || msg_index >= history->count)
    return 0.0;
  const ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count || !msg->gen_times)
    return 0.0;
  return msg->gen_times[swipe_index];
}

void history_set_output_tps(ChatHistory *history, size_t msg_index,
                            size_t swipe_index, double tps) {
  if (!history || msg_index >= history->count)
    return;
  ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count || !msg->output_tps)
    return;
  msg->output_tps[swipe_index] = tps;
}

double history_get_output_tps(const ChatHistory *history, size_t msg_index,
                              size_t swipe_index) {
  if (!history || msg_index >= history->count)
    return 0.0;
  const ChatMessage *msg = &history->messages[msg_index];
  if (swipe_index >= msg->swipe_count || !msg->output_tps)
    return 0.0;
  return msg->output_tps[swipe_index];
}

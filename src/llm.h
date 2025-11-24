#ifndef LLM_H
#define LLM_H

#include "config.h"
#include "history.h"
#include <stdbool.h>
#include <stddef.h>

typedef void (*LLMStreamCallback)(const char *chunk, void *userdata);
typedef void (*LLMProgressCallback)(void *userdata);

typedef struct {
  char *content;
  size_t len;
  size_t cap;
  bool success;
  char error[256];
} LLMResponse;

void llm_init(void);
void llm_cleanup(void);

LLMResponse llm_chat(const ModelConfig *config, const ChatHistory *history,
                     LLMStreamCallback stream_cb,
                     LLMProgressCallback progress_cb, void *userdata);

void llm_response_free(LLMResponse *resp);

#endif

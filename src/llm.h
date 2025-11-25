#ifndef LLM_H
#define LLM_H

#include "character.h"
#include "config.h"
#include "history.h"
#include "persona.h"
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

typedef struct {
  const CharacterCard *character;
  const Persona *persona;
} LLMContext;

void llm_init(void);
void llm_cleanup(void);

LLMResponse llm_chat(const ModelConfig *config, const ChatHistory *history,
                     const LLMContext *context, LLMStreamCallback stream_cb,
                     LLMProgressCallback progress_cb, void *userdata);

void llm_response_free(LLMResponse *resp);

#endif

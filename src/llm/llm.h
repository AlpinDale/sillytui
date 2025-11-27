#ifndef LLM_H
#define LLM_H

#include "character/character.h"
#include "character/persona.h"
#include "chat/history.h"
#include "core/config.h"
#include "llm/sampler.h"
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
  int prompt_tokens;
  int completion_tokens;
  double elapsed_ms;
  double output_tps;
} LLMResponse;

typedef struct {
  const CharacterCard *character;
  const Persona *persona;
  const SamplerSettings *samplers;
} LLMContext;

void llm_init(void);
void llm_cleanup(void);

int llm_estimate_tokens(const char *text);
int llm_tokenize(const ModelConfig *config, const char *text);

LLMResponse llm_chat(const ModelConfig *config, const ChatHistory *history,
                     const LLMContext *context, LLMStreamCallback stream_cb,
                     LLMProgressCallback progress_cb, void *userdata);

void llm_response_free(LLMResponse *resp);

#endif

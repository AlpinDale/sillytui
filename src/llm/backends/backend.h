#ifndef LLM_BACKEND_H
#define LLM_BACKEND_H

#include "character/character.h"
#include "character/persona.h"
#include "chat/author_note.h"
#include "chat/history.h"
#include "core/config.h"
#include "llm/sampler.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
  const char *content;
  const char *role;
} ExampleMessage;

typedef struct {
  const CharacterCard *character;
  const Persona *persona;
  const SamplerSettings *samplers;
  const AuthorNote *author_note;
} LLMContext;

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

typedef void (*LLMStreamCallback)(const char *chunk, void *userdata);
typedef void (*LLMProgressCallback)(void *userdata);

typedef struct {
  LLMResponse *resp;
  LLMStreamCallback cb;
  LLMProgressCallback progress_cb;
  void *userdata;
  char line_buffer[4096];
  size_t line_len;
  bool got_content;
  int prompt_tokens;
  int completion_tokens;
  bool is_anthropic;
} StreamCtx;

typedef struct LLMBackend {
  int (*tokenize)(const ModelConfig *config, const char *text);

  char *(*build_request)(const ModelConfig *config, const ChatHistory *history,
                         const LLMContext *context);

  void (*parse_stream)(StreamCtx *ctx, const char *line);

  void (*add_headers)(void *curl, const ModelConfig *config);
} LLMBackend;

const LLMBackend *backend_get(ApiType type);

extern const LLMBackend backend_openai;
extern const LLMBackend backend_anthropic;
extern const LLMBackend backend_kobold;

#endif

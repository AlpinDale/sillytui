#ifndef LLM_BACKEND_H
#define LLM_BACKEND_H

#include "character/character.h"
#include "character/persona.h"
#include "chat/author_note.h"
#include "chat/history.h"
#include "core/config.h"
#include "llm/sampler.h"
#include "lore/lorebook.h"
#include "tokenizer/selector.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/time.h>

typedef struct {
  const char *content;
  const char *role;
} ExampleMessage;

typedef struct {
  const CharacterCard *character;
  const Persona *persona;
  const SamplerSettings *samplers;
  const AuthorNote *author_note;
  const Lorebook *lorebook;
  ChatTokenizer *tokenizer;
} LLMContext;

typedef struct {
  char *content;
  size_t len;
  size_t cap;
  char *reasoning;
  size_t reasoning_len;
  size_t reasoning_cap;
  bool success;
  char error[256];
  int prompt_tokens;
  int completion_tokens;
  int reasoning_tokens;
  double elapsed_ms;
  double output_tps;
  double reasoning_ms;
  char finish_reason[32];
} LLMResponse;

typedef void (*LLMStreamCallback)(const char *chunk, void *userdata);
typedef void (*LLMReasoningCallback)(const char *chunk, double elapsed_ms,
                                     void *userdata);
typedef void (*LLMProgressCallback)(void *userdata);

typedef struct {
  LLMResponse *resp;
  LLMStreamCallback cb;
  LLMReasoningCallback reasoning_cb;
  LLMProgressCallback progress_cb;
  void *userdata;
  char line_buffer[4096];
  size_t line_len;
  bool got_content;
  bool in_reasoning;
  int prompt_tokens;
  int completion_tokens;
  bool is_anthropic;
#ifndef _WIN32
  struct timeval first_token_time;
  struct timeval last_token_time;
  struct timeval reasoning_start_time;
#endif
  bool has_first_token;
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

static const char *ANTHROPIC_MODELS[] = {
    "claude-sonnet-4-5",
    "claude-sonnet-4-5-20250929",
    "claude-haiku-4-5",
    "claude-haiku-4-5-20251001",
    "claude-opus-4-1",
    "claude-opus-4-1-20250805",
    "claude-opus-4-0",
    "claude-opus-4-20250514",
    "claude-sonnet-4-0",
    "claude-sonnet-4-20250514",
    "claude-3-7-sonnet-latest",
    "claude-3-7-sonnet-20250219",
    "claude-3-5-sonnet-latest",
    "claude-3-5-sonnet-20241022",
    "claude-3-5-sonnet-20240620",
    "claude-3-5-haiku-latest",
    "claude-3-5-haiku-20241022",
    "claude-3-opus-20240229",
    "claude-3-haiku-20240307",
};

static const size_t ANTHROPIC_MODELS_COUNT =
    sizeof(ANTHROPIC_MODELS) / sizeof(ANTHROPIC_MODELS[0]);

static inline const char **anthropic_get_models(size_t *count) {
  if (count)
    *count = ANTHROPIC_MODELS_COUNT;
  return ANTHROPIC_MODELS;
}

#endif

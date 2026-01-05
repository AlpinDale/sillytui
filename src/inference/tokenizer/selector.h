#ifndef TOKENIZER_SELECTOR_H
#define TOKENIZER_SELECTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  TOKENIZER_API = 0,
  TOKENIZER_OPENAI_LEGACY,
  TOKENIZER_OPENAI_NEW,
  TOKENIZER_DEEPSEEK,
  TOKENIZER_GLM4,
  TOKENIZER_LLAMA3,
  TOKENIZER_QWEN3,
  TOKENIZER_COUNT
} TokenizerSelection;

typedef struct {
  TokenizerSelection selection;
  bool loaded;
  void *instance;
} ChatTokenizer;

typedef struct {
  uint32_t *ids;
  size_t *offsets;
  size_t count;
  size_t cap;
} TokenResult;

void token_result_init(TokenResult *tr);
void token_result_free(TokenResult *tr);

void chat_tokenizer_init(ChatTokenizer *ct);
void chat_tokenizer_free(ChatTokenizer *ct);
bool chat_tokenizer_set(ChatTokenizer *ct, TokenizerSelection sel);
int chat_tokenizer_count(ChatTokenizer *ct, const char *text);
bool chat_tokenizer_is_api(ChatTokenizer *ct);
int chat_tokenizer_encode(ChatTokenizer *ct, const char *text,
                          TokenResult *out);

const char *tokenizer_selection_name(TokenizerSelection sel);
const char *tokenizer_selection_description(TokenizerSelection sel);
TokenizerSelection tokenizer_selection_from_name(const char *name);

#endif

#include "selector.h"
#include "gpt2bpe.h"
#include "simd.h"
#include "tiktoken.h"
#include <stdlib.h>
#include <string.h>

typedef enum { TYPE_TIKTOKEN, TYPE_GPT2BPE } InternalType;

typedef struct {
  TokenizerSelection sel;
  const char *name;
  const char *description;
  InternalType type;
  const char *path1;
  const char *path2;
} TokenizerDef;

static const TokenizerDef TOKENIZER_DEFS[] = {
    {TOKENIZER_API, "api", "Use API tokenization (Aphrodite/Anthropic)", 0,
     NULL, NULL},
    {TOKENIZER_OPENAI_LEGACY, "openai", "OpenAI cl100k (GPT-4, GPT-3.5)",
     TYPE_TIKTOKEN, "tokenizers/openai/cl100k_base.tiktoken", NULL},
    {TOKENIZER_OPENAI_NEW, "openai-o200k", "OpenAI o200k (GPT-4o)",
     TYPE_TIKTOKEN, "tokenizers/openai/o200k_base.tiktoken", NULL},
    {TOKENIZER_DEEPSEEK, "deepseek", "DeepSeek R1 (128k vocab)", TYPE_GPT2BPE,
     "tokenizers/deepseek-r1/vocab.json", "tokenizers/deepseek-r1/merges.txt"},
    {TOKENIZER_GLM4, "glm4", "GLM-4.5 (151k vocab)", TYPE_GPT2BPE,
     "tokenizers/glm4/vocab.json", "tokenizers/glm4/merges.txt"},
    {TOKENIZER_LLAMA3, "llama3", "Llama 3 / 3.1 (128k vocab)", TYPE_GPT2BPE,
     "tokenizers/llama3/vocab.json", "tokenizers/llama3/merges.txt"},
    {TOKENIZER_QWEN3, "qwen3", "Qwen 3 (151k vocab)", TYPE_GPT2BPE,
     "tokenizers/qwen3/vocab.json", "tokenizers/qwen3/merges.txt"},
};

static bool g_simd_initialized = false;

void token_result_init(TokenResult *tr) {
  if (!tr)
    return;
  tr->ids = NULL;
  tr->offsets = NULL;
  tr->count = 0;
  tr->cap = 0;
}

void token_result_free(TokenResult *tr) {
  if (!tr)
    return;
  free(tr->ids);
  free(tr->offsets);
  tr->ids = NULL;
  tr->offsets = NULL;
  tr->count = 0;
  tr->cap = 0;
}

void chat_tokenizer_init(ChatTokenizer *ct) {
  if (!ct)
    return;
  ct->selection = TOKENIZER_API;
  ct->loaded = false;
  ct->instance = NULL;
}

void chat_tokenizer_free(ChatTokenizer *ct) {
  if (!ct || !ct->instance)
    return;
  const TokenizerDef *def = &TOKENIZER_DEFS[ct->selection];
  if (def->path1) {
    if (def->type == TYPE_TIKTOKEN) {
      tokenizer_free((Tokenizer *)ct->instance);
    } else if (def->type == TYPE_GPT2BPE) {
      gpt2_free((GPT2BPETokenizer *)ct->instance);
    }
    free(ct->instance);
  }
  ct->instance = NULL;
  ct->loaded = false;
}

bool chat_tokenizer_set(ChatTokenizer *ct, TokenizerSelection sel) {
  if (!ct || sel >= TOKENIZER_COUNT)
    return false;
  chat_tokenizer_free(ct);
  ct->selection = sel;
  if (sel == TOKENIZER_API) {
    ct->loaded = true;
    return true;
  }
  if (!g_simd_initialized) {
    simd_init();
    g_simd_initialized = true;
  }
  const TokenizerDef *def = &TOKENIZER_DEFS[sel];
  if (!def->path1)
    return false;
  if (def->type == TYPE_TIKTOKEN) {
    Tokenizer *tok = malloc(sizeof(Tokenizer));
    if (!tok)
      return false;
    tokenizer_init(tok);
    if (!tokenizer_load_tiktoken(tok, def->path1)) {
      free(tok);
      return false;
    }
    ct->instance = tok;
    ct->loaded = true;
    return true;
  } else if (def->type == TYPE_GPT2BPE) {
    GPT2BPETokenizer *tok = malloc(sizeof(GPT2BPETokenizer));
    if (!tok)
      return false;
    gpt2_init(tok);
    if (!gpt2_load(tok, def->path1, def->path2)) {
      free(tok);
      return false;
    }
    ct->instance = tok;
    ct->loaded = true;
    return true;
  }
  return false;
}

int chat_tokenizer_count(ChatTokenizer *ct, const char *text) {
  if (!ct || !text)
    return -1;
  if (!ct->loaded || ct->selection == TOKENIZER_API)
    return -1;
  uint32_t tokens[8192];
  const TokenizerDef *def = &TOKENIZER_DEFS[ct->selection];
  if (def->type == TYPE_TIKTOKEN) {
    return tokenizer_encode((Tokenizer *)ct->instance, text, tokens, 8192);
  } else if (def->type == TYPE_GPT2BPE) {
    return gpt2_encode((GPT2BPETokenizer *)ct->instance, text, tokens, 8192);
  }
  return -1;
}

bool chat_tokenizer_is_api(ChatTokenizer *ct) {
  return ct && ct->selection == TOKENIZER_API;
}

int chat_tokenizer_encode(ChatTokenizer *ct, const char *text,
                          TokenResult *out) {
  if (!ct || !text || !out)
    return -1;
  if (!ct->loaded || ct->selection == TOKENIZER_API)
    return -1;

  uint32_t tokens[8192];
  int count = 0;
  const TokenizerDef *def = &TOKENIZER_DEFS[ct->selection];

  if (def->type == TYPE_TIKTOKEN) {
    count = tokenizer_encode((Tokenizer *)ct->instance, text, tokens, 8192);
  } else if (def->type == TYPE_GPT2BPE) {
    count = gpt2_encode((GPT2BPETokenizer *)ct->instance, text, tokens, 8192);
  }

  if (count <= 0) {
    out->count = 0;
    return 0;
  }

  if ((size_t)count > out->cap) {
    free(out->ids);
    free(out->offsets);
    out->cap = count + 64;
    out->ids = malloc(out->cap * sizeof(uint32_t));
    out->offsets = malloc((out->cap + 1) * sizeof(size_t));
    if (!out->ids || !out->offsets) {
      token_result_free(out);
      return -1;
    }
  }

  out->count = count;
  memcpy(out->ids, tokens, count * sizeof(uint32_t));

  size_t offset = 0;
  for (int i = 0; i < count; i++) {
    out->offsets[i] = offset;
    char *decoded = NULL;
    if (def->type == TYPE_TIKTOKEN) {
      decoded = tokenizer_decode((Tokenizer *)ct->instance, &tokens[i], 1);
    } else if (def->type == TYPE_GPT2BPE) {
      decoded = gpt2_decode((GPT2BPETokenizer *)ct->instance, &tokens[i], 1);
    }
    if (decoded) {
      offset += strlen(decoded);
      free(decoded);
    }
  }
  out->offsets[count] = offset;

  return count;
}

const char *tokenizer_selection_name(TokenizerSelection sel) {
  if (sel >= TOKENIZER_COUNT)
    return "unknown";
  return TOKENIZER_DEFS[sel].name;
}

const char *tokenizer_selection_description(TokenizerSelection sel) {
  if (sel >= TOKENIZER_COUNT)
    return "Unknown tokenizer";
  return TOKENIZER_DEFS[sel].description;
}

TokenizerSelection tokenizer_selection_from_name(const char *name) {
  if (!name)
    return TOKENIZER_API;
  for (int i = 0; i < TOKENIZER_COUNT; i++) {
    if (strcmp(TOKENIZER_DEFS[i].name, name) == 0) {
      return (TokenizerSelection)i;
    }
  }
  return TOKENIZER_API;
}

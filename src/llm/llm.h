#ifndef LLM_H
#define LLM_H

#include "llm/backends/backend.h"

void llm_init(void);
void llm_cleanup(void);

int llm_estimate_tokens(const char *text);
int llm_tokenize(const ModelConfig *config, const char *text);

LLMResponse llm_chat(const ModelConfig *config, const ChatHistory *history,
                     const LLMContext *context, LLMStreamCallback stream_cb,
                     LLMReasoningCallback reasoning_cb,
                     LLMProgressCallback progress_cb, void *userdata);

void llm_response_free(LLMResponse *resp);

#endif

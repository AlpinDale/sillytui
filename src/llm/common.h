#ifndef LLM_COMMON_H
#define LLM_COMMON_H

#include "llm/backends/backend.h"
#include <curl/curl.h>
#include <stddef.h>

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} StringBuilder;

void sb_init(StringBuilder *sb);
void sb_append(StringBuilder *sb, const char *str);
char *sb_finish(StringBuilder *sb);
void sb_free(StringBuilder *sb);

char *escape_json_string(const char *str);
char *find_json_string(const char *json, const char *key);
int find_json_int(const char *json, const char *key);

char *build_system_prompt(const LLMContext *context);

ExampleMessage *parse_mes_example(const char *mes_example, size_t *count,
                                  const char *char_name, const char *user_name);
void free_example_messages(ExampleMessage *messages, size_t count);

int count_tokens(const ModelConfig *config, const char *text);
int count_tokens_with_tokenizer(ChatTokenizer *tokenizer,
                                const ModelConfig *config, const char *text);
void set_current_tokenizer(ChatTokenizer *tokenizer);

void append_to_response(LLMResponse *resp, const char *data, size_t len);
void append_to_reasoning(LLMResponse *resp, const char *data, size_t len);

size_t stream_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal, curl_off_t ulnow);

typedef struct {
  char **body;
  size_t *pos;
  size_t *cap;
} RequestBuilder;

bool ensure_buffer_capacity(RequestBuilder *rb, size_t needed);
void add_message_to_json(RequestBuilder *rb, bool *first, const char *role, const char *content);
size_t calculate_history_start_index(const ModelConfig *config, const ChatHistory *history, 
                                     int available_tokens);
void process_history_message(const char *msg, MessageRole msg_role, 
                             const char *char_name, const char *user_name,
                             char **out_content);
void add_sampler_settings_openai_compatible(RequestBuilder *rb, const ModelConfig *config,
                                             const SamplerSettings *s);
void add_sampler_settings_anthropic(RequestBuilder *rb, const SamplerSettings *s);
void add_sampler_settings_kobold(RequestBuilder *rb, const SamplerSettings *s);

#endif

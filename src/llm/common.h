#ifndef LLM_COMMON_H
#define LLM_COMMON_H

#include "llm/backends/backend.h"
#include <stddef.h>
#include <stdint.h>

#ifdef CURL_FOUND
#include <curl/curl.h>
typedef curl_off_t llm_off_t;
#else
typedef int64_t llm_off_t;
#endif

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

char *expand_attachments(const char *content);
char *base64_encode(const unsigned char *data, size_t input_length);
char *get_file_extension(const char *filename);
const char *get_image_mime_type(const char *extension);
bool is_image_file(const char *filename);

void append_to_response(LLMResponse *resp, const char *data, size_t len);
void append_to_reasoning(LLMResponse *resp, const char *data, size_t len);

size_t stream_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
int progress_callback(void *clientp, llm_off_t dltotal, llm_off_t dlnow,
                      llm_off_t ultotal, llm_off_t ulnow);

#endif

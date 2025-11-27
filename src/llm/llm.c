#include "llm/llm.h"
#include "core/macros.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static char *escape_json_string(const char *str);

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
  struct timeval first_token_time;
  struct timeval last_token_time;
  bool has_first_token;
  bool is_anthropic;
} StreamCtx;

void llm_init(void) { curl_global_init(CURL_GLOBAL_DEFAULT); }

void llm_cleanup(void) { curl_global_cleanup(); }

int llm_estimate_tokens(const char *text) {
  if (!text)
    return 0;
  size_t len = strlen(text);
  return (int)(len / 3.35) + 1;
}

static size_t tokenize_write_callback(char *ptr, size_t size, size_t nmemb,
                                      void *userdata) {
  size_t bytes = size * nmemb;
  char **response = (char **)userdata;
  size_t old_len = *response ? strlen(*response) : 0;
  char *tmp = realloc(*response, old_len + bytes + 1);
  if (!tmp)
    return 0;
  *response = tmp;
  memcpy(*response + old_len, ptr, bytes);
  (*response)[old_len + bytes] = '\0';
  return bytes;
}

static int parse_token_count(const char *json) {
  const char *patterns[] = {"\"input_tokens\":", "\"length\":", "\"count\":",
                            "\"value\":", "\"tokens\":"};
  for (int i = 0; i < 5; i++) {
    const char *p = strstr(json, patterns[i]);
    if (p) {
      p += strlen(patterns[i]);
      while (*p == ' ' || *p == '\t')
        p++;
      if (i == 4) {
        if (*p == '[') {
          int count = 0;
          p++;
          while (*p && *p != ']') {
            if (*p == ',')
              count++;
            else if (*p != ' ' && *p != '\t' && *p != '\n' && count == 0)
              count = 1;
            p++;
          }
          return count;
        }
      } else {
        return atoi(p);
      }
    }
  }
  return -1;
}

int llm_tokenize(const ModelConfig *config, const char *text) {
  if (!config || !text)
    return llm_estimate_tokens(text);

  if (config->api_type == API_TYPE_OPENAI) {
    return llm_estimate_tokens(text);
  }

  CURL *curl = curl_easy_init();
  if (!curl)
    return llm_estimate_tokens(text);

  char url[512];
  char *body = NULL;
  const char *base = config->base_url;
  size_t base_len = strlen(base);
  while (base_len > 0 && base[base_len - 1] == '/')
    base_len--;

  char base_trimmed[256];
  snprintf(base_trimmed, sizeof(base_trimmed), "%.*s", (int)base_len, base);

  char *base_no_v1 = base_trimmed;
  size_t len = strlen(base_no_v1);
  if (len >= 3 && strcmp(base_no_v1 + len - 3, "/v1") == 0) {
    base_no_v1[len - 3] = '\0';
  }

  switch (config->api_type) {
  case API_TYPE_APHRODITE:
    snprintf(url, sizeof(url), "%s/v1/tokenize", base_no_v1);
    break;
  case API_TYPE_VLLM:
    snprintf(url, sizeof(url), "%s/tokenize", base_no_v1);
    break;
  case API_TYPE_LLAMACPP:
    snprintf(url, sizeof(url), "%s/tokenize", base_no_v1);
    break;
  case API_TYPE_KOBOLDCPP:
    snprintf(url, sizeof(url), "%s/api/extra/tokencount", base_no_v1);
    break;
  case API_TYPE_TABBY:
    snprintf(url, sizeof(url), "%s/v1/token/encode", base_no_v1);
    break;
  case API_TYPE_ANTHROPIC:
    snprintf(url, sizeof(url), "%s/v1/messages/count_tokens", base_no_v1);
    break;
  default:
    curl_easy_cleanup(curl);
    return llm_estimate_tokens(text);
  }

  char *escaped_text = escape_json_string(text);
  if (!escaped_text) {
    curl_easy_cleanup(curl);
    return llm_estimate_tokens(text);
  }

  size_t body_size = strlen(escaped_text) + 256;
  body = malloc(body_size);
  if (!body) {
    free(escaped_text);
    curl_easy_cleanup(curl);
    return llm_estimate_tokens(text);
  }

  switch (config->api_type) {
  case API_TYPE_APHRODITE:
  case API_TYPE_VLLM:
    snprintf(body, body_size, "{\"model\":\"%s\",\"prompt\":\"%s\"}",
             config->model_id, escaped_text);
    break;
  case API_TYPE_LLAMACPP:
    snprintf(body, body_size, "{\"content\":\"%s\"}", escaped_text);
    break;
  case API_TYPE_KOBOLDCPP:
    snprintf(body, body_size, "{\"prompt\":\"%s\"}", escaped_text);
    break;
  case API_TYPE_TABBY:
    snprintf(body, body_size, "{\"text\":\"%s\"}", escaped_text);
    break;
  case API_TYPE_ANTHROPIC:
    snprintf(body, body_size,
             "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":"
             "\"%s\"}]}",
             config->model_id, escaped_text);
    break;
  default:
    break;
  }

  free(escaped_text);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  if (config->api_key[0]) {
    char auth[300];
    if (config->api_type == API_TYPE_ANTHROPIC) {
      snprintf(auth, sizeof(auth), "x-api-key: %s", config->api_key);
      headers = curl_slist_append(headers, auth);
      headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    } else {
      snprintf(auth, sizeof(auth), "Authorization: Bearer %s", config->api_key);
      headers = curl_slist_append(headers, auth);
    }
  }

  char *response = NULL;
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, tokenize_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  if (strstr(url, "localhost") || strstr(url, "127.0.0.1") ||
      strstr(url, "0.0.0.0")) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
  }
  curl_easy_setopt(curl, CURLOPT_NOPROXY, "localhost,127.0.0.1,0.0.0.0");

  CURLcode res = curl_easy_perform(curl);

  int token_count = -1;
  if (res == CURLE_OK && response) {
    token_count = parse_token_count(response);
  }

  free(response);
  free(body);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (token_count < 0)
    return llm_estimate_tokens(text);

  return token_count;
}

static void append_to_response(LLMResponse *resp, const char *data,
                               size_t len) {
  if (resp->len + len + 1 > resp->cap) {
    size_t newcap = resp->cap == 0 ? 1024 : resp->cap * 2;
    while (newcap < resp->len + len + 1)
      newcap *= 2;
    char *tmp = realloc(resp->content, newcap);
    if (!tmp)
      return;
    resp->content = tmp;
    resp->cap = newcap;
  }
  memcpy(resp->content + resp->len, data, len);
  resp->len += len;
  resp->content[resp->len] = '\0';
}

static char *find_json_string(const char *json, const char *key) {
  char search[128];
  snprintf(search, sizeof(search), "\"%s\"", key);
  const char *p = strstr(json, search);
  if (!p)
    return NULL;
  p += strlen(search);
  while (*p == ' ' || *p == ':')
    p++;
  if (*p != '"')
    return NULL;
  p++;
  const char *end = p;
  while (*end && *end != '"') {
    if (*end == '\\' && *(end + 1))
      end += 2;
    else
      end++;
  }
  size_t len = end - p;
  char *result = malloc(len + 1);
  if (!result)
    return NULL;

  size_t j = 0;
  for (size_t i = 0; i < len; i++) {
    if (p[i] == '\\' && i + 1 < len) {
      i++;
      if (p[i] == 'n')
        result[j++] = '\n';
      else if (p[i] == 't')
        result[j++] = '\t';
      else if (p[i] == 'r')
        result[j++] = '\r';
      else
        result[j++] = p[i];
    } else {
      result[j++] = p[i];
    }
  }
  result[j] = '\0';
  return result;
}

static int find_json_int(const char *json, const char *key) {
  char search[64];
  snprintf(search, sizeof(search), "\"%s\"", key);
  const char *p = strstr(json, search);
  if (!p)
    return -1;
  p += strlen(search);
  while (*p && (*p == ' ' || *p == ':'))
    p++;
  if (*p == '-' || (*p >= '0' && *p <= '9'))
    return atoi(p);
  return -1;
}

static void process_sse_line(StreamCtx *ctx, const char *line,
                             bool is_anthropic) {
  if (strncmp(line, "data: ", 6) != 0)
    return;
  const char *data = line + 6;

  if (strcmp(data, "[DONE]") == 0)
    return;

  const char *usage = strstr(data, "\"usage\"");
  if (usage) {
    int prompt = find_json_int(usage, "prompt_tokens");
    int completion = find_json_int(usage, "completion_tokens");
    if (prompt < 0)
      prompt = find_json_int(usage, "input_tokens");
    if (completion < 0)
      completion = find_json_int(usage, "output_tokens");
    if (prompt > 0)
      ctx->prompt_tokens = prompt;
    if (completion > 0)
      ctx->completion_tokens = completion;
  }

  char *content = NULL;

  if (is_anthropic) {
    if (strstr(data, "\"content_block_delta\"") ||
        strstr(data, "\"text_delta\"")) {
      const char *delta = strstr(data, "\"delta\"");
      if (delta) {
        content = find_json_string(delta, "text");
      }
    }
  } else {
    const char *choices = strstr(data, "\"choices\"");
    if (!choices)
      return;

    const char *delta = strstr(choices, "\"delta\"");
    if (!delta)
      return;

    const char *content_key = strstr(delta, "\"content\"");
    if (!content_key)
      return;

    content = find_json_string(delta, "content");
  }

  if (content && content[0]) {
    if (!ctx->has_first_token) {
      gettimeofday(&ctx->first_token_time, NULL);
      ctx->has_first_token = true;
    }
    gettimeofday(&ctx->last_token_time, NULL);
    ctx->got_content = true;
    append_to_response(ctx->resp, content, strlen(content));
    if (ctx->cb) {
      ctx->cb(content, ctx->userdata);
    }
    free(content);
  }
}

static size_t stream_callback(char *ptr, size_t size, size_t nmemb,
                              void *userdata) {
  StreamCtx *ctx = userdata;
  size_t bytes = size * nmemb;

  for (size_t i = 0; i < bytes; i++) {
    char c = ptr[i];
    if (c == '\n') {
      ctx->line_buffer[ctx->line_len] = '\0';
      if (ctx->line_len > 0) {
        process_sse_line(ctx, ctx->line_buffer, ctx->is_anthropic);
      }
      ctx->line_len = 0;
    } else if (c != '\r' && ctx->line_len < sizeof(ctx->line_buffer) - 1) {
      ctx->line_buffer[ctx->line_len++] = c;
    }
  }

  return bytes;
}

static int progress_callback(void *clientp, curl_off_t dltotal,
                             curl_off_t dlnow, curl_off_t ultotal,
                             curl_off_t ulnow) {
  (void)dltotal;
  (void)dlnow;
  (void)ultotal;
  (void)ulnow;
  StreamCtx *ctx = clientp;
  if (!ctx->got_content && ctx->progress_cb) {
    ctx->progress_cb(ctx->userdata);
  }
  return 0;
}

static char *escape_json_string(const char *str) {
  size_t len = strlen(str);
  char *escaped = malloc(len * 2 + 1);
  if (!escaped)
    return NULL;

  size_t j = 0;
  for (size_t i = 0; i < len; i++) {
    char c = str[i];
    if (c == '"' || c == '\\') {
      escaped[j++] = '\\';
      escaped[j++] = c;
    } else if (c == '\n') {
      escaped[j++] = '\\';
      escaped[j++] = 'n';
    } else if (c == '\r') {
      escaped[j++] = '\\';
      escaped[j++] = 'r';
    } else if (c == '\t') {
      escaped[j++] = '\\';
      escaped[j++] = 't';
    } else {
      escaped[j++] = c;
    }
  }
  escaped[j] = '\0';
  return escaped;
}

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} StringBuilder;

static void sb_init(StringBuilder *sb) {
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
}

static void sb_append(StringBuilder *sb, const char *str) {
  if (!str)
    return;
  size_t slen = strlen(str);
  if (sb->len + slen + 1 > sb->cap) {
    size_t newcap = sb->cap == 0 ? 1024 : sb->cap * 2;
    while (newcap < sb->len + slen + 1)
      newcap *= 2;
    char *tmp = realloc(sb->data, newcap);
    if (!tmp)
      return;
    sb->data = tmp;
    sb->cap = newcap;
  }
  memcpy(sb->data + sb->len, str, slen);
  sb->len += slen;
  sb->data[sb->len] = '\0';
}

static void sb_free(StringBuilder *sb) {
  free(sb->data);
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
}

typedef struct {
  char *role;
  char *content;
} ExampleMessage;

static ExampleMessage *parse_mes_example(const char *mes_example,
                                         size_t *out_count,
                                         const char *char_name,
                                         const char *user_name) {
  *out_count = 0;
  if (!mes_example || !mes_example[0])
    return NULL;

  size_t cap = 16;
  ExampleMessage *messages = malloc(cap * sizeof(ExampleMessage));
  if (!messages)
    return NULL;

  char *substituted = macro_substitute(mes_example, char_name, user_name);
  if (!substituted) {
    free(messages);
    return NULL;
  }

  char *lines = substituted;
  char *line = strtok(lines, "\n");

  char *current_role = NULL;
  StringBuilder current_content;
  sb_init(&current_content);

  char user_prefix[128];
  char char_prefix[128];
  snprintf(user_prefix, sizeof(user_prefix), "%s:", user_name);
  snprintf(char_prefix, sizeof(char_prefix), "%s:", char_name);

  while (line) {
    while (*line == ' ' || *line == '\t')
      line++;

    if (strncmp(line, "<START>", 7) == 0) {
      if (current_role && current_content.data) {
        if (*out_count >= cap) {
          cap *= 2;
          ExampleMessage *tmp = realloc(messages, cap * sizeof(ExampleMessage));
          if (!tmp)
            break;
          messages = tmp;
        }
        messages[*out_count].role = current_role;
        messages[*out_count].content = current_content.data;
        (*out_count)++;
        current_role = NULL;
        sb_init(&current_content);
      }
      line = strtok(NULL, "\n");
      continue;
    }

    bool is_user = strncmp(line, user_prefix, strlen(user_prefix)) == 0;
    bool is_char = strncmp(line, char_prefix, strlen(char_prefix)) == 0;

    if (is_user || is_char) {
      if (current_role && current_content.data) {
        if (*out_count >= cap) {
          cap *= 2;
          ExampleMessage *tmp = realloc(messages, cap * sizeof(ExampleMessage));
          if (!tmp)
            break;
          messages = tmp;
        }
        messages[*out_count].role = current_role;
        messages[*out_count].content = current_content.data;
        (*out_count)++;
        sb_init(&current_content);
      }

      current_role = is_user ? strdup("user") : strdup("assistant");
      const char *content_start =
          line + (is_user ? strlen(user_prefix) : strlen(char_prefix));
      while (*content_start == ' ')
        content_start++;
      sb_append(&current_content, content_start);
    } else if (current_role) {
      sb_append(&current_content, "\n");
      sb_append(&current_content, line);
    }

    line = strtok(NULL, "\n");
  }

  if (current_role && current_content.data) {
    if (*out_count >= cap) {
      cap *= 2;
      ExampleMessage *tmp = realloc(messages, cap * sizeof(ExampleMessage));
      if (tmp)
        messages = tmp;
    }
    if (*out_count < cap) {
      messages[*out_count].role = current_role;
      messages[*out_count].content = current_content.data;
      (*out_count)++;
    } else {
      free(current_role);
      sb_free(&current_content);
    }
  } else {
    free(current_role);
    sb_free(&current_content);
  }

  free(substituted);
  return messages;
}

static void free_example_messages(ExampleMessage *messages, size_t count) {
  if (!messages)
    return;
  for (size_t i = 0; i < count; i++) {
    free(messages[i].role);
    free(messages[i].content);
  }
  free(messages);
}

static char *build_system_prompt(const LLMContext *context) {
  if (!context || !context->character)
    return NULL;

  const CharacterCard *card = context->character;
  const Persona *persona = context->persona;
  const char *char_name = card->name;
  const char *user_name = persona ? persona_get_name(persona) : "User";

  StringBuilder sb;
  sb_init(&sb);

  if (card->system_prompt && card->system_prompt[0]) {
    char *substituted =
        macro_substitute(card->system_prompt, char_name, user_name);
    if (substituted) {
      sb_append(&sb, substituted);
      free(substituted);
    }
  } else {
    char default_prompt[512];
    snprintf(default_prompt, sizeof(default_prompt),
             "Write %.127s's next reply in a fictional chat between %.127s and "
             "%.127s.",
             char_name, char_name, user_name);
    sb_append(&sb, default_prompt);
  }

  if (persona && persona->description[0]) {
    sb_append(&sb, "\n\n[User Persona: ");
    sb_append(&sb, user_name);
    sb_append(&sb, "]\n");
    char *substituted =
        macro_substitute(persona->description, char_name, user_name);
    if (substituted) {
      sb_append(&sb, substituted);
      free(substituted);
    }
  }

  if (card->description && card->description[0]) {
    sb_append(&sb, "\n\n[Character: ");
    sb_append(&sb, char_name);
    sb_append(&sb, "]\n");
    char *substituted =
        macro_substitute(card->description, char_name, user_name);
    if (substituted) {
      sb_append(&sb, substituted);
      free(substituted);
    }
  }

  if (card->personality && card->personality[0]) {
    sb_append(&sb, "\n\n[Personality]\n");
    char *substituted =
        macro_substitute(card->personality, char_name, user_name);
    if (substituted) {
      sb_append(&sb, substituted);
      free(substituted);
    }
  }

  if (card->scenario && card->scenario[0]) {
    sb_append(&sb, "\n\n[Scenario]\n");
    char *substituted = macro_substitute(card->scenario, char_name, user_name);
    if (substituted) {
      sb_append(&sb, substituted);
      free(substituted);
    }
  }

  return sb.data;
}

static int count_tokens(const ModelConfig *config, const char *text) {
  if (!config || config->api_type == API_TYPE_OPENAI)
    return llm_estimate_tokens(text);
  return llm_tokenize(config, text);
}

static char *build_anthropic_request_body(const ModelConfig *config,
                                          const ChatHistory *history,
                                          const LLMContext *context) {
  int context_length = config->context_length > 0 ? config->context_length
                                                  : DEFAULT_CONTEXT_LENGTH;
  size_t cap = 16384;
  char *body = malloc(cap);
  if (!body)
    return NULL;

  const char *char_name =
      (context && context->character) ? context->character->name : NULL;
  const char *user_name = (context && context->persona)
                              ? persona_get_name(context->persona)
                              : "User";

  size_t pos = 0;
  pos += snprintf(body + pos, cap - pos, "{\"model\":\"%s\"", config->model_id);

  char *system_prompt = build_system_prompt(context);
  if (system_prompt) {
    char *escaped = escape_json_string(system_prompt);
    if (escaped) {
      size_t needed = strlen(escaped) + 64;
      if (pos + needed >= cap) {
        cap = (pos + needed) * 2;
        char *tmp = realloc(body, cap);
        if (tmp)
          body = tmp;
      }
      pos += snprintf(body + pos, cap - pos, ",\"system\":\"%s\"", escaped);
      free(escaped);
    }
    free(system_prompt);
  }

  pos += snprintf(body + pos, cap - pos, ",\"messages\":[");

  bool first = true;

  if (context && context->character && context->character->mes_example) {
    size_t example_count = 0;
    ExampleMessage *examples = parse_mes_example(
        context->character->mes_example, &example_count, char_name, user_name);
    if (examples) {
      for (size_t i = 0; i < example_count; i++) {
        char *escaped = escape_json_string(examples[i].content);
        if (!escaped)
          continue;

        size_t needed = strlen(escaped) + 64;
        if (pos + needed >= cap) {
          cap = (pos + needed) * 2;
          char *tmp = realloc(body, cap);
          if (tmp)
            body = tmp;
        }

        if (!first)
          body[pos++] = ',';
        first = false;

        pos += snprintf(body + pos, cap - pos,
                        "{\"role\":\"%s\",\"content\":\"%s\"}",
                        examples[i].role, escaped);
        free(escaped);
      }
      free_example_messages(examples, example_count);
    }
  }

  int tokens_used = count_tokens(config, body);

  const SamplerSettings *s = context ? context->samplers : NULL;
  int max_tok = (s && s->max_tokens > 0) ? s->max_tokens : 4096;
  int available_tokens = context_length - tokens_used - max_tok;

  int post_history_tokens = 0;
  if (context && context->character &&
      context->character->post_history_instructions &&
      context->character->post_history_instructions[0]) {
    post_history_tokens =
        count_tokens(config, context->character->post_history_instructions) +
        20;
    available_tokens -= post_history_tokens;
  }

  size_t start_index = 0;
  if (history->count > 0 && available_tokens > 0) {
    int cumulative_tokens = 0;
    for (size_t i = history->count; i > 0; i--) {
      const char *msg = history_get(history, i - 1);
      if (!msg)
        continue;

      int msg_tokens = count_tokens(config, msg) + 20;
      if (cumulative_tokens + msg_tokens > available_tokens) {
        start_index = i;
        break;
      }
      cumulative_tokens += msg_tokens;
    }
  }

  for (size_t i = start_index; i < history->count; i++) {
    const char *msg = history_get(history, i);
    if (!msg)
      continue;
    const char *role = NULL;
    const char *content = NULL;

    if (strncmp(msg, "You: ", 5) == 0) {
      role = "user";
      content = msg + 5;
    } else if (strncmp(msg, "Bot: ", 5) == 0) {
      role = "assistant";
      content = msg + 5;
    } else if (strncmp(msg, "Bot:", 4) == 0) {
      role = "assistant";
      content = msg + 4;
      while (*content == ' ')
        content++;
    } else {
      continue;
    }

    char *escaped = escape_json_string(content);
    if (!escaped)
      continue;

    size_t needed = strlen(escaped) + 64;
    if (pos + needed >= cap) {
      cap = (pos + needed) * 2;
      char *tmp = realloc(body, cap);
      if (!tmp) {
        free(escaped);
        free(body);
        return NULL;
      }
      body = tmp;
    }

    if (!first)
      body[pos++] = ',';
    first = false;

    pos += snprintf(body + pos, cap - pos,
                    "{\"role\":\"%s\",\"content\":\"%s\"}", role, escaped);
    free(escaped);
  }

  if (context && context->character &&
      context->character->post_history_instructions &&
      context->character->post_history_instructions[0]) {
    char *substituted = macro_substitute(
        context->character->post_history_instructions, char_name, user_name);
    if (substituted) {
      char *escaped = escape_json_string(substituted);
      if (escaped) {
        size_t needed = strlen(escaped) + 64;
        if (pos + needed >= cap) {
          cap = (pos + needed) * 2;
          char *tmp = realloc(body, cap);
          if (tmp)
            body = tmp;
        }
        if (!first)
          body[pos++] = ',';
        pos += snprintf(body + pos, cap - pos,
                        "{\"role\":\"user\",\"content\":\"%s\"}", escaped);
        free(escaped);
      }
      free(substituted);
    }
  }

  pos += snprintf(body + pos, cap - pos, "]");

  pos += snprintf(body + pos, cap - pos, ",\"max_tokens\":%d", max_tok);
  pos += snprintf(body + pos, cap - pos, ",\"stream\":true");

  if (s) {
    if (s->temperature != 1.0)
      pos += snprintf(body + pos, cap - pos, ",\"temperature\":%.4g",
                      s->temperature);
    if (s->top_p != 1.0)
      pos += snprintf(body + pos, cap - pos, ",\"top_p\":%.4g", s->top_p);
    if (s->top_k > 0)
      pos += snprintf(body + pos, cap - pos, ",\"top_k\":%d", s->top_k);
  }

  pos += snprintf(body + pos, cap - pos, "}");
  return body;
}

static char *build_request_body(const ModelConfig *config,
                                const ChatHistory *history,
                                const LLMContext *context) {
  int context_length = config->context_length > 0 ? config->context_length
                                                  : DEFAULT_CONTEXT_LENGTH;
  size_t cap = 16384;
  char *body = malloc(cap);
  if (!body)
    return NULL;

  const char *char_name =
      (context && context->character) ? context->character->name : NULL;
  const char *user_name = (context && context->persona)
                              ? persona_get_name(context->persona)
                              : "User";

  size_t pos = 0;
  pos += snprintf(body + pos, cap - pos, "{\"model\":\"%s\",\"messages\":[",
                  config->model_id);

  bool first = true;

  char *system_prompt = build_system_prompt(context);
  if (system_prompt) {
    char *escaped = escape_json_string(system_prompt);
    if (escaped) {
      size_t needed = strlen(escaped) + 64;
      if (pos + needed >= cap) {
        cap = (pos + needed) * 2;
        char *tmp = realloc(body, cap);
        if (tmp)
          body = tmp;
      }
      pos += snprintf(body + pos, cap - pos,
                      "{\"role\":\"system\",\"content\":\"%s\"}", escaped);
      first = false;
      free(escaped);
    }
    free(system_prompt);
  }

  if (context && context->character && context->character->mes_example) {
    size_t example_count = 0;
    ExampleMessage *examples = parse_mes_example(
        context->character->mes_example, &example_count, char_name, user_name);
    if (examples) {
      for (size_t i = 0; i < example_count; i++) {
        char *escaped = escape_json_string(examples[i].content);
        if (!escaped)
          continue;

        size_t needed = strlen(escaped) + 64;
        if (pos + needed >= cap) {
          cap = (pos + needed) * 2;
          char *tmp = realloc(body, cap);
          if (tmp)
            body = tmp;
        }

        if (!first)
          body[pos++] = ',';
        first = false;

        pos += snprintf(body + pos, cap - pos,
                        "{\"role\":\"%s\",\"content\":\"%s\"}",
                        examples[i].role, escaped);
        free(escaped);
      }
      free_example_messages(examples, example_count);
    }
  }

  int tokens_used = count_tokens(config, body);

  int max_tokens_reserve =
      (context && context->samplers && context->samplers->max_tokens > 0)
          ? context->samplers->max_tokens
          : 512;
  int available_tokens = context_length - tokens_used - max_tokens_reserve;

  int post_history_tokens = 0;
  if (context && context->character &&
      context->character->post_history_instructions &&
      context->character->post_history_instructions[0]) {
    post_history_tokens =
        count_tokens(config, context->character->post_history_instructions) +
        20;
    available_tokens -= post_history_tokens;
  }

  size_t start_index = 0;
  if (history->count > 0 && available_tokens > 0) {
    int cumulative_tokens = 0;
    for (size_t i = history->count; i > 0; i--) {
      const char *msg = history_get(history, i - 1);
      if (!msg)
        continue;

      int msg_tokens = count_tokens(config, msg) + 20;
      if (cumulative_tokens + msg_tokens > available_tokens) {
        start_index = i;
        break;
      }
      cumulative_tokens += msg_tokens;
    }
  }

  for (size_t i = start_index; i < history->count; i++) {
    const char *msg = history_get(history, i);
    if (!msg)
      continue;
    const char *role = NULL;
    const char *content = NULL;

    if (strncmp(msg, "You: ", 5) == 0) {
      role = "user";
      content = msg + 5;
    } else if (strncmp(msg, "Bot: ", 5) == 0) {
      role = "assistant";
      content = msg + 5;
    } else if (strncmp(msg, "Bot:", 4) == 0) {
      role = "assistant";
      content = msg + 4;
      while (*content == ' ')
        content++;
    } else {
      continue;
    }

    char *escaped = escape_json_string(content);
    if (!escaped)
      continue;

    size_t needed = strlen(escaped) + 64;
    if (pos + needed >= cap) {
      cap = (pos + needed) * 2;
      char *tmp = realloc(body, cap);
      if (!tmp) {
        free(escaped);
        free(body);
        return NULL;
      }
      body = tmp;
    }

    if (!first) {
      body[pos++] = ',';
    }
    first = false;

    pos += snprintf(body + pos, cap - pos,
                    "{\"role\":\"%s\",\"content\":\"%s\"}", role, escaped);
    free(escaped);
  }

  if (context && context->character &&
      context->character->post_history_instructions &&
      context->character->post_history_instructions[0]) {
    char *substituted = macro_substitute(
        context->character->post_history_instructions, char_name, user_name);
    if (substituted) {
      char *escaped = escape_json_string(substituted);
      if (escaped) {
        size_t needed = strlen(escaped) + 64;
        if (pos + needed >= cap) {
          cap = (pos + needed) * 2;
          char *tmp = realloc(body, cap);
          if (tmp)
            body = tmp;
        }
        if (!first)
          body[pos++] = ',';
        pos += snprintf(body + pos, cap - pos,
                        "{\"role\":\"system\",\"content\":\"%s\"}", escaped);
        free(escaped);
      }
      free(substituted);
    }
  }

  pos += snprintf(body + pos, cap - pos, "],\"stream\":true");

  const SamplerSettings *s = context ? context->samplers : NULL;
  int max_tok = (s && s->max_tokens > 0) ? s->max_tokens : 512;
  pos += snprintf(body + pos, cap - pos, ",\"max_tokens\":%d", max_tok);

  if (s) {
    if (s->temperature != 1.0)
      pos += snprintf(body + pos, cap - pos, ",\"temperature\":%.4g",
                      s->temperature);
    if (s->top_p != 1.0)
      pos += snprintf(body + pos, cap - pos, ",\"top_p\":%.4g", s->top_p);
    if (s->frequency_penalty != 0.0)
      pos += snprintf(body + pos, cap - pos, ",\"frequency_penalty\":%.4g",
                      s->frequency_penalty);
    if (s->presence_penalty != 0.0)
      pos += snprintf(body + pos, cap - pos, ",\"presence_penalty\":%.4g",
                      s->presence_penalty);

    if (config->api_type == API_TYPE_APHRODITE ||
        config->api_type == API_TYPE_VLLM ||
        config->api_type == API_TYPE_LLAMACPP ||
        config->api_type == API_TYPE_KOBOLDCPP ||
        config->api_type == API_TYPE_TABBY) {
      if (s->top_k > 0)
        pos += snprintf(body + pos, cap - pos, ",\"top_k\":%d", s->top_k);
      if (s->min_p > 0.0)
        pos += snprintf(body + pos, cap - pos, ",\"min_p\":%.4g", s->min_p);
      if (s->repetition_penalty != 1.0)
        pos += snprintf(body + pos, cap - pos, ",\"repetition_penalty\":%.4g",
                        s->repetition_penalty);
      if (s->typical_p != 1.0)
        pos += snprintf(body + pos, cap - pos, ",\"typical_p\":%.4g",
                        s->typical_p);
      if (s->tfs != 1.0)
        pos += snprintf(body + pos, cap - pos, ",\"tfs\":%.4g", s->tfs);
      if (s->top_a > 0.0)
        pos += snprintf(body + pos, cap - pos, ",\"top_a\":%.4g", s->top_a);
      if (s->min_tokens > 0)
        pos += snprintf(body + pos, cap - pos, ",\"min_tokens\":%d",
                        s->min_tokens);
    }

    if (config->api_type == API_TYPE_APHRODITE) {
      if (s->smoothing_factor > 0.0)
        pos += snprintf(body + pos, cap - pos, ",\"smoothing_factor\":%.4g",
                        s->smoothing_factor);
      if (s->dynatemp_min > 0.0 || s->dynatemp_max > 0.0) {
        pos += snprintf(body + pos, cap - pos, ",\"dynatemp_min\":%.4g",
                        s->dynatemp_min);
        pos += snprintf(body + pos, cap - pos, ",\"dynatemp_max\":%.4g",
                        s->dynatemp_max);
        pos += snprintf(body + pos, cap - pos, ",\"dynatemp_exponent\":%.4g",
                        s->dynatemp_exponent);
      }
      if (s->mirostat_mode > 0) {
        pos += snprintf(body + pos, cap - pos, ",\"mirostat_mode\":%d",
                        s->mirostat_mode);
        pos += snprintf(body + pos, cap - pos, ",\"mirostat_tau\":%.4g",
                        s->mirostat_tau);
        pos += snprintf(body + pos, cap - pos, ",\"mirostat_eta\":%.4g",
                        s->mirostat_eta);
      }
      if (s->dry_multiplier > 0.0) {
        pos += snprintf(body + pos, cap - pos, ",\"dry_multiplier\":%.4g",
                        s->dry_multiplier);
        pos +=
            snprintf(body + pos, cap - pos, ",\"dry_base\":%.4g", s->dry_base);
        pos += snprintf(body + pos, cap - pos, ",\"dry_allowed_length\":%d",
                        s->dry_allowed_length);
        pos +=
            snprintf(body + pos, cap - pos, ",\"dry_range\":%d", s->dry_range);
      }
      if (s->xtc_probability > 0.0) {
        pos += snprintf(body + pos, cap - pos, ",\"xtc_threshold\":%.4g",
                        s->xtc_threshold);
        pos += snprintf(body + pos, cap - pos, ",\"xtc_probability\":%.4g",
                        s->xtc_probability);
      }
      if (s->nsigma > 0.0)
        pos += snprintf(body + pos, cap - pos, ",\"nsigma\":%.4g", s->nsigma);
      if (s->skew != 0.0)
        pos += snprintf(body + pos, cap - pos, ",\"skew\":%.4g", s->skew);
    }

    for (int i = 0; i < s->custom_count; i++) {
      const CustomSampler *cs = &s->custom[i];
      if (cs->type == SAMPLER_TYPE_STRING) {
        char *escaped = escape_json_string(cs->str_value);
        pos += snprintf(body + pos, cap - pos, ",\"%s\":\"%s\"", cs->name,
                        escaped ? escaped : cs->str_value);
        free(escaped);
      } else if (cs->type == SAMPLER_TYPE_INT) {
        pos += snprintf(body + pos, cap - pos, ",\"%s\":%d", cs->name,
                        (int)cs->value);
      } else if (cs->type == SAMPLER_TYPE_BOOL) {
        pos += snprintf(body + pos, cap - pos, ",\"%s\":%s", cs->name,
                        cs->value != 0 ? "true" : "false");
      } else if (cs->type == SAMPLER_TYPE_LIST_FLOAT ||
                 cs->type == SAMPLER_TYPE_LIST_INT) {
        pos += snprintf(body + pos, cap - pos, ",\"%s\":[", cs->name);
        for (int j = 0; j < cs->list_count; j++) {
          if (cs->type == SAMPLER_TYPE_LIST_INT)
            pos +=
                snprintf(body + pos, cap - pos, "%d", (int)cs->list_values[j]);
          else
            pos += snprintf(body + pos, cap - pos, "%.4g", cs->list_values[j]);
          if (j < cs->list_count - 1)
            pos += snprintf(body + pos, cap - pos, ",");
        }
        pos += snprintf(body + pos, cap - pos, "]");
      } else if (cs->type == SAMPLER_TYPE_LIST_STRING) {
        pos += snprintf(body + pos, cap - pos, ",\"%s\":[", cs->name);
        for (int j = 0; j < cs->list_count; j++) {
          char *escaped = escape_json_string(cs->list_strings[j]);
          pos += snprintf(body + pos, cap - pos, "\"%s\"",
                          escaped ? escaped : cs->list_strings[j]);
          free(escaped);
          if (j < cs->list_count - 1)
            pos += snprintf(body + pos, cap - pos, ",");
        }
        pos += snprintf(body + pos, cap - pos, "]");
      } else if (cs->type == SAMPLER_TYPE_DICT) {
        pos += snprintf(body + pos, cap - pos, ",\"%s\":{", cs->name);
        for (int j = 0; j < cs->dict_count; j++) {
          const DictEntry *de = &cs->dict_entries[j];
          if (de->is_string) {
            char *escaped = escape_json_string(de->str_val);
            pos += snprintf(body + pos, cap - pos, "\"%s\":\"%s\"", de->key,
                            escaped ? escaped : de->str_val);
            free(escaped);
          } else {
            pos += snprintf(body + pos, cap - pos, "\"%s\":%.4g", de->key,
                            de->num_val);
          }
          if (j < cs->dict_count - 1)
            pos += snprintf(body + pos, cap - pos, ",");
        }
        pos += snprintf(body + pos, cap - pos, "}");
      } else {
        pos += snprintf(body + pos, cap - pos, ",\"%s\":%.4g", cs->name,
                        cs->value);
      }
    }
  }

  pos += snprintf(body + pos, cap - pos,
                  ",\"stream_options\":{\"include_usage\":true}}");
  return body;
}

LLMResponse llm_chat(const ModelConfig *config, const ChatHistory *history,
                     const LLMContext *context, LLMStreamCallback stream_cb,
                     LLMProgressCallback progress_cb, void *userdata) {
  LLMResponse resp = {0};

  if (!config || !config->base_url[0] || !config->model_id[0]) {
    snprintf(resp.error, sizeof(resp.error), "No model configured");
    return resp;
  }

  CURL *curl = curl_easy_init();
  if (!curl) {
    snprintf(resp.error, sizeof(resp.error), "Failed to init curl");
    return resp;
  }

  char url[512];
  if (config->api_type == API_TYPE_ANTHROPIC) {
    const char *base = config->base_url;
    size_t base_len = strlen(base);
    while (base_len > 0 && base[base_len - 1] == '/')
      base_len--;
    char base_trimmed[256];
    snprintf(base_trimmed, sizeof(base_trimmed), "%.*s", (int)base_len, base);
    if (base_len >= 3 && strcmp(base_trimmed + base_len - 3, "/v1") == 0)
      base_trimmed[base_len - 3] = '\0';
    snprintf(url, sizeof(url), "%s/v1/messages", base_trimmed);
  } else {
    snprintf(url, sizeof(url), "%s/chat/completions", config->base_url);
  }

  char *body;
  if (config->api_type == API_TYPE_ANTHROPIC) {
    body = build_anthropic_request_body(config, history, context);
  } else {
    body = build_request_body(config, history, context);
  }
  if (!body) {
    snprintf(resp.error, sizeof(resp.error), "Failed to build request");
    curl_easy_cleanup(curl);
    return resp;
  }

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  if (config->api_key[0]) {
    char auth[320];
    if (config->api_type == API_TYPE_ANTHROPIC) {
      snprintf(auth, sizeof(auth), "x-api-key: %s", config->api_key);
      headers = curl_slist_append(headers, auth);
      headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    } else {
      snprintf(auth, sizeof(auth), "Authorization: Bearer %s", config->api_key);
      headers = curl_slist_append(headers, auth);
    }
  }

  StreamCtx ctx = {.resp = &resp,
                   .cb = stream_cb,
                   .progress_cb = progress_cb,
                   .userdata = userdata,
                   .line_len = 0,
                   .got_content = false,
                   .prompt_tokens = 0,
                   .completion_tokens = 0,
                   .has_first_token = false,
                   .is_anthropic = (config->api_type == API_TYPE_ANTHROPIC)};

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

  if (strstr(url, "localhost") || strstr(url, "127.0.0.1") ||
      strstr(url, "0.0.0.0")) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
  }

  curl_easy_setopt(curl, CURLOPT_NOPROXY, "localhost,127.0.0.1,0.0.0.0");

  if (strncmp(url, "http://", 7) == 0) {
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  }

  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
  curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);

  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  CURLcode res = curl_easy_perform(curl);

  gettimeofday(&end_time, NULL);
  double elapsed_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                      (end_time.tv_usec - start_time.tv_usec) / 1000.0;

  if (res != CURLE_OK) {
    snprintf(resp.error, sizeof(resp.error), "Request failed: %s",
             curl_easy_strerror(res));
  } else {
    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 200 && http_code < 300) {
      resp.success = true;
    } else {
      snprintf(resp.error, sizeof(resp.error), "HTTP %ld", http_code);
    }
  }

  resp.prompt_tokens = ctx.prompt_tokens;
  resp.completion_tokens = ctx.completion_tokens;
  resp.elapsed_ms = elapsed_ms;

  if (ctx.has_first_token && ctx.completion_tokens > 0) {
    double gen_time_ms =
        (ctx.last_token_time.tv_sec - ctx.first_token_time.tv_sec) * 1000.0 +
        (ctx.last_token_time.tv_usec - ctx.first_token_time.tv_usec) / 1000.0;
    if (gen_time_ms > 0) {
      resp.output_tps = (ctx.completion_tokens * 1000.0) / gen_time_ms;
    }
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  free(body);

  return resp;
}

void llm_response_free(LLMResponse *resp) {
  free(resp->content);
  resp->content = NULL;
  resp->len = 0;
  resp->cap = 0;
}

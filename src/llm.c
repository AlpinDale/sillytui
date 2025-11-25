#include "llm.h"
#include "macros.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  LLMResponse *resp;
  LLMStreamCallback cb;
  LLMProgressCallback progress_cb;
  void *userdata;
  char line_buffer[4096];
  size_t line_len;
  bool got_content;
} StreamCtx;

void llm_init(void) { curl_global_init(CURL_GLOBAL_DEFAULT); }

void llm_cleanup(void) { curl_global_cleanup(); }

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

static void process_sse_line(StreamCtx *ctx, const char *line) {
  if (strncmp(line, "data: ", 6) != 0)
    return;
  const char *data = line + 6;

  if (strcmp(data, "[DONE]") == 0)
    return;

  const char *choices = strstr(data, "\"choices\"");
  if (!choices)
    return;

  const char *delta = strstr(choices, "\"delta\"");
  if (!delta)
    return;

  const char *content_key = strstr(delta, "\"content\"");
  if (!content_key)
    return;

  char *content = find_json_string(delta, "content");
  if (content && content[0]) {
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
        process_sse_line(ctx, ctx->line_buffer);
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
    char default_prompt[256];
    snprintf(default_prompt, sizeof(default_prompt),
             "Write %s's next reply in a fictional chat between %s and %s.",
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

static char *build_request_body(const char *model_id,
                                const ChatHistory *history,
                                const LLMContext *context) {
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
                  model_id);

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

  for (size_t i = 0; i < history->count; i++) {
    const char *msg = history->items[i];
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

  pos += snprintf(body + pos, cap - pos, "],\"stream\":true}");
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
  snprintf(url, sizeof(url), "%s/chat/completions", config->base_url);

  char *body = build_request_body(config->model_id, history, context);
  if (!body) {
    snprintf(resp.error, sizeof(resp.error), "Failed to build request");
    curl_easy_cleanup(curl);
    return resp;
  }

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  if (config->api_key[0]) {
    char auth[320];
    snprintf(auth, sizeof(auth), "Authorization: Bearer %s", config->api_key);
    headers = curl_slist_append(headers, auth);
  }

  StreamCtx ctx = {.resp = &resp,
                   .cb = stream_cb,
                   .progress_cb = progress_cb,
                   .userdata = userdata,
                   .line_len = 0,
                   .got_content = false};

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
  curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);

  CURLcode res = curl_easy_perform(curl);

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

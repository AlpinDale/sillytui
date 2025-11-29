#include "common.h"
#include "chat/history.h"
#include "core/macros.h"
#include "tokenizer/selector.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sb_init(StringBuilder *sb) {
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
}

void sb_append(StringBuilder *sb, const char *str) {
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

char *sb_finish(StringBuilder *sb) {
  char *result = sb->data;
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
  return result;
}

void sb_free(StringBuilder *sb) {
  free(sb->data);
  sb->data = NULL;
  sb->len = 0;
  sb->cap = 0;
}

char *escape_json_string(const char *str) {
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

char *find_json_string(const char *json, const char *key) {
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

int find_json_int(const char *json, const char *key) {
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

void append_to_response(LLMResponse *resp, const char *data, size_t len) {
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

void append_to_reasoning(LLMResponse *resp, const char *data, size_t len) {
  if (resp->reasoning_len + len + 1 > resp->reasoning_cap) {
    size_t newcap = resp->reasoning_cap == 0 ? 1024 : resp->reasoning_cap * 2;
    while (newcap < resp->reasoning_len + len + 1)
      newcap *= 2;
    char *tmp = realloc(resp->reasoning, newcap);
    if (!tmp)
      return;
    resp->reasoning = tmp;
    resp->reasoning_cap = newcap;
  }
  memcpy(resp->reasoning + resp->reasoning_len, data, len);
  resp->reasoning_len += len;
  resp->reasoning[resp->reasoning_len] = '\0';
}

size_t stream_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  StreamCtx *ctx = userdata;
  size_t bytes = size * nmemb;

  for (size_t i = 0; i < bytes; i++) {
    char c = ptr[i];
    if (c == '\n') {
      ctx->line_buffer[ctx->line_len] = '\0';
      if (ctx->line_len > 0) {
        extern void process_sse_line(StreamCtx * ctx, const char *line,
                                     bool is_anthropic);
        process_sse_line(ctx, ctx->line_buffer, ctx->is_anthropic);
      }
      ctx->line_len = 0;
    } else if (c != '\r' && ctx->line_len < sizeof(ctx->line_buffer) - 1) {
      ctx->line_buffer[ctx->line_len++] = c;
    }
  }

  return bytes;
}

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal, curl_off_t ulnow) {
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

ExampleMessage *parse_mes_example(const char *mes_example, size_t *out_count,
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

void free_example_messages(ExampleMessage *messages, size_t count) {
  if (!messages)
    return;
  for (size_t i = 0; i < count; i++) {
    free((void *)messages[i].role);
    free((void *)messages[i].content);
  }
  free(messages);
}

char *build_system_prompt(const LLMContext *context) {
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

static ChatTokenizer *g_current_tokenizer = NULL;

void set_current_tokenizer(ChatTokenizer *tokenizer) {
  g_current_tokenizer = tokenizer;
}

int count_tokens_with_tokenizer(ChatTokenizer *tokenizer,
                                const ModelConfig *config, const char *text) {
  if (!text)
    return -1;
  if (tokenizer && !chat_tokenizer_is_api(tokenizer) && tokenizer->loaded) {
    int result = chat_tokenizer_count(tokenizer, text);
    if (result >= 0)
      return result;
  }
  if (!config)
    return (int)(strlen(text) / 4);
  extern int llm_tokenize(const ModelConfig *config, const char *text);
  int result = llm_tokenize(config, text);
  if (result < 0) {
    return (int)(strlen(text) / 4);
  }
  return result;
}

int count_tokens(const ModelConfig *config, const char *text) {
  return count_tokens_with_tokenizer(g_current_tokenizer, config, text);
}

bool ensure_buffer_capacity(RequestBuilder *rb, size_t needed) {
  if (!rb || !rb->body || !rb->pos || !rb->cap)
    return false;
  
  if (*rb->pos + needed < *rb->cap)
    return true;
  
  size_t new_cap = (*rb->cap == 0) ? 16384 : *rb->cap * 2;
  while (new_cap < *rb->pos + needed)
    new_cap *= 2;
  
  char *tmp = realloc(*rb->body, new_cap);
  if (!tmp)
    return false;
  
  *rb->body = tmp;
  *rb->cap = new_cap;
  return true;
}

void add_message_to_json(RequestBuilder *rb, bool *first, const char *role, const char *content) {
  if (!rb || !rb->body || !rb->pos || !rb->cap || !first || !role || !content)
    return;
  
  char *escaped = escape_json_string(content);
  if (!escaped)
    return;
  
  size_t needed = strlen(escaped) + strlen(role) + 64;
  if (!ensure_buffer_capacity(rb, needed)) {
    free(escaped);
    return;
  }
  
  if (!*first) {
    (*rb->body)[(*rb->pos)++] = ',';
  }
  *first = false;
  
  *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                       "{\"role\":\"%s\",\"content\":\"%s\"}", role, escaped);
  free(escaped);
}

size_t calculate_history_start_index(const ModelConfig *config, const ChatHistory *history,
                                     int available_tokens) {
  if (!config || !history || available_tokens <= 0 || history->count == 0)
    return 0;
  
  int cumulative_tokens = 0;
  for (size_t i = history->count; i > 0; i--) {
    const char *msg = history_get(history, i - 1);
    if (!msg)
      continue;
    
    int msg_tokens = count_tokens(config, msg) + 20;
    if (cumulative_tokens + msg_tokens > available_tokens) {
      return i;
    }
    cumulative_tokens += msg_tokens;
  }
  
  return 0;
}

void process_history_message(const char *msg, MessageRole msg_role,
                             const char *char_name, const char *user_name,
                             char **out_content) {
  if (!msg || !out_content) {
    if (out_content)
      *out_content = NULL;
    return;
  }
  
  const char *content = msg;
  
  if (msg_role == ROLE_USER && strncmp(msg, "You: ", 5) == 0) {
    content = msg + 5;
  } else if (msg_role == ROLE_ASSISTANT && strncmp(msg, "Bot: ", 5) == 0) {
    content = msg + 5;
  } else if (msg_role == ROLE_ASSISTANT && strncmp(msg, "Bot:", 4) == 0) {
    content = msg + 4;
    while (*content == ' ')
      content++;
  }
  
  char *expanded = expand_attachments(content);
  char *substituted = macro_substitute(expanded ? expanded : content, char_name, user_name);
  if (expanded)
    free(expanded);
  
  *out_content = substituted;
}

void add_sampler_settings_openai_compatible(RequestBuilder *rb, const ModelConfig *config,
                                            const SamplerSettings *s) {
  if (!rb || !rb->body || !rb->pos || !rb->cap || !s)
    return;
  
  if (s->temperature != 1.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"temperature\":%.4g", s->temperature);
  }
  if (s->top_p != 1.0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"top_p\":%.4g", s->top_p);
  }
  if (s->frequency_penalty != 0.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"frequency_penalty\":%.4g", s->frequency_penalty);
  }
  if (s->presence_penalty != 0.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"presence_penalty\":%.4g", s->presence_penalty);
  }
  
  if (config && (config->api_type == API_TYPE_APHRODITE ||
                 config->api_type == API_TYPE_VLLM ||
                 config->api_type == API_TYPE_LLAMACPP ||
                 config->api_type == API_TYPE_TABBY)) {
    if (s->top_k > 0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"top_k\":%d", s->top_k);
    }
    if (s->min_p > 0.0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"min_p\":%.4g", s->min_p);
    }
    if (s->repetition_penalty != 1.0) {
      ensure_buffer_capacity(rb, 64);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"repetition_penalty\":%.4g", s->repetition_penalty);
    }
    if (s->typical_p != 1.0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"typical_p\":%.4g", s->typical_p);
    }
    if (s->tfs != 1.0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"tfs\":%.4g", s->tfs);
    }
    if (s->top_a > 0.0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"top_a\":%.4g", s->top_a);
    }
    if (s->min_tokens > 0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"min_tokens\":%d", s->min_tokens);
    }
  }
  
  if (config && config->api_type == API_TYPE_APHRODITE) {
    if (s->smoothing_factor > 0.0) {
      ensure_buffer_capacity(rb, 64);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"smoothing_factor\":%.4g", s->smoothing_factor);
    }
    if (s->dynatemp_min > 0.0 || s->dynatemp_max > 0.0) {
      ensure_buffer_capacity(rb, 128);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"dynatemp_min\":%.4g", s->dynatemp_min);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"dynatemp_max\":%.4g", s->dynatemp_max);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"dynatemp_exponent\":%.4g", s->dynatemp_exponent);
    }
    if (s->mirostat_mode > 0) {
      ensure_buffer_capacity(rb, 96);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"mirostat_mode\":%d", s->mirostat_mode);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"mirostat_tau\":%.4g", s->mirostat_tau);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"mirostat_eta\":%.4g", s->mirostat_eta);
    }
    if (s->dry_multiplier > 0.0) {
      ensure_buffer_capacity(rb, 128);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"dry_multiplier\":%.4g", s->dry_multiplier);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"dry_base\":%.4g", s->dry_base);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"dry_allowed_length\":%d", s->dry_allowed_length);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"dry_range\":%d", s->dry_range);
    }
    if (s->xtc_probability > 0.0) {
      ensure_buffer_capacity(rb, 64);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"xtc_threshold\":%.4g", s->xtc_threshold);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"xtc_probability\":%.4g", s->xtc_probability);
    }
    if (s->nsigma > 0.0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"nsigma\":%.4g", s->nsigma);
    }
    if (s->skew != 0.0) {
      ensure_buffer_capacity(rb, 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"skew\":%.4g", s->skew);
    }
  }
  
  for (int i = 0; i < s->custom_count; i++) {
    const CustomSampler *cs = &s->custom[i];
    if (cs->type == SAMPLER_TYPE_STRING) {
      char *escaped = escape_json_string(cs->str_value);
      ensure_buffer_capacity(rb, strlen(cs->name) + strlen(escaped ? escaped : cs->str_value) + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":\"%s\"", cs->name, escaped ? escaped : cs->str_value);
      free(escaped);
    } else if (cs->type == SAMPLER_TYPE_INT) {
      ensure_buffer_capacity(rb, strlen(cs->name) + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":%d", cs->name, (int)cs->value);
    } else if (cs->type == SAMPLER_TYPE_BOOL) {
      ensure_buffer_capacity(rb, strlen(cs->name) + 16);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":%s", cs->name, cs->value != 0 ? "true" : "false");
    } else if (cs->type == SAMPLER_TYPE_LIST_FLOAT || cs->type == SAMPLER_TYPE_LIST_INT) {
      ensure_buffer_capacity(rb, strlen(cs->name) + cs->list_count * 32 + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, ",\"%s\":[", cs->name);
      for (int j = 0; j < cs->list_count; j++) {
        if (cs->type == SAMPLER_TYPE_LIST_INT)
          *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                               "%d", (int)cs->list_values[j]);
        else
          *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                               "%.4g", cs->list_values[j]);
        if (j < cs->list_count - 1)
          *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, ",");
      }
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, "]");
    } else if (cs->type == SAMPLER_TYPE_LIST_STRING) {
      ensure_buffer_capacity(rb, strlen(cs->name) + cs->list_count * 128 + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, ",\"%s\":[", cs->name);
      for (int j = 0; j < cs->list_count; j++) {
        char *escaped = escape_json_string(cs->list_strings[j]);
        *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                             "\"%s\"", escaped ? escaped : cs->list_strings[j]);
        free(escaped);
        if (j < cs->list_count - 1)
          *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, ",");
      }
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, "]");
    } else if (cs->type == SAMPLER_TYPE_DICT) {
      ensure_buffer_capacity(rb, strlen(cs->name) + cs->dict_count * 128 + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, ",\"%s\":{", cs->name);
      for (int j = 0; j < cs->dict_count; j++) {
        const DictEntry *de = &cs->dict_entries[j];
        if (de->is_string) {
          char *escaped = escape_json_string(de->str_val);
          *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                               "\"%s\":\"%s\"", de->key, escaped ? escaped : de->str_val);
          free(escaped);
        } else {
          *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                               "\"%s\":%.4g", de->key, de->num_val);
        }
        if (j < cs->dict_count - 1)
          *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, ",");
      }
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos, "}");
    } else {
      ensure_buffer_capacity(rb, strlen(cs->name) + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":%.4g", cs->name, cs->value);
    }
  }
}

void add_sampler_settings_anthropic(RequestBuilder *rb, const SamplerSettings *s) {
  if (!rb || !rb->body || !rb->pos || !rb->cap || !s)
    return;
  
  if (s->temperature != 1.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"temperature\":%.4g", s->temperature);
  }
  if (s->top_p != 1.0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"top_p\":%.4g", s->top_p);
  }
  if (s->top_k > 0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"top_k\":%d", s->top_k);
  }
}

void add_sampler_settings_kobold(RequestBuilder *rb, const SamplerSettings *s) {
  if (!rb || !rb->body || !rb->pos || !rb->cap || !s)
    return;
  
  if (s->temperature != 1.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"temperature\":%.4g", s->temperature);
  }
  if (s->top_p != 1.0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"top_p\":%.4g", s->top_p);
  }
  if (s->frequency_penalty != 0.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"frequency_penalty\":%.4g", s->frequency_penalty);
  }
  if (s->presence_penalty != 0.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"presence_penalty\":%.4g", s->presence_penalty);
  }
  if (s->top_k > 0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"top_k\":%d", s->top_k);
  }
  if (s->min_p > 0.0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"min_p\":%.4g", s->min_p);
  }
  if (s->repetition_penalty != 1.0) {
    ensure_buffer_capacity(rb, 64);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"repetition_penalty\":%.4g", s->repetition_penalty);
  }
  if (s->typical_p != 1.0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"typical_p\":%.4g", s->typical_p);
  }
  if (s->tfs != 1.0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"tfs\":%.4g", s->tfs);
  }
  if (s->min_tokens > 0) {
    ensure_buffer_capacity(rb, 32);
    *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                         ",\"min_tokens\":%d", s->min_tokens);
  }
  
  for (int i = 0; i < s->custom_count; i++) {
    const CustomSampler *cs = &s->custom[i];
    if (cs->type == SAMPLER_TYPE_STRING) {
      char *escaped = escape_json_string(cs->str_value);
      ensure_buffer_capacity(rb, strlen(cs->name) + strlen(escaped ? escaped : cs->str_value) + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":\"%s\"", cs->name, escaped ? escaped : cs->str_value);
      free(escaped);
    } else if (cs->type == SAMPLER_TYPE_INT) {
      ensure_buffer_capacity(rb, strlen(cs->name) + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":%d", cs->name, (int)cs->value);
    } else if (cs->type == SAMPLER_TYPE_BOOL) {
      ensure_buffer_capacity(rb, strlen(cs->name) + 16);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":%s", cs->name, cs->value != 0 ? "true" : "false");
    } else {
      ensure_buffer_capacity(rb, strlen(cs->name) + 32);
      *rb->pos += snprintf(*rb->body + *rb->pos, *rb->cap - *rb->pos,
                           ",\"%s\":%.4g", cs->name, cs->value);
    }
  }
}

static char *load_attachment_content(const char *ref) {
  if (!ref || strncmp(ref, "[Attachment: ", 13) != 0)
    return NULL;

  const char *filename_start = ref + 13;
  const char *filename_end = strchr(filename_start, ']');
  if (!filename_end)
    return NULL;

  size_t filename_len = filename_end - filename_start;
  if (filename_len == 0 || filename_len >= 256)
    return NULL;

  const char *home = getenv("HOME");
  if (!home)
    return NULL;

  char filename[256];
  strncpy(filename, filename_start, filename_len);
  filename[filename_len] = '\0';

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/.config/sillytui/attachments/%s",
           home, filename);

  FILE *f = fopen(filepath, "r");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (len <= 0) {
    fclose(f);
    return NULL;
  }

  char *content = malloc(len + 1);
  if (!content) {
    fclose(f);
    return NULL;
  }

  size_t read_len = fread(content, 1, len, f);
  fclose(f);
  content[read_len] = '\0';

  return content;
}

char *expand_attachments(const char *content) {
  if (!content)
    return NULL;

  const char *attachment_start = strstr(content, "[Attachment: ");
  if (!attachment_start)
    return NULL;

  const char *attachment_end = strchr(attachment_start, ']');
  if (!attachment_end)
    return NULL;

  char *attachment_content = load_attachment_content(attachment_start);
  if (!attachment_content)
    return NULL;

  size_t before_len = attachment_start - content;
  size_t after_len = strlen(attachment_end + 1);
  size_t attachment_content_len = strlen(attachment_content);

  size_t total_len = before_len + attachment_content_len + after_len + 1;
  char *expanded = malloc(total_len);
  if (!expanded) {
    free(attachment_content);
    return NULL;
  }

  memcpy(expanded, content, before_len);
  memcpy(expanded + before_len, attachment_content, attachment_content_len);
  if (after_len > 0) {
    memcpy(expanded + before_len + attachment_content_len, attachment_end + 1,
           after_len);
  }
  expanded[total_len - 1] = '\0';

  free(attachment_content);
  return expanded;
}

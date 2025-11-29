#include "common.h"
#include "core/macros.h"
#include "tokenizer/selector.h"
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

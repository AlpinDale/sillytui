#include "backend.h"
#include "character/character.h"
#include "character/persona.h"
#include "core/config.h"
#include "core/macros.h"
#include "llm/common.h"
#include <curl/curl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANTHROPIC_DEFAULT_CONTEXT_LENGTH 200000

typedef struct {
  char *filename;
  char *mime_type;
  char *base64_data;
} ImageAttachment;

static char *load_image_attachment_base64(const char *filename,
                                          const char **out_mime_type) {
  const char *home = getenv("HOME");
  if (!home)
    return NULL;

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/.config/sillytui/attachments/%s",
           home, filename);

  FILE *f = fopen(filepath, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (len <= 0 || len > 10 * 1024 * 1024) {
    fclose(f);
    return NULL;
  }

  unsigned char *data = malloc(len);
  if (!data) {
    fclose(f);
    return NULL;
  }

  size_t read_len = fread(data, 1, len, f);
  fclose(f);

  if (read_len != (size_t)len) {
    free(data);
    return NULL;
  }

  char *ext = get_file_extension(filename);
  if (ext && out_mime_type) {
    *out_mime_type = get_image_mime_type(ext);
    free(ext);
  }

  char *base64 = base64_encode(data, read_len);
  free(data);
  return base64;
}

static char *build_message_content(const char *text, const char *char_name,
                                   const char *user_name, bool *has_images) {
  if (!text)
    return NULL;

  *has_images = false;
  bool found_image = false;
  const char *scan = text;
  while ((scan = strstr(scan, "[Attachment: ")) != NULL) {
    const char *end = strchr(scan, ']');
    if (!end)
      break;

    size_t filename_len = end - (scan + 13);
    if (filename_len > 0 && filename_len < 256) {
      char filename[256];
      strncpy(filename, scan + 13, filename_len);
      filename[filename_len] = '\0';

      if (is_image_file(filename)) {
        found_image = true;
        break;
      }
    }
    scan = end + 1;
  }

  if (!found_image) {
    char *substituted = macro_substitute(text, char_name, user_name);
    char *escaped = escape_json_string(substituted ? substituted : text);
    free(substituted);
    return escaped;
  }

  *has_images = true;
  StringBuilder sb;
  sb_init(&sb);
  sb_append(&sb, "[");

  const char *remaining = text;
  bool first_block = true;

  while (*remaining) {
    const char *attachment_start = strstr(remaining, "[Attachment: ");
    if (!attachment_start) {
      if (*remaining) {
        char *substituted = macro_substitute(remaining, char_name, user_name);
        char *escaped =
            escape_json_string(substituted ? substituted : remaining);
        free(substituted);
        if (escaped) {
          if (!first_block)
            sb_append(&sb, ",");
          sb_append(&sb, "{\"type\":\"text\",\"text\":\"");
          sb_append(&sb, escaped);
          sb_append(&sb, "\"}");
          free(escaped);
          first_block = false;
        }
      }
      break;
    }

    if (attachment_start > remaining) {
      size_t text_len = attachment_start - remaining;
      char *text_part = malloc(text_len + 1);
      if (text_part) {
        strncpy(text_part, remaining, text_len);
        text_part[text_len] = '\0';
        char *substituted = macro_substitute(text_part, char_name, user_name);
        char *escaped =
            escape_json_string(substituted ? substituted : text_part);
        free(substituted);
        free(text_part);
        if (escaped) {
          if (!first_block)
            sb_append(&sb, ",");
          sb_append(&sb, "{\"type\":\"text\",\"text\":\"");
          sb_append(&sb, escaped);
          sb_append(&sb, "\"}");
          free(escaped);
          first_block = false;
        }
      }
    }

    const char *attachment_end = strchr(attachment_start, ']');
    if (!attachment_end) {
      remaining = attachment_start + 13;
      continue;
    }

    size_t filename_len = attachment_end - (attachment_start + 13);
    if (filename_len > 0 && filename_len < 256) {
      char filename[256];
      strncpy(filename, attachment_start + 13, filename_len);
      filename[filename_len] = '\0';

      if (is_image_file(filename)) {
        const char *mime_type = NULL;
        char *base64 = load_image_attachment_base64(filename, &mime_type);
        if (base64 && mime_type) {
          if (!first_block)
            sb_append(&sb, ",");
          sb_append(&sb, "{\"type\":\"image\",\"source\":{\"type\":\"base64\","
                         "\"media_type\":\"");
          sb_append(&sb, mime_type);
          sb_append(&sb, "\",\"data\":\"");
          sb_append(&sb, base64);
          sb_append(&sb, "\"}}");
          free(base64);
          first_block = false;
        }
      }
    }

    remaining = attachment_end + 1;
  }

  sb_append(&sb, "]");
  return sb_finish(&sb);
}

static int anthropic_tokenize(const ModelConfig *config, const char *text) {
  (void)config;
  (void)text;
  return -1;
}

static char *anthropic_build_request(const ModelConfig *config,
                                     const ChatHistory *history,
                                     const LLMContext *context) {
  int context_length = config->context_length > 0
                           ? config->context_length
                           : ANTHROPIC_DEFAULT_CONTEXT_LENGTH;
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
  const AuthorNote *note = context ? context->author_note : NULL;
  bool note_before =
      note && note->text[0] && note->position == AN_POS_BEFORE_SCENARIO;
  bool note_after =
      note && note->text[0] && note->position == AN_POS_AFTER_SCENARIO;

  char *lore_ctx = NULL;
  if (context && context->lorebook) {
    lore_ctx = lorebook_build_context(context->lorebook, history, 0);
  }

  if (note_before || system_prompt || note_after || lore_ctx) {
    size_t sys_len = 0;
    if (note_before)
      sys_len += strlen(note->text) + 2;
    if (system_prompt)
      sys_len += strlen(system_prompt);
    if (lore_ctx)
      sys_len += strlen(lore_ctx) + 20;
    if (note_after)
      sys_len += strlen(note->text) + 2;
    char *combined = malloc(sys_len + 1);
    if (combined) {
      combined[0] = '\0';
      if (note_before) {
        strcat(combined, note->text);
        if (system_prompt || lore_ctx)
          strcat(combined, "\n\n");
      }
      if (system_prompt)
        strcat(combined, system_prompt);
      if (lore_ctx) {
        if (system_prompt || note_before)
          strcat(combined, "\n\n");
        strcat(combined, "[World Info]\n");
        strcat(combined, lore_ctx);
      }
      if (note_after) {
        if (system_prompt || note_before || lore_ctx)
          strcat(combined, "\n\n");
        strcat(combined, note->text);
      }
      char *escaped = escape_json_string(combined);
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
      free(combined);
    }
  }
  free(lore_ctx);
  if (system_prompt)
    free(system_prompt);

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

  bool note_in_chat = note && note->text[0] && note->position == AN_POS_IN_CHAT;
  size_t note_inject_idx = SIZE_MAX;
  if (note_in_chat && history->count > 0) {
    size_t from_end = (size_t)note->depth;
    if (from_end < history->count)
      note_inject_idx = history->count - from_end;
    else
      note_inject_idx = start_index;
  }

  for (size_t i = start_index; i < history->count; i++) {
    if (note_in_chat && i == note_inject_idx) {
      char *escaped_note = escape_json_string(note->text);
      if (escaped_note) {
        size_t needed = strlen(escaped_note) + 64;
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
                        "{\"role\":\"user\",\"content\":\"%s\"}", escaped_note);
        free(escaped_note);
      }
    }

    const char *msg = history_get(history, i);
    if (!msg)
      continue;

    MessageRole msg_role = history_get_role(history, i);
    const char *role =
        (msg_role == ROLE_SYSTEM) ? "user" : role_to_string(msg_role);
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

    bool has_images = false;
    char *content_json =
        build_message_content(content, char_name, user_name, &has_images);
    if (!content_json)
      continue;

    size_t needed = strlen(content_json) + 128;
    if (pos + needed >= cap) {
      cap = (pos + needed) * 2;
      char *tmp = realloc(body, cap);
      if (!tmp) {
        free(content_json);
        free(body);
        return NULL;
      }
      body = tmp;
    }

    if (!first)
      body[pos++] = ',';
    first = false;

    if (has_images) {
      pos += snprintf(body + pos, cap - pos, "{\"role\":\"%s\",\"content\":%s}",
                      role, content_json);
    } else {
      pos +=
          snprintf(body + pos, cap - pos,
                   "{\"role\":\"%s\",\"content\":\"%s\"}", role, content_json);
    }
    free(content_json);
  }

  if (context && context->character &&
      context->character->post_history_instructions &&
      context->character->post_history_instructions[0]) {
    bool has_images = false;
    char *content_json =
        build_message_content(context->character->post_history_instructions,
                              char_name, user_name, &has_images);
    if (content_json) {
      size_t needed = strlen(content_json) + 128;
      if (pos + needed >= cap) {
        cap = (pos + needed) * 2;
        char *tmp = realloc(body, cap);
        if (tmp)
          body = tmp;
      }
      if (!first)
        body[pos++] = ',';
      if (has_images) {
        pos += snprintf(body + pos, cap - pos,
                        "{\"role\":\"user\",\"content\":%s}", content_json);
      } else {
        pos += snprintf(body + pos, cap - pos,
                        "{\"role\":\"user\",\"content\":\"%s\"}", content_json);
      }
      free(content_json);
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

static void anthropic_parse_stream(StreamCtx *ctx, const char *line) {
  if (strncmp(line, "data: ", 6) != 0)
    return;
  const char *data = line + 6;

  const char *usage = strstr(data, "\"usage\"");
  if (usage) {
    int prompt = find_json_int(usage, "input_tokens");
    int completion = find_json_int(usage, "output_tokens");
    if (prompt > 0)
      ctx->prompt_tokens = prompt;
    if (completion > 0)
      ctx->completion_tokens = completion;
  }

  if (strstr(data, "\"content_block_delta\"") ||
      strstr(data, "\"text_delta\"")) {
    const char *delta = strstr(data, "\"delta\"");
    if (delta) {
      char *content = find_json_string(delta, "text");
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
  }
}

static void anthropic_add_headers(void *curl_handle,
                                  const ModelConfig *config) {
  CURL *curl = curl_handle;
  struct curl_slist *headers = NULL;

  headers = curl_slist_append(headers, "Content-Type: application/json");

  char api_key_header[512];
  snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s",
           config->api_key);
  headers = curl_slist_append(headers, api_key_header);

  headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
}

const LLMBackend backend_anthropic = {
    .tokenize = anthropic_tokenize,
    .build_request = anthropic_build_request,
    .parse_stream = anthropic_parse_stream,
    .add_headers = anthropic_add_headers,
};

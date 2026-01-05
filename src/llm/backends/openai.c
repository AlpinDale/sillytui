#include "backend.h"
#include "character/character.h"
#include "character/persona.h"
#include "core/config.h"
#include "core/macros.h"
#include "core/platform.h"
#include "llm/common.h"
#include <curl/curl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *load_image_attachment_base64(const char *filename,
                                          const char **out_mime_type) {
  const char *home = get_home_dir();
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
          sb_append(&sb,
                    "{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:");
          sb_append(&sb, mime_type);
          sb_append(&sb, ";base64,");
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

static int openai_tokenize(const ModelConfig *config, const char *text) {
  (void)config;
  (void)text;
  return -1;
}

static char *openai_build_request(const ModelConfig *config,
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
  const AuthorNote *note = context ? context->author_note : NULL;
  bool note_before =
      note && note->text[0] && note->position == AN_POS_BEFORE_SCENARIO;

  if (note_before && note->text[0]) {
    char *escaped = escape_json_string(note->text);
    if (escaped) {
      const char *role = author_note_role_to_string(note->role);
      size_t needed = strlen(escaped) + 64;
      if (pos + needed >= cap) {
        cap = (pos + needed) * 2;
        char *tmp = realloc(body, cap);
        if (tmp)
          body = tmp;
      }
      pos += snprintf(body + pos, cap - pos,
                      "{\"role\":\"%s\",\"content\":\"%s\"}", role, escaped);
      first = false;
      free(escaped);
    }
  }

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
      if (!first)
        body[pos++] = ',';
      pos += snprintf(body + pos, cap - pos,
                      "{\"role\":\"system\",\"content\":\"%s\"}", escaped);
      first = false;
      free(escaped);
    }
    free(system_prompt);
  }

  if (context && context->lorebook) {
    char *lore_ctx = lorebook_build_context(context->lorebook, history, 0);
    if (lore_ctx && lore_ctx[0]) {
      char *escaped = escape_json_string(lore_ctx);
      if (escaped) {
        size_t needed = strlen(escaped) + 128;
        if (pos + needed >= cap) {
          cap = (pos + needed) * 2;
          char *tmp = realloc(body, cap);
          if (tmp)
            body = tmp;
        }
        if (!first)
          body[pos++] = ',';
        pos += snprintf(
            body + pos, cap - pos,
            "{\"role\":\"system\",\"content\":\"[World Info]\\n%s\"}", escaped);
        first = false;
        free(escaped);
      }
      free(lore_ctx);
    }
  }

  bool note_after =
      note && note->text[0] && note->position == AN_POS_AFTER_SCENARIO;
  if (note_after) {
    char *escaped = escape_json_string(note->text);
    if (escaped) {
      const char *role = author_note_role_to_string(note->role);
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
                      "{\"role\":\"%s\",\"content\":\"%s\"}", role, escaped);
      first = false;
      free(escaped);
    }
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

  bool note_in_chat = note && note->text[0] && note->position == AN_POS_IN_CHAT;
  size_t note_inject_index = SIZE_MAX;
  if (note_in_chat && history->count > 0) {
    size_t from_end = (size_t)note->depth;
    if (from_end < history->count)
      note_inject_index = history->count - from_end;
    else
      note_inject_index = start_index;
  }

  for (size_t i = start_index; i < history->count; i++) {
    if (note_in_chat && i == note_inject_index) {
      char *escaped_note = escape_json_string(note->text);
      if (escaped_note) {
        const char *note_role = author_note_role_to_string(note->role);
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
                        "{\"role\":\"%s\",\"content\":\"%s\"}", note_role,
                        escaped_note);
        free(escaped_note);
      }
    }

    const char *msg = history_get(history, i);
    if (!msg)
      continue;

    MessageRole msg_role = history_get_role(history, i);
    const char *role = role_to_string(msg_role);
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

    if (!first) {
      body[pos++] = ',';
    }
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
                        "{\"role\":\"system\",\"content\":%s}", content_json);
      } else {
        pos +=
            snprintf(body + pos, cap - pos,
                     "{\"role\":\"system\",\"content\":\"%s\"}", content_json);
      }
      free(content_json);
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

static void openai_parse_stream(StreamCtx *ctx, const char *line) {
  if (strncmp(line, "data: ", 6) != 0)
    return;
  const char *data = line + 6;

  if (strcmp(data, "[DONE]") == 0)
    return;

  const char *usage = strstr(data, "\"usage\"");
  if (usage) {
    int prompt = find_json_int(usage, "prompt_tokens");
    int completion = find_json_int(usage, "completion_tokens");
    if (prompt > 0)
      ctx->prompt_tokens = prompt;
    if (completion > 0)
      ctx->completion_tokens = completion;
  }

  const char *choices = strstr(data, "\"choices\"");
  if (!choices)
    return;

  char *finish_reason = find_json_string(choices, "finish_reason");
  if (finish_reason && finish_reason[0] && ctx->resp) {
    strncpy(ctx->resp->finish_reason, finish_reason,
            sizeof(ctx->resp->finish_reason) - 1);
    ctx->resp->finish_reason[sizeof(ctx->resp->finish_reason) - 1] = '\0';
    free(finish_reason);
  } else if (finish_reason) {
    free(finish_reason);
  }

  const char *delta = strstr(choices, "\"delta\"");
  if (!delta)
    return;

  char *reasoning = find_json_string(delta, "reasoning_content");
  if (reasoning && reasoning[0]) {
    if (!ctx->in_reasoning) {
      ctx->in_reasoning = true;
      gettimeofday(&ctx->reasoning_start_time, NULL);
    }
    if (!ctx->has_first_token) {
      gettimeofday(&ctx->first_token_time, NULL);
      ctx->has_first_token = true;
    }
    gettimeofday(&ctx->last_token_time, NULL);
    append_to_reasoning(ctx->resp, reasoning, strlen(reasoning));
    if (ctx->reasoning_cb) {
      struct timeval now;
      gettimeofday(&now, NULL);
      double elapsed_ms =
          (now.tv_sec - ctx->reasoning_start_time.tv_sec) * 1000.0 +
          (now.tv_usec - ctx->reasoning_start_time.tv_usec) / 1000.0;
      ctx->reasoning_cb(reasoning, elapsed_ms, ctx->userdata);
    }
    free(reasoning);
    return;
  }

  char *content = find_json_string(delta, "content");
  if (content && content[0]) {
    if (ctx->in_reasoning) {
      struct timeval now;
      gettimeofday(&now, NULL);
      ctx->resp->reasoning_ms =
          (now.tv_sec - ctx->reasoning_start_time.tv_sec) * 1000.0 +
          (now.tv_usec - ctx->reasoning_start_time.tv_usec) / 1000.0;
      ctx->in_reasoning = false;
    }
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

static void openai_add_headers(void *curl_handle, const ModelConfig *config) {
  CURL *curl = curl_handle;
  struct curl_slist *headers = NULL;

  headers = curl_slist_append(headers, "Content-Type: application/json");

  char auth[512];
  snprintf(auth, sizeof(auth), "Authorization: Bearer %s", config->api_key);
  headers = curl_slist_append(headers, auth);

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
}

const LLMBackend backend_openai = {
    .tokenize = openai_tokenize,
    .build_request = openai_build_request,
    .parse_stream = openai_parse_stream,
    .add_headers = openai_add_headers,
};

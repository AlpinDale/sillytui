#include "backend.h"
#include "character/character.h"
#include "character/persona.h"
#include "core/config.h"
#include "core/macros.h"
#include "llm/common.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int kobold_tokenize(const ModelConfig *config, const char *text) {
  (void)config;
  (void)text;
  return -1;
}

static char *kobold_build_request(const ModelConfig *config,
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

    char *substituted = macro_substitute(content, char_name, user_name);
    char *escaped = escape_json_string(substituted ? substituted : content);
    free(substituted);
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
    if (s->top_k > 0)
      pos += snprintf(body + pos, cap - pos, ",\"top_k\":%d", s->top_k);
    if (s->min_p > 0.0)
      pos += snprintf(body + pos, cap - pos, ",\"min_p\":%.4g", s->min_p);
    if (s->repetition_penalty != 1.0)
      pos += snprintf(body + pos, cap - pos, ",\"repetition_penalty\":%.4g",
                      s->repetition_penalty);
    if (s->typical_p != 1.0)
      pos +=
          snprintf(body + pos, cap - pos, ",\"typical_p\":%.4g", s->typical_p);
    if (s->tfs != 1.0)
      pos += snprintf(body + pos, cap - pos, ",\"tfs\":%.4g", s->tfs);
    if (s->min_tokens > 0)
      pos +=
          snprintf(body + pos, cap - pos, ",\"min_tokens\":%d", s->min_tokens);

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

static void kobold_parse_stream(StreamCtx *ctx, const char *line) {
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

static void kobold_add_headers(void *curl_handle, const ModelConfig *config) {
  CURL *curl = curl_handle;
  struct curl_slist *headers = NULL;

  headers = curl_slist_append(headers, "Content-Type: application/json");

  if (config->api_key[0]) {
    char auth[512];
    snprintf(auth, sizeof(auth), "Authorization: Bearer %s", config->api_key);
    headers = curl_slist_append(headers, auth);
  }

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
}

const LLMBackend backend_kobold = {
    .tokenize = kobold_tokenize,
    .build_request = kobold_build_request,
    .parse_stream = kobold_parse_stream,
    .add_headers = kobold_add_headers,
};

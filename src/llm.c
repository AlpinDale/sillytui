#include "llm.h"
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

static char *build_request_body(const char *model_id,
                                const ChatHistory *history) {
  size_t cap = 4096;
  char *body = malloc(cap);
  if (!body)
    return NULL;

  size_t pos = 0;
  pos += snprintf(body + pos, cap - pos, "{\"model\":\"%s\",\"messages\":[",
                  model_id);

  bool first = true;
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

  pos += snprintf(body + pos, cap - pos, "],\"stream\":true}");
  return body;
}

LLMResponse llm_chat(const ModelConfig *config, const ChatHistory *history,
                     LLMStreamCallback stream_cb,
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

  char *body = build_request_body(config->model_id, history);
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

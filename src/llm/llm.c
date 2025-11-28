#include "llm/llm.h"
#include "llm/backends/backend.h"
#include "llm/common.h"
#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

void llm_init(void) { curl_global_init(CURL_GLOBAL_DEFAULT); }

void llm_cleanup(void) { curl_global_cleanup(); }

int llm_estimate_tokens(const char *text) {
  if (!text)
    return 0;
  return (int)(strlen(text) / 4);
}

const LLMBackend *backend_get(ApiType type) {
  switch (type) {
  case API_TYPE_ANTHROPIC:
    return &backend_anthropic;
  case API_TYPE_KOBOLDCPP:
    return &backend_kobold;
  case API_TYPE_OPENAI:
  case API_TYPE_APHRODITE:
  case API_TYPE_VLLM:
  case API_TYPE_LLAMACPP:
  case API_TYPE_TABBY:
  default:
    return &backend_openai;
  }
}

static size_t tokenize_write_callback(char *ptr, size_t size, size_t nmemb,
                                      void *userdata) {
  char **resp = userdata;
  size_t total = size * nmemb;

  char *new_resp = realloc(*resp, (*resp ? strlen(*resp) : 0) + total + 1);
  if (!new_resp)
    return 0;

  if (!*resp)
    new_resp[0] = '\0';
  *resp = new_resp;
  strncat(*resp, ptr, total);
  return total;
}

static int parse_token_count(const char *json) {
  int count = find_json_int(json, "count");
  if (count >= 0)
    return count;

  count = find_json_int(json, "length");
  if (count >= 0)
    return count;

  count = find_json_int(json, "input_tokens");
  if (count >= 0)
    return count;

  const char *tokens = strstr(json, "\"tokens\"");
  if (tokens) {
    const char *arr = strchr(tokens, '[');
    if (arr) {
      int token_count = 0;
      const char *p = arr + 1;
      while (*p && *p != ']') {
        if (*p == ',' || (*p >= '0' && *p <= '9'))
          if (*p == ',')
            token_count++;
        p++;
      }
      if (p > arr + 1)
        token_count++;
      return token_count;
    }
  }
  return -1;
}

int llm_tokenize(const ModelConfig *config, const char *text) {
  if (!config || !text || !config->base_url[0])
    return -1;

  const char *base = config->base_url;
  size_t base_len = strlen(base);
  while (base_len > 0 && base[base_len - 1] == '/')
    base_len--;

  char base_no_v1[256];
  snprintf(base_no_v1, sizeof(base_no_v1), "%.*s", (int)base_len, base);
  if (base_len >= 3 && strcmp(base_no_v1 + base_len - 3, "/v1") == 0)
    base_no_v1[base_len - 3] = '\0';

  char url[512];
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
    return llm_estimate_tokens(text);
  }

  char *escaped_text = escape_json_string(text);
  if (!escaped_text)
    return llm_estimate_tokens(text);

  size_t body_size = strlen(escaped_text) + 256;
  char *body = malloc(body_size);
  if (!body) {
    free(escaped_text);
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
    free(body);
    free(escaped_text);
    return llm_estimate_tokens(text);
  }
  free(escaped_text);

  CURL *curl = curl_easy_init();
  if (!curl) {
    free(body);
    return llm_estimate_tokens(text);
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

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  free(body);
  free(response);

  if (token_count < 0)
    return llm_estimate_tokens(text);
  return token_count;
}

void process_sse_line(StreamCtx *ctx, const char *line, bool is_anthropic) {
  const LLMBackend *backend =
      is_anthropic ? &backend_anthropic : &backend_openai;
  backend->parse_stream(ctx, line);
}

LLMResponse llm_chat(const ModelConfig *config, const ChatHistory *history,
                     const LLMContext *context, LLMStreamCallback stream_cb,
                     LLMProgressCallback progress_cb, void *userdata) {
  LLMResponse resp = {0};

  if (!config || !config->base_url[0] || !config->model_id[0]) {
    snprintf(resp.error, sizeof(resp.error), "No model configured");
    return resp;
  }

  const LLMBackend *backend = backend_get(config->api_type);

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

  char *body = backend->build_request(config, history, context);
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

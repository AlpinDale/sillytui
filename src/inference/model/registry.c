#include "inference/model/registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static model_entry_t g_registry[MODEL_REGISTRY_MAX];
static int g_registry_count = 0;

int model_register(const char *name, const inference_model_ops_t *ops) {
  if (!name || !ops)
    return -1;
  if (g_registry_count >= MODEL_REGISTRY_MAX)
    return -1;

  for (int i = 0; i < g_registry_count; i++) {
    if (strcmp(g_registry[i].name, name) == 0)
      return 0;
  }

  g_registry[g_registry_count].name = name;
  g_registry[g_registry_count].ops = ops;
  g_registry_count++;
  return 0;
}

const inference_model_ops_t *model_get_by_name(const char *name) {
  if (!name)
    return NULL;

  for (int i = 0; i < g_registry_count; i++) {
    if (strcmp(g_registry[i].name, name) == 0)
      return g_registry[i].ops;
  }
  return NULL;
}

static char *read_file_contents(const char *path, size_t *out_size) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (size <= 0 || size > 1024 * 1024) {
    fclose(f);
    return NULL;
  }

  char *buf = (char *)malloc(size + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t read = fread(buf, 1, size, f);
  fclose(f);

  if (read != (size_t)size) {
    free(buf);
    return NULL;
  }

  buf[size] = '\0';
  if (out_size)
    *out_size = size;
  return buf;
}

static const char *find_json_string(const char *json, const char *key) {
  static char value_buf[256];
  char search[128];
  snprintf(search, sizeof(search), "\"%s\"", key);

  const char *pos = strstr(json, search);
  if (!pos)
    return NULL;

  pos += strlen(search);
  while (*pos && (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\n'))
    pos++;

  if (*pos != '"')
    return NULL;
  pos++;

  size_t i = 0;
  while (*pos && *pos != '"' && i < sizeof(value_buf) - 1) {
    value_buf[i++] = *pos++;
  }
  value_buf[i] = '\0';
  return value_buf;
}

const char *model_detect_arch(const char *model_dir) {
  if (!model_dir)
    return NULL;

  char config_path[512];
  snprintf(config_path, sizeof(config_path), "%s/config.json", model_dir);

  char *json = read_file_contents(config_path, NULL);
  if (!json)
    return NULL;

  const char *arch = find_json_string(json, "model_type");
  if (!arch) {
    const char *archs = find_json_string(json, "architectures");
    if (archs) {
      if (strstr(archs, "Qwen") || strstr(archs, "qwen"))
        arch = "qwen3";
      else if (strstr(archs, "Llama") || strstr(archs, "llama"))
        arch = "llama";
      else if (strstr(archs, "Mistral") || strstr(archs, "mistral"))
        arch = "mistral";
      else if (strstr(archs, "GPT") || strstr(archs, "gpt"))
        arch = "gpt";
    }
  }

  static char result[64];
  if (arch) {
    strncpy(result, arch, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';

    for (char *p = result; *p; p++) {
      if (*p >= 'A' && *p <= 'Z')
        *p = *p - 'A' + 'a';
    }
  }

  free(json);
  return arch ? result : NULL;
}

int model_registry_count(void) { return g_registry_count; }

int model_registry_list(const model_entry_t **out_list) {
  if (out_list)
    *out_list = g_registry;
  return g_registry_count;
}

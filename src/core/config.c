#include "core/config.h"
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *API_TYPE_NAMES[] = {"openai",   "aphrodite", "vllm",
                                       "llamacpp", "koboldcpp", "tabby",
                                       "anthropic"};

const char *api_type_name(ApiType type) {
  if (type >= 0 && type < API_TYPE_COUNT)
    return API_TYPE_NAMES[type];
  return "openai";
}

ApiType api_type_from_name(const char *name) {
  if (!name || !name[0])
    return API_TYPE_APHRODITE;
  for (int i = 0; i < API_TYPE_COUNT; i++) {
    if (strcasecmp(name, API_TYPE_NAMES[i]) == 0)
      return (ApiType)i;
  }
  return API_TYPE_APHRODITE;
}

static bool get_config_path(char *buf, size_t bufsize) {
  const char *home = getenv("HOME");
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }
  if (!home)
    return false;
  snprintf(buf, bufsize, "%s/.config/sillytui", home);
  return true;
}

static bool get_models_path(char *buf, size_t bufsize) {
  char dir[500];
  if (!get_config_path(dir, sizeof(dir)))
    return false;
  int written = snprintf(buf, bufsize, "%s/models.json", dir);
  return written > 0 && (size_t)written < bufsize;
}

bool config_ensure_dir(void) {
  char path[512];
  if (!get_config_path(path, sizeof(path)))
    return false;

  char tmp[512];
  snprintf(tmp, sizeof(tmp), "%s", path);

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  mkdir(tmp, 0755);
  return true;
}

static char *skip_ws(char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    p++;
  return p;
}

static char *parse_string(char *p, char *out, size_t outsize) {
  p = skip_ws(p);
  if (*p != '"')
    return NULL;
  p++;
  size_t i = 0;
  while (*p && *p != '"' && i < outsize - 1) {
    if (*p == '\\' && *(p + 1)) {
      p++;
      if (*p == 'n')
        out[i++] = '\n';
      else if (*p == 't')
        out[i++] = '\t';
      else
        out[i++] = *p;
      p++;
    } else {
      out[i++] = *p++;
    }
  }
  out[i] = '\0';
  if (*p == '"')
    p++;
  return p;
}

static void write_escaped(FILE *f, const char *s) {
  fputc('"', f);
  while (*s) {
    if (*s == '"' || *s == '\\')
      fputc('\\', f);
    else if (*s == '\n') {
      fputs("\\n", f);
      s++;
      continue;
    } else if (*s == '\t') {
      fputs("\\t", f);
      s++;
      continue;
    }
    fputc(*s++, f);
  }
  fputc('"', f);
}

bool config_load_models(ModelsFile *mf) {
  if (!mf)
    return false;
  memset(mf, 0, sizeof(*mf));
  mf->active_index = -1;

  char path[512];
  if (!get_models_path(path, sizeof(path)))
    return false;

  FILE *f = fopen(path, "r");
  if (!f)
    return true;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *data = malloc(len + 1);
  if (!data) {
    fclose(f);
    return false;
  }
  size_t read_len = fread(data, 1, len, f);
  data[read_len] = '\0';
  fclose(f);

  char *p = data;
  p = skip_ws(p);
  if (*p != '{') {
    free(data);
    return false;
  }
  p++;

  while (*p) {
    p = skip_ws(p);
    if (*p == '}')
      break;
    if (*p == ',') {
      p++;
      continue;
    }

    char key[64];
    p = parse_string(p, key, sizeof(key));
    if (!p)
      break;
    p = skip_ws(p);
    if (*p == ':')
      p++;
    p = skip_ws(p);

    if (strcmp(key, "active") == 0) {
      mf->active_index = atoi(p);
      while (*p && *p != ',' && *p != '}')
        p++;
    } else if (strcmp(key, "models") == 0) {
      if (*p != '[')
        break;
      p++;
      while (*p) {
        p = skip_ws(p);
        if (*p == ']') {
          p++;
          break;
        }
        if (*p == ',') {
          p++;
          continue;
        }
        if (*p != '{')
          break;
        p++;

        ModelConfig *m = &mf->models[mf->count];
        memset(m, 0, sizeof(*m));
        m->context_length = DEFAULT_CONTEXT_LENGTH;

        while (*p) {
          p = skip_ws(p);
          if (*p == '}') {
            p++;
            break;
          }
          if (*p == ',') {
            p++;
            continue;
          }

          char mkey[64];
          p = parse_string(p, mkey, sizeof(mkey));
          if (!p)
            break;
          p = skip_ws(p);
          if (*p == ':')
            p++;

          if (strcmp(mkey, "name") == 0) {
            p = parse_string(p, m->name, sizeof(m->name));
          } else if (strcmp(mkey, "base_url") == 0) {
            p = parse_string(p, m->base_url, sizeof(m->base_url));
          } else if (strcmp(mkey, "api_key") == 0) {
            p = parse_string(p, m->api_key, sizeof(m->api_key));
          } else if (strcmp(mkey, "model_id") == 0) {
            p = parse_string(p, m->model_id, sizeof(m->model_id));
          } else if (strcmp(mkey, "context_length") == 0) {
            p = skip_ws(p);
            m->context_length = atoi(p);
            if (m->context_length <= 0)
              m->context_length = DEFAULT_CONTEXT_LENGTH;
            while (*p && *p != ',' && *p != '}')
              p++;
          } else if (strcmp(mkey, "api_type") == 0) {
            char type_str[32] = {0};
            p = parse_string(p, type_str, sizeof(type_str));
            m->api_type = api_type_from_name(type_str);
          }
          if (!p)
            break;
        }
        if (m->name[0])
          mf->count++;
        if (mf->count >= MAX_MODELS)
          break;
      }
    }
  }

  free(data);
  return true;
}

bool config_save_models(const ModelsFile *mf) {
  if (!mf || !config_ensure_dir())
    return false;

  char path[512];
  if (!get_models_path(path, sizeof(path)))
    return false;

  FILE *f = fopen(path, "w");
  if (!f)
    return false;

  fprintf(f, "{\n  \"active\": %d,\n  \"models\": [\n", mf->active_index);

  for (size_t i = 0; i < mf->count; i++) {
    const ModelConfig *m = &mf->models[i];
    fprintf(f, "    {\n");
    fprintf(f, "      \"name\": ");
    write_escaped(f, m->name);
    fprintf(f, ",\n");
    fprintf(f, "      \"base_url\": ");
    write_escaped(f, m->base_url);
    fprintf(f, ",\n");
    fprintf(f, "      \"api_key\": ");
    write_escaped(f, m->api_key);
    fprintf(f, ",\n");
    fprintf(f, "      \"model_id\": ");
    write_escaped(f, m->model_id);
    fprintf(f, ",\n");
    fprintf(f, "      \"context_length\": %d,\n",
            m->context_length > 0 ? m->context_length : DEFAULT_CONTEXT_LENGTH);
    fprintf(f, "      \"api_type\": \"%s\"\n", api_type_name(m->api_type));
    fprintf(f, "    }%s\n", (i < mf->count - 1) ? "," : "");
  }

  fprintf(f, "  ]\n}\n");
  fclose(f);
  return true;
}

bool config_add_model(ModelsFile *mf, const ModelConfig *model) {
  if (!mf || !model || mf->count >= MAX_MODELS)
    return false;
  mf->models[mf->count++] = *model;
  return true;
}

bool config_remove_model(ModelsFile *mf, size_t index) {
  if (!mf || index >= mf->count)
    return false;
  for (size_t i = index; i < mf->count - 1; i++) {
    mf->models[i] = mf->models[i + 1];
  }
  mf->count--;
  if (mf->active_index == (int)index)
    mf->active_index = -1;
  else if (mf->active_index > (int)index)
    mf->active_index--;
  return true;
}

ModelConfig *config_get_active(ModelsFile *mf) {
  if (!mf || mf->active_index < 0 || mf->active_index >= (int)mf->count)
    return NULL;
  return &mf->models[mf->active_index];
}

void config_set_active(ModelsFile *mf, size_t index) {
  if (!mf)
    return;
  if (index < mf->count) {
    mf->active_index = (int)index;
  }
}

static const char *get_settings_path(void) {
  static char path[512] = {0};
  if (path[0] == '\0') {
    const char *home = getenv("HOME");
    if (home) {
      snprintf(path, sizeof(path), "%s/.config/sillytui/settings.json", home);
    }
  }
  return path;
}

bool config_load_settings(AppSettings *settings) {
  memset(settings, 0, sizeof(*settings));
  settings->paste_attachment_threshold = 1000;

  const char *path = get_settings_path();
  FILE *f = fopen(path, "r");
  if (!f)
    return false;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (len <= 0) {
    fclose(f);
    return false;
  }

  char *buf = malloc(len + 1);
  if (!buf) {
    fclose(f);
    return false;
  }

  size_t read_len = fread(buf, 1, len, f);
  fclose(f);
  buf[read_len] = '\0';

  if (strstr(buf, "\"skip_exit_confirm\":true") ||
      strstr(buf, "\"skip_exit_confirm\": true")) {
    settings->skip_exit_confirm = true;
  }

  const char *threshold_key = "\"paste_attachment_threshold\"";
  const char *threshold_pos = strstr(buf, threshold_key);
  if (threshold_pos) {
    const char *colon = strchr(threshold_pos, ':');
    if (colon) {
      int threshold = atoi(colon + 1);
      if (threshold > 0) {
        settings->paste_attachment_threshold = threshold;
      }
    }
  }

  free(buf);
  return true;
}

bool config_save_settings(const AppSettings *settings) {
  config_ensure_dir();
  const char *path = get_settings_path();
  FILE *f = fopen(path, "w");
  if (!f)
    return false;

  fprintf(f, "{\n");
  fprintf(f, "  \"skip_exit_confirm\": %s,\n",
          settings->skip_exit_confirm ? "true" : "false");
  fprintf(f, "  \"paste_attachment_threshold\": %d\n",
          settings->paste_attachment_threshold);
  fprintf(f, "}\n");

  fclose(f);
  return true;
}

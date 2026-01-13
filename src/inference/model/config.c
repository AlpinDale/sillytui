#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *skip_ws(const char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    p++;
  return (char *)p;
}

static char *parse_json_string(const char **pp, char *out, size_t outsize) {
  const char *p = skip_ws(*pp);
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
      else if (*p == 'r')
        out[i++] = '\r';
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
  *pp = p;
  return (char *)p;
}

static int parse_json_int(const char **pp) {
  const char *p = skip_ws(*pp);
  int sign = 1;
  if (*p == '-') {
    sign = -1;
    p++;
  }
  int val = 0;
  while (*p >= '0' && *p <= '9') {
    val = val * 10 + (*p - '0');
    p++;
  }
  *pp = p;
  return val * sign;
}

static float parse_json_float(const char **pp) {
  const char *p = skip_ws(*pp);
  char *end;
  float val = strtof(p, &end);
  *pp = end;
  return val;
}

static bool parse_json_bool(const char **pp) {
  const char *p = skip_ws(*pp);
  if (strncmp(p, "true", 4) == 0) {
    *pp = p + 4;
    return true;
  }
  if (strncmp(p, "false", 5) == 0) {
    *pp = p + 5;
    return false;
  }
  return false;
}

static const char *find_key(const char *json, const char *key) {
  char search[128];
  snprintf(search, sizeof(search), "\"%s\"", key);
  const char *p = strstr(json, search);
  if (!p)
    return NULL;
  p += strlen(search);
  p = skip_ws(p);
  if (*p == ':')
    p++;
  return skip_ws(p);
}

static char *read_file_contents(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *data = malloc(len + 1);
  if (!data) {
    fclose(f);
    return NULL;
  }

  size_t read_len = fread(data, 1, len, f);
  data[read_len] = '\0';
  fclose(f);
  return data;
}

activation_type_t activation_from_string(const char *name) {
  if (!name)
    return ACT_UNKNOWN;
  if (strcmp(name, "silu") == 0 || strcmp(name, "swish") == 0)
    return ACT_SILU;
  if (strcmp(name, "gelu") == 0)
    return ACT_GELU;
  if (strcmp(name, "gelu_tanh") == 0 || strcmp(name, "gelu_new") == 0 ||
      strcmp(name, "gelu_pytorch_tanh") == 0)
    return ACT_GELU_TANH;
  if (strcmp(name, "gelu_quick") == 0 || strcmp(name, "gelu_fast") == 0)
    return ACT_GELU_QUICK;
  if (strcmp(name, "relu") == 0)
    return ACT_RELU;
  return ACT_UNKNOWN;
}

const char *activation_to_string(activation_type_t act) {
  switch (act) {
  case ACT_SILU:
    return "silu";
  case ACT_GELU:
    return "gelu";
  case ACT_GELU_TANH:
    return "gelu_tanh";
  case ACT_GELU_QUICK:
    return "gelu_quick";
  case ACT_RELU:
    return "relu";
  default:
    return "unknown";
  }
}

bool model_config_load(model_config_t *config, const char *config_path) {
  if (!config || !config_path)
    return false;

  memset(config, 0, sizeof(*config));
  config->norm_type = NORM_RMS;
  config->hidden_act = ACT_SILU;
  config->tie_word_embeddings = true;

  char *data = read_file_contents(config_path);
  if (!data)
    return false;

  const char *p;
  char str_buf[64];

  p = find_key(data, "model_type");
  if (p && *p == '"')
    parse_json_string(&p, config->model_type, sizeof(config->model_type));

  p = find_key(data, "hidden_size");
  if (p)
    config->hidden_size = parse_json_int(&p);

  p = find_key(data, "num_attention_heads");
  if (p)
    config->num_attention_heads = parse_json_int(&p);

  p = find_key(data, "num_key_value_heads");
  if (p)
    config->num_key_value_heads = parse_json_int(&p);
  else
    config->num_key_value_heads = config->num_attention_heads;

  p = find_key(data, "num_hidden_layers");
  if (p)
    config->num_hidden_layers = parse_json_int(&p);

  p = find_key(data, "intermediate_size");
  if (p)
    config->intermediate_size = parse_json_int(&p);

  p = find_key(data, "vocab_size");
  if (p)
    config->vocab_size = parse_json_int(&p);

  p = find_key(data, "max_position_embeddings");
  if (p)
    config->max_position_embeddings = parse_json_int(&p);

  p = find_key(data, "head_dim");
  if (p)
    config->head_dim = parse_json_int(&p);

  p = find_key(data, "rope_theta");
  if (p)
    config->rope_theta = parse_json_float(&p);
  else
    config->rope_theta = 10000.0f;

  p = find_key(data, "rms_norm_eps");
  if (p)
    config->norm_eps = parse_json_float(&p);
  else {
    p = find_key(data, "layer_norm_eps");
    if (p) {
      config->norm_eps = parse_json_float(&p);
      config->norm_type = NORM_LAYER;
    } else {
      config->norm_eps = 1e-6f;
    }
  }

  p = find_key(data, "hidden_act");
  if (p && *p == '"') {
    parse_json_string(&p, str_buf, sizeof(str_buf));
    config->hidden_act = activation_from_string(str_buf);
  }

  p = find_key(data, "attention_bias");
  if (p)
    config->attention_bias = parse_json_bool(&p);

  p = find_key(data, "mlp_bias");
  if (p)
    config->mlp_bias = parse_json_bool(&p);

  p = find_key(data, "bos_token_id");
  if (p)
    config->bos_token_id = parse_json_int(&p);

  p = find_key(data, "eos_token_id");
  if (p)
    config->eos_token_id = parse_json_int(&p);

  p = find_key(data, "tie_word_embeddings");
  if (p)
    config->tie_word_embeddings = parse_json_bool(&p);

  if (config->head_dim == 0 && config->hidden_size > 0 &&
      config->num_attention_heads > 0) {
    config->head_dim = config->hidden_size / config->num_attention_heads;
  }

  free(data);
  return config->hidden_size > 0 && config->num_attention_heads > 0 &&
         config->num_hidden_layers > 0;
}

void model_config_free(model_config_t *config) {
  if (config)
    memset(config, 0, sizeof(*config));
}

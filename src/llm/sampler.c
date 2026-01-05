#include "llm/sampler.h"
#include "core/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <pwd.h>
#endif

void sampler_init_defaults(SamplerSettings *s) {
  if (!s)
    return;
  memset(s, 0, sizeof(*s));
  s->temperature = 1.0;
  s->top_p = 1.0;
  s->top_k = -1;
  s->min_p = 0.0;
  s->frequency_penalty = 0.0;
  s->presence_penalty = 0.0;
  s->repetition_penalty = 1.0;
  s->typical_p = 1.0;
  s->tfs = 1.0;
  s->top_a = 0.0;
  s->smoothing_factor = 0.0;
  s->min_tokens = 0;
  s->max_tokens = 512;
  s->dynatemp_min = 0.0;
  s->dynatemp_max = 0.0;
  s->dynatemp_exponent = 1.0;
  s->mirostat_mode = 0;
  s->mirostat_tau = 0.0;
  s->mirostat_eta = 0.0;
  s->dry_multiplier = 0.0;
  s->dry_base = 1.75;
  s->dry_allowed_length = 2;
  s->dry_range = 0;
  s->xtc_threshold = 0.1;
  s->xtc_probability = 0.0;
  s->nsigma = 0.0;
  s->skew = 0.0;
}

static bool get_config_path(char *buf, size_t bufsize) {
  const char *home = get_home_dir();
#ifndef _WIN32
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }
#endif
  if (!home)
    return false;
  snprintf(buf, bufsize, "%s/.config/sillytui", home);
  return true;
}

static bool get_samplers_path(char *buf, size_t bufsize, ApiType api_type) {
  char dir[500];
  if (!get_config_path(dir, sizeof(dir)))
    return false;
  int written = snprintf(buf, bufsize, "%s/samplers_%s.json", dir,
                         api_type_name(api_type));
  return written > 0 && (size_t)written < bufsize;
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

static double parse_double(char **pp) {
  char *p = *pp;
  p = skip_ws(p);
  char *end;
  double val = strtod(p, &end);
  *pp = end;
  return val;
}

static int parse_int(char **pp) {
  char *p = *pp;
  p = skip_ws(p);
  char *end;
  int val = (int)strtol(p, &end, 10);
  *pp = end;
  return val;
}

bool sampler_load(SamplerSettings *s, ApiType api_type) {
  sampler_init_defaults(s);

  char path[512];
  if (!get_samplers_path(path, sizeof(path), api_type))
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

    if (strcmp(key, "temperature") == 0)
      s->temperature = parse_double(&p);
    else if (strcmp(key, "top_p") == 0)
      s->top_p = parse_double(&p);
    else if (strcmp(key, "top_k") == 0)
      s->top_k = parse_int(&p);
    else if (strcmp(key, "min_p") == 0)
      s->min_p = parse_double(&p);
    else if (strcmp(key, "frequency_penalty") == 0)
      s->frequency_penalty = parse_double(&p);
    else if (strcmp(key, "presence_penalty") == 0)
      s->presence_penalty = parse_double(&p);
    else if (strcmp(key, "repetition_penalty") == 0)
      s->repetition_penalty = parse_double(&p);
    else if (strcmp(key, "typical_p") == 0)
      s->typical_p = parse_double(&p);
    else if (strcmp(key, "tfs") == 0)
      s->tfs = parse_double(&p);
    else if (strcmp(key, "top_a") == 0)
      s->top_a = parse_double(&p);
    else if (strcmp(key, "smoothing_factor") == 0)
      s->smoothing_factor = parse_double(&p);
    else if (strcmp(key, "min_tokens") == 0)
      s->min_tokens = parse_int(&p);
    else if (strcmp(key, "max_tokens") == 0)
      s->max_tokens = parse_int(&p);
    else if (strcmp(key, "dynatemp_min") == 0)
      s->dynatemp_min = parse_double(&p);
    else if (strcmp(key, "dynatemp_max") == 0)
      s->dynatemp_max = parse_double(&p);
    else if (strcmp(key, "dynatemp_exponent") == 0)
      s->dynatemp_exponent = parse_double(&p);
    else if (strcmp(key, "mirostat_mode") == 0)
      s->mirostat_mode = parse_int(&p);
    else if (strcmp(key, "mirostat_tau") == 0)
      s->mirostat_tau = parse_double(&p);
    else if (strcmp(key, "mirostat_eta") == 0)
      s->mirostat_eta = parse_double(&p);
    else if (strcmp(key, "dry_multiplier") == 0)
      s->dry_multiplier = parse_double(&p);
    else if (strcmp(key, "dry_base") == 0)
      s->dry_base = parse_double(&p);
    else if (strcmp(key, "dry_allowed_length") == 0)
      s->dry_allowed_length = parse_int(&p);
    else if (strcmp(key, "dry_range") == 0)
      s->dry_range = parse_int(&p);
    else if (strcmp(key, "xtc_threshold") == 0)
      s->xtc_threshold = parse_double(&p);
    else if (strcmp(key, "xtc_probability") == 0)
      s->xtc_probability = parse_double(&p);
    else if (strcmp(key, "nsigma") == 0)
      s->nsigma = parse_double(&p);
    else if (strcmp(key, "skew") == 0)
      s->skew = parse_double(&p);
    else if (strcmp(key, "custom") == 0) {
      p = skip_ws(p);
      if (*p == '[') {
        p++;
        while (*p && *p != ']') {
          p = skip_ws(p);
          if (*p == ',') {
            p++;
            continue;
          }
          if (*p == '{') {
            p++;
            char cname[CUSTOM_SAMPLER_NAME_LEN] = {0};
            char cstr[CUSTOM_SAMPLER_STR_LEN] = {0};
            double cval = 0, cmin = 0, cmax = 100, cstep = 0.1;
            SamplerValueType ctype = SAMPLER_TYPE_FLOAT;
            double list_vals[MAX_LIST_ITEMS] = {0};
            char list_strs[MAX_LIST_ITEMS][64] = {{0}};
            int list_cnt = 0;
            char dict_keys[MAX_DICT_ITEMS][DICT_KEY_LEN] = {{0}};
            char dict_str_vals[MAX_DICT_ITEMS][DICT_VAL_LEN] = {{0}};
            double dict_num_vals[MAX_DICT_ITEMS] = {0};
            bool dict_is_str[MAX_DICT_ITEMS] = {0};
            int dict_cnt = 0;
            while (*p && *p != '}') {
              p = skip_ws(p);
              if (*p == ',') {
                p++;
                continue;
              }
              char ckey[32];
              p = parse_string(p, ckey, sizeof(ckey));
              if (!p)
                break;
              p = skip_ws(p);
              if (*p == ':')
                p++;
              p = skip_ws(p);
              if (strcmp(ckey, "name") == 0) {
                p = parse_string(p, cname, sizeof(cname));
              } else if (strcmp(ckey, "value") == 0) {
                cval = parse_double(&p);
              } else if (strcmp(ckey, "str_value") == 0) {
                p = parse_string(p, cstr, sizeof(cstr));
              } else if (strcmp(ckey, "type") == 0) {
                char tstr[16] = {0};
                p = parse_string(p, tstr, sizeof(tstr));
                if (strcmp(tstr, "int") == 0)
                  ctype = SAMPLER_TYPE_INT;
                else if (strcmp(tstr, "string") == 0)
                  ctype = SAMPLER_TYPE_STRING;
                else if (strcmp(tstr, "bool") == 0)
                  ctype = SAMPLER_TYPE_BOOL;
                else if (strcmp(tstr, "list_float") == 0)
                  ctype = SAMPLER_TYPE_LIST_FLOAT;
                else if (strcmp(tstr, "list_int") == 0)
                  ctype = SAMPLER_TYPE_LIST_INT;
                else if (strcmp(tstr, "list_string") == 0)
                  ctype = SAMPLER_TYPE_LIST_STRING;
                else if (strcmp(tstr, "dict") == 0)
                  ctype = SAMPLER_TYPE_DICT;
                else
                  ctype = SAMPLER_TYPE_FLOAT;
              } else if (strcmp(ckey, "is_int") == 0) {
                if (strncmp(p, "true", 4) == 0) {
                  ctype = SAMPLER_TYPE_INT;
                  p += 4;
                } else if (strncmp(p, "false", 5) == 0) {
                  p += 5;
                }
              } else if (strcmp(ckey, "min") == 0) {
                cmin = parse_double(&p);
              } else if (strcmp(ckey, "max") == 0) {
                cmax = parse_double(&p);
              } else if (strcmp(ckey, "step") == 0) {
                cstep = parse_double(&p);
              } else if (strcmp(ckey, "list") == 0) {
                if (*p == '[') {
                  p++;
                  while (*p && *p != ']' && list_cnt < MAX_LIST_ITEMS) {
                    p = skip_ws(p);
                    if (*p == ',') {
                      p++;
                      continue;
                    }
                    if (*p == '"') {
                      char tmp[64] = {0};
                      p = parse_string(p, tmp, sizeof(tmp));
                      strncpy(list_strs[list_cnt], tmp, 63);
                      list_cnt++;
                    } else if (*p == '-' || (*p >= '0' && *p <= '9')) {
                      list_vals[list_cnt] = parse_double(&p);
                      list_cnt++;
                    } else {
                      break;
                    }
                  }
                  if (*p == ']')
                    p++;
                }
              } else if (strcmp(ckey, "dict") == 0) {
                if (*p == '{') {
                  p++;
                  while (*p && *p != '}' && dict_cnt < MAX_DICT_ITEMS) {
                    p = skip_ws(p);
                    if (*p == ',') {
                      p++;
                      continue;
                    }
                    if (*p == '"') {
                      char dkey[DICT_KEY_LEN] = {0};
                      p = parse_string(p, dkey, sizeof(dkey));
                      p = skip_ws(p);
                      if (*p == ':')
                        p++;
                      p = skip_ws(p);
                      strncpy(dict_keys[dict_cnt], dkey, DICT_KEY_LEN - 1);
                      if (*p == '"') {
                        char dval[DICT_VAL_LEN] = {0};
                        p = parse_string(p, dval, sizeof(dval));
                        strncpy(dict_str_vals[dict_cnt], dval,
                                DICT_VAL_LEN - 1);
                        dict_is_str[dict_cnt] = true;
                      } else {
                        dict_num_vals[dict_cnt] = parse_double(&p);
                        dict_is_str[dict_cnt] = false;
                      }
                      dict_cnt++;
                    } else {
                      break;
                    }
                  }
                  if (*p == '}')
                    p++;
                }
              }
            }
            if (*p == '}')
              p++;
            if (cname[0] && s->custom_count < MAX_CUSTOM_SAMPLERS) {
              CustomSampler *cs = &s->custom[s->custom_count];
              strncpy(cs->name, cname, CUSTOM_SAMPLER_NAME_LEN - 1);
              cs->type = ctype;
              cs->value = cval;
              strncpy(cs->str_value, cstr, CUSTOM_SAMPLER_STR_LEN - 1);
              cs->min_val = cmin;
              cs->max_val = cmax;
              cs->step =
                  cstep > 0 ? cstep : (ctype == SAMPLER_TYPE_INT ? 1.0 : 0.1);
              cs->list_count = list_cnt;
              for (int li = 0; li < list_cnt; li++) {
                cs->list_values[li] = list_vals[li];
                strncpy(cs->list_strings[li], list_strs[li], 63);
              }
              cs->dict_count = dict_cnt;
              for (int di = 0; di < dict_cnt; di++) {
                strncpy(cs->dict_entries[di].key, dict_keys[di],
                        DICT_KEY_LEN - 1);
                strncpy(cs->dict_entries[di].str_val, dict_str_vals[di],
                        DICT_VAL_LEN - 1);
                cs->dict_entries[di].num_val = dict_num_vals[di];
                cs->dict_entries[di].is_string = dict_is_str[di];
              }
              s->custom_count++;
            }
          } else {
            break;
          }
        }
        if (*p == ']')
          p++;
      }
    } else {
      while (*p && *p != ',' && *p != '}')
        p++;
    }
  }

  free(data);
  return true;
}

bool sampler_save(const SamplerSettings *s, ApiType api_type) {
  config_ensure_dir();

  char path[512];
  if (!get_samplers_path(path, sizeof(path), api_type))
    return false;

  FILE *f = fopen(path, "w");
  if (!f)
    return false;

  fprintf(f, "{\n");
  fprintf(f, "  \"temperature\": %.4g,\n", s->temperature);
  fprintf(f, "  \"top_p\": %.4g,\n", s->top_p);
  fprintf(f, "  \"top_k\": %d,\n", s->top_k);
  fprintf(f, "  \"min_p\": %.4g,\n", s->min_p);
  fprintf(f, "  \"frequency_penalty\": %.4g,\n", s->frequency_penalty);
  fprintf(f, "  \"presence_penalty\": %.4g,\n", s->presence_penalty);
  fprintf(f, "  \"repetition_penalty\": %.4g,\n", s->repetition_penalty);
  fprintf(f, "  \"typical_p\": %.4g,\n", s->typical_p);
  fprintf(f, "  \"tfs\": %.4g,\n", s->tfs);
  fprintf(f, "  \"top_a\": %.4g,\n", s->top_a);
  fprintf(f, "  \"smoothing_factor\": %.4g,\n", s->smoothing_factor);
  fprintf(f, "  \"min_tokens\": %d,\n", s->min_tokens);
  fprintf(f, "  \"max_tokens\": %d,\n", s->max_tokens);
  fprintf(f, "  \"dynatemp_min\": %.4g,\n", s->dynatemp_min);
  fprintf(f, "  \"dynatemp_max\": %.4g,\n", s->dynatemp_max);
  fprintf(f, "  \"dynatemp_exponent\": %.4g,\n", s->dynatemp_exponent);
  fprintf(f, "  \"mirostat_mode\": %d,\n", s->mirostat_mode);
  fprintf(f, "  \"mirostat_tau\": %.4g,\n", s->mirostat_tau);
  fprintf(f, "  \"mirostat_eta\": %.4g,\n", s->mirostat_eta);
  fprintf(f, "  \"dry_multiplier\": %.4g,\n", s->dry_multiplier);
  fprintf(f, "  \"dry_base\": %.4g,\n", s->dry_base);
  fprintf(f, "  \"dry_allowed_length\": %d,\n", s->dry_allowed_length);
  fprintf(f, "  \"dry_range\": %d,\n", s->dry_range);
  fprintf(f, "  \"xtc_threshold\": %.4g,\n", s->xtc_threshold);
  fprintf(f, "  \"xtc_probability\": %.4g,\n", s->xtc_probability);
  fprintf(f, "  \"nsigma\": %.4g,\n", s->nsigma);
  fprintf(f, "  \"skew\": %.4g", s->skew);

  if (s->custom_count > 0) {
    fprintf(f, ",\n  \"custom\": [\n");
    for (int i = 0; i < s->custom_count; i++) {
      const CustomSampler *cs = &s->custom[i];
      const char *type_str;
      switch (cs->type) {
      case SAMPLER_TYPE_INT:
        type_str = "int";
        break;
      case SAMPLER_TYPE_STRING:
        type_str = "string";
        break;
      case SAMPLER_TYPE_LIST_FLOAT:
        type_str = "list_float";
        break;
      case SAMPLER_TYPE_LIST_INT:
        type_str = "list_int";
        break;
      case SAMPLER_TYPE_LIST_STRING:
        type_str = "list_string";
        break;
      case SAMPLER_TYPE_DICT:
        type_str = "dict";
        break;
      case SAMPLER_TYPE_BOOL:
        type_str = "bool";
        break;
      default:
        type_str = "float";
        break;
      }

      if (cs->type == SAMPLER_TYPE_BOOL) {
        fprintf(f,
                "    {\"name\": \"%s\", \"type\": \"%s\", \"value\": %s}%s\n",
                cs->name, type_str, cs->value != 0 ? "true" : "false",
                i < s->custom_count - 1 ? "," : "");
      } else if (cs->type == SAMPLER_TYPE_DICT) {
        fprintf(f, "    {\"name\": \"%s\", \"type\": \"%s\", \"dict\": {",
                cs->name, type_str);
        for (int j = 0; j < cs->dict_count; j++) {
          const DictEntry *de = &cs->dict_entries[j];
          if (de->is_string)
            fprintf(f, "\"%s\": \"%s\"", de->key, de->str_val);
          else
            fprintf(f, "\"%s\": %.4g", de->key, de->num_val);
          if (j < cs->dict_count - 1)
            fprintf(f, ", ");
        }
        fprintf(f, "}}%s\n", i < s->custom_count - 1 ? "," : "");
      } else if (cs->type == SAMPLER_TYPE_STRING) {
        fprintf(f,
                "    {\"name\": \"%s\", \"type\": \"%s\", \"str_value\": "
                "\"%s\"}%s\n",
                cs->name, type_str, cs->str_value,
                i < s->custom_count - 1 ? "," : "");
      } else if (cs->type == SAMPLER_TYPE_LIST_FLOAT ||
                 cs->type == SAMPLER_TYPE_LIST_INT) {
        fprintf(f, "    {\"name\": \"%s\", \"type\": \"%s\", \"list\": [",
                cs->name, type_str);
        for (int j = 0; j < cs->list_count; j++) {
          if (cs->type == SAMPLER_TYPE_LIST_INT)
            fprintf(f, "%d", (int)cs->list_values[j]);
          else
            fprintf(f, "%.4g", cs->list_values[j]);
          if (j < cs->list_count - 1)
            fprintf(f, ", ");
        }
        fprintf(f, "]}%s\n", i < s->custom_count - 1 ? "," : "");
      } else if (cs->type == SAMPLER_TYPE_LIST_STRING) {
        fprintf(f, "    {\"name\": \"%s\", \"type\": \"%s\", \"list\": [",
                cs->name, type_str);
        for (int j = 0; j < cs->list_count; j++) {
          fprintf(f, "\"%s\"", cs->list_strings[j]);
          if (j < cs->list_count - 1)
            fprintf(f, ", ");
        }
        fprintf(f, "]}%s\n", i < s->custom_count - 1 ? "," : "");
      } else {
        fprintf(f,
                "    {\"name\": \"%s\", \"type\": \"%s\", \"value\": %.4g, "
                "\"min\": %.4g, \"max\": %.4g, \"step\": %.4g}%s\n",
                cs->name, type_str, cs->value, cs->min_val, cs->max_val,
                cs->step, i < s->custom_count - 1 ? "," : "");
      }
    }
    fprintf(f, "  ]\n");
  } else {
    fprintf(f, "\n");
  }
  fprintf(f, "}\n");

  fclose(f);
  return true;
}

bool sampler_add_custom(SamplerSettings *s, const char *name,
                        SamplerValueType type, double value,
                        const char *str_value, double min_val, double max_val,
                        double step) {
  if (s->custom_count >= MAX_CUSTOM_SAMPLERS)
    return false;
  CustomSampler *cs = &s->custom[s->custom_count];
  memset(cs, 0, sizeof(*cs));
  strncpy(cs->name, name, CUSTOM_SAMPLER_NAME_LEN - 1);
  cs->name[CUSTOM_SAMPLER_NAME_LEN - 1] = '\0';
  cs->type = type;
  cs->value = value;
  if (str_value) {
    strncpy(cs->str_value, str_value, CUSTOM_SAMPLER_STR_LEN - 1);
    cs->str_value[CUSTOM_SAMPLER_STR_LEN - 1] = '\0';
  }
  cs->min_val = min_val;
  cs->max_val = max_val;
  cs->step = step > 0 ? step : (type == SAMPLER_TYPE_INT ? 1.0 : 0.1);
  cs->list_count = 0;
  s->custom_count++;
  return true;
}

bool sampler_remove_custom(SamplerSettings *s, int index) {
  if (index < 0 || index >= s->custom_count)
    return false;
  for (int i = index; i < s->custom_count - 1; i++) {
    s->custom[i] = s->custom[i + 1];
  }
  s->custom_count--;
  return true;
}

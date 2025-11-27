#include "character/character.h"
#include <ctype.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *expand_tilde(const char *path) {
  if (!path || path[0] != '~')
    return strdup(path);

  const char *home = getenv("HOME");
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }
  if (!home)
    return strdup(path);

  size_t home_len = strlen(home);
  size_t path_len = strlen(path);
  char *expanded = malloc(home_len + path_len);
  if (!expanded)
    return strdup(path);

  strcpy(expanded, home);
  strcat(expanded, path + 1);
  return expanded;
}

static char *skip_ws(char *p) {
  while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    p++;
  return p;
}

static char *parse_json_string(char *p, char **out) {
  p = skip_ws(p);
  if (*p != '"')
    return NULL;
  p++;

  size_t cap = 256;
  size_t len = 0;
  char *buf = malloc(cap);
  if (!buf)
    return NULL;

  while (*p && *p != '"') {
    if (len + 4 >= cap) {
      cap *= 2;
      char *newbuf = realloc(buf, cap);
      if (!newbuf) {
        free(buf);
        return NULL;
      }
      buf = newbuf;
    }

    if (*p == '\\' && *(p + 1)) {
      p++;
      switch (*p) {
      case 'n':
        buf[len++] = '\n';
        break;
      case 'r':
        buf[len++] = '\r';
        break;
      case 't':
        buf[len++] = '\t';
        break;
      case '\\':
        buf[len++] = '\\';
        break;
      case '"':
        buf[len++] = '"';
        break;
      case 'u': {
        if (p[1] && p[2] && p[3] && p[4]) {
          char hex[5] = {p[1], p[2], p[3], p[4], 0};
          unsigned int codepoint = (unsigned int)strtol(hex, NULL, 16);
          p += 4;
          if (codepoint < 0x80) {
            buf[len++] = (char)codepoint;
          } else if (codepoint < 0x800) {
            buf[len++] = (char)(0xC0 | (codepoint >> 6));
            buf[len++] = (char)(0x80 | (codepoint & 0x3F));
          } else {
            buf[len++] = (char)(0xE0 | (codepoint >> 12));
            buf[len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            buf[len++] = (char)(0x80 | (codepoint & 0x3F));
          }
        }
        break;
      }
      default:
        buf[len++] = *p;
        break;
      }
      p++;
    } else {
      buf[len++] = *p++;
    }
  }

  buf[len] = '\0';
  if (*p == '"')
    p++;

  *out = buf;
  return p;
}

static char *skip_json_value(char *p) {
  p = skip_ws(p);
  if (*p == '"') {
    p++;
    while (*p && *p != '"') {
      if (*p == '\\' && *(p + 1))
        p += 2;
      else
        p++;
    }
    if (*p == '"')
      p++;
  } else if (*p == '{') {
    int depth = 1;
    p++;
    while (*p && depth > 0) {
      if (*p == '{')
        depth++;
      else if (*p == '}')
        depth--;
      else if (*p == '"') {
        p++;
        while (*p && *p != '"') {
          if (*p == '\\' && *(p + 1))
            p += 2;
          else
            p++;
        }
      }
      if (*p)
        p++;
    }
  } else if (*p == '[') {
    int depth = 1;
    p++;
    while (*p && depth > 0) {
      if (*p == '[')
        depth++;
      else if (*p == ']')
        depth--;
      else if (*p == '"') {
        p++;
        while (*p && *p != '"') {
          if (*p == '\\' && *(p + 1))
            p += 2;
          else
            p++;
        }
      }
      if (*p)
        p++;
    }
  } else {
    while (*p && *p != ',' && *p != '}' && *p != ']')
      p++;
  }
  return p;
}

static char *parse_string_array(char *p, char ***out, size_t *count) {
  *out = NULL;
  *count = 0;

  p = skip_ws(p);
  if (*p != '[')
    return p;
  p++;

  size_t cap = 8;
  char **arr = malloc(cap * sizeof(char *));
  if (!arr)
    return p;

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
    if (*p == '"') {
      char *str = NULL;
      p = parse_json_string(p, &str);
      if (str) {
        if (*count >= cap) {
          cap *= 2;
          char **newarr = realloc(arr, cap * sizeof(char *));
          if (!newarr) {
            free(str);
            break;
          }
          arr = newarr;
        }
        arr[(*count)++] = str;
      }
    } else {
      p = skip_json_value(p);
    }
  }

  *out = arr;
  return p;
}

static bool parse_character_data(char *json, CharacterCard *card) {
  char *p = json;
  p = skip_ws(p);
  if (*p != '{')
    return false;
  p++;

  bool in_data = false;

  while (*p) {
    p = skip_ws(p);
    if (*p == '}') {
      if (in_data)
        in_data = false;
      p++;
      continue;
    }
    if (*p == ',') {
      p++;
      continue;
    }

    char *key = NULL;
    p = parse_json_string(p, &key);
    if (!p || !key)
      break;

    p = skip_ws(p);
    if (*p == ':')
      p++;
    p = skip_ws(p);

    if (strcmp(key, "data") == 0 && *p == '{') {
      free(key);
      p++;
      in_data = true;
      continue;
    }

    if (strcmp(key, "name") == 0) {
      char *val = NULL;
      p = parse_json_string(p, &val);
      if (val) {
        strncpy(card->name, val, CHAR_NAME_MAX - 1);
        card->name[CHAR_NAME_MAX - 1] = '\0';
        free(val);
      }
    } else if (strcmp(key, "description") == 0) {
      p = parse_json_string(p, &card->description);
    } else if (strcmp(key, "personality") == 0) {
      p = parse_json_string(p, &card->personality);
    } else if (strcmp(key, "scenario") == 0) {
      p = parse_json_string(p, &card->scenario);
    } else if (strcmp(key, "first_mes") == 0) {
      p = parse_json_string(p, &card->first_mes);
    } else if (strcmp(key, "mes_example") == 0) {
      p = parse_json_string(p, &card->mes_example);
    } else if (strcmp(key, "system_prompt") == 0) {
      p = parse_json_string(p, &card->system_prompt);
    } else if (strcmp(key, "post_history_instructions") == 0) {
      p = parse_json_string(p, &card->post_history_instructions);
    } else if (strcmp(key, "creator_notes") == 0) {
      p = parse_json_string(p, &card->creator_notes);
    } else if (strcmp(key, "creator") == 0) {
      p = parse_json_string(p, &card->creator);
    } else if (strcmp(key, "character_version") == 0) {
      p = parse_json_string(p, &card->character_version);
    } else if (strcmp(key, "alternate_greetings") == 0) {
      p = parse_string_array(p, &card->alternate_greetings,
                             &card->alternate_greetings_count);
    } else if (strcmp(key, "tags") == 0) {
      p = parse_string_array(p, &card->tags, &card->tags_count);
    } else {
      p = skip_json_value(p);
    }

    free(key);
    if (!p)
      break;
  }

  return card->name[0] != '\0';
}

bool character_load_json(CharacterCard *card, const char *path) {
  memset(card, 0, sizeof(*card));

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

  char *data = malloc(len + 1);
  if (!data) {
    fclose(f);
    return false;
  }

  size_t read_len = fread(data, 1, len, f);
  fclose(f);
  data[read_len] = '\0';

  bool result = parse_character_data(data, card);
  free(data);
  return result;
}

static int base64_decode_char(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}

static char *base64_decode(const char *input, size_t *out_len) {
  size_t input_len = strlen(input);
  size_t padding = 0;
  if (input_len > 0 && input[input_len - 1] == '=')
    padding++;
  if (input_len > 1 && input[input_len - 2] == '=')
    padding++;

  size_t output_len = (input_len / 4) * 3 - padding;
  char *output = malloc(output_len + 1);
  if (!output)
    return NULL;

  size_t j = 0;
  for (size_t i = 0; i < input_len;) {
    int sextet[4] = {0, 0, 0, 0};
    int valid = 0;

    for (int k = 0; k < 4 && i < input_len; k++) {
      while (i < input_len &&
             (input[i] == '\n' || input[i] == '\r' || input[i] == ' '))
        i++;
      if (i >= input_len)
        break;
      if (input[i] == '=') {
        i++;
        continue;
      }
      sextet[k] = base64_decode_char(input[i++]);
      if (sextet[k] >= 0)
        valid++;
    }

    if (valid >= 2) {
      uint32_t triple = ((uint32_t)sextet[0] << 18) |
                        ((uint32_t)sextet[1] << 12) |
                        ((uint32_t)sextet[2] << 6) | (uint32_t)sextet[3];
      if (j < output_len)
        output[j++] = (triple >> 16) & 0xFF;
      if (j < output_len && valid >= 3)
        output[j++] = (triple >> 8) & 0xFF;
      if (j < output_len && valid >= 4)
        output[j++] = triple & 0xFF;
    }
  }

  output[j] = '\0';
  if (out_len)
    *out_len = j;
  return output;
}

static uint32_t read_be32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

bool character_load_png(CharacterCard *card, const char *path) {
  memset(card, 0, sizeof(*card));

  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  uint8_t header[8];
  if (fread(header, 1, 8, f) != 8) {
    fclose(f);
    return false;
  }

  static const uint8_t png_sig[8] = {0x89, 0x50, 0x4E, 0x47,
                                     0x0D, 0x0A, 0x1A, 0x0A};
  if (memcmp(header, png_sig, 8) != 0) {
    fclose(f);
    return false;
  }

  char *json_data = NULL;

  while (!feof(f)) {
    uint8_t chunk_header[8];
    if (fread(chunk_header, 1, 8, f) != 8)
      break;

    uint32_t chunk_len = read_be32(chunk_header);
    char chunk_type[5] = {0};
    memcpy(chunk_type, chunk_header + 4, 4);

    if (strcmp(chunk_type, "tEXt") == 0) {
      char *chunk_data = malloc(chunk_len + 1);
      if (!chunk_data)
        break;

      if (fread(chunk_data, 1, chunk_len, f) != chunk_len) {
        free(chunk_data);
        break;
      }
      chunk_data[chunk_len] = '\0';

      size_t keyword_len = strlen(chunk_data);
      if (strcmp(chunk_data, "chara") == 0 || strcmp(chunk_data, "ccv3") == 0) {
        const char *base64_data = chunk_data + keyword_len + 1;
        size_t decoded_len = 0;
        json_data = base64_decode(base64_data, &decoded_len);
        free(chunk_data);
        break;
      }

      free(chunk_data);
      fseek(f, 4, SEEK_CUR);
    } else if (strcmp(chunk_type, "IEND") == 0) {
      break;
    } else {
      fseek(f, chunk_len + 4, SEEK_CUR);
    }
  }

  fclose(f);

  if (!json_data)
    return false;

  bool result = parse_character_data(json_data, card);
  free(json_data);
  return result;
}

bool character_load(CharacterCard *card, const char *path) {
  char *expanded = expand_tilde(path);
  if (!expanded)
    return false;

  size_t len = strlen(expanded);
  bool result;
  if (len >= 4 && strcasecmp(expanded + len - 4, ".png") == 0) {
    result = character_load_png(card, expanded);
  } else {
    result = character_load_json(card, expanded);
  }
  free(expanded);
  return result;
}

void character_free(CharacterCard *card) {
  if (!card)
    return;

  free(card->description);
  free(card->personality);
  free(card->scenario);
  free(card->first_mes);
  free(card->mes_example);
  free(card->system_prompt);
  free(card->post_history_instructions);
  free(card->creator_notes);
  free(card->creator);
  free(card->character_version);

  if (card->alternate_greetings) {
    for (size_t i = 0; i < card->alternate_greetings_count; i++) {
      free(card->alternate_greetings[i]);
    }
    free(card->alternate_greetings);
  }

  if (card->tags) {
    for (size_t i = 0; i < card->tags_count; i++) {
      free(card->tags[i]);
    }
    free(card->tags);
  }

  memset(card, 0, sizeof(*card));
}

const char *character_get_greeting(const CharacterCard *card, size_t index) {
  if (!card)
    return NULL;
  if (index == 0)
    return card->first_mes;
  if (index <= card->alternate_greetings_count)
    return card->alternate_greetings[index - 1];
  return NULL;
}

static bool ensure_characters_dir(void) {
  const char *home = getenv("HOME");
  if (!home)
    return false;

  char path[512];
  snprintf(path, sizeof(path), "%s/.config/sillytui", home);
  mkdir(path, 0755);
  snprintf(path, sizeof(path), "%s/.config/sillytui/characters", home);
  mkdir(path, 0755);
  return true;
}

char *character_copy_to_config(const char *src_path) {
  if (!src_path || !src_path[0])
    return NULL;

  char *expanded = expand_tilde(src_path);
  if (!expanded)
    return NULL;

  FILE *src = fopen(expanded, "rb");
  if (!src) {
    free(expanded);
    return NULL;
  }

  fseek(src, 0, SEEK_END);
  long size = ftell(src);
  fseek(src, 0, SEEK_SET);

  if (size <= 0) {
    fclose(src);
    free(expanded);
    return NULL;
  }

  char *data = malloc(size);
  if (!data) {
    fclose(src);
    free(expanded);
    return NULL;
  }

  if (fread(data, 1, size, src) != (size_t)size) {
    free(data);
    fclose(src);
    free(expanded);
    return NULL;
  }
  fclose(src);

  const char *filename = strrchr(expanded, '/');
  if (filename)
    filename++;
  else
    filename = expanded;

  if (!ensure_characters_dir()) {
    free(data);
    free(expanded);
    return NULL;
  }

  const char *home = getenv("HOME");
  char *dest_path = malloc(512);
  if (!dest_path) {
    free(data);
    free(expanded);
    return NULL;
  }
  snprintf(dest_path, 512, "%s/.config/sillytui/characters/%s", home, filename);

  FILE *dst = fopen(dest_path, "rb");
  if (dst) {
    fclose(dst);
    free(data);
    free(expanded);
    return dest_path;
  }

  dst = fopen(dest_path, "wb");
  if (!dst) {
    free(dest_path);
    free(data);
    free(expanded);
    return NULL;
  }

  fwrite(data, 1, size, dst);
  fclose(dst);
  free(data);
  free(expanded);

  return dest_path;
}

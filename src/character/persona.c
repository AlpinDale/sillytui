#include "character/persona.h"
#include "core/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <pwd.h>
#endif

static bool get_persona_path(char *buf, size_t bufsize) {
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
  snprintf(buf, bufsize, "%s/.config/sillytui/persona.json", home);
  return true;
}

static char *skip_ws(char *p) {
  while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
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
    } else if (*s == '\r') {
      fputs("\\r", f);
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

bool persona_load(Persona *persona) {
  memset(persona, 0, sizeof(*persona));
  strncpy(persona->name, "User", PERSONA_NAME_MAX - 1);

  char path[512];
  if (!get_persona_path(path, sizeof(path)))
    return false;

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

    if (strcmp(key, "name") == 0) {
      p = parse_string(p, persona->name, sizeof(persona->name));
    } else if (strcmp(key, "description") == 0) {
      p = parse_string(p, persona->description, sizeof(persona->description));
    } else {
      while (*p && *p != ',' && *p != '}')
        p++;
    }

    if (!p)
      break;
  }

  free(data);
  return true;
}

static bool ensure_config_dir(void) {
  const char *home = get_home_dir();
  if (!home)
    return false;

  char path[512];
  snprintf(path, sizeof(path), "%s/.config/sillytui", home);

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

bool persona_save(const Persona *persona) {
  if (!ensure_config_dir())
    return false;

  char path[512];
  if (!get_persona_path(path, sizeof(path)))
    return false;

  FILE *f = fopen(path, "w");
  if (!f)
    return false;

  fprintf(f, "{\n");
  fprintf(f, "  \"name\": ");
  write_escaped(f, persona->name);
  fprintf(f, ",\n");
  fprintf(f, "  \"description\": ");
  write_escaped(f, persona->description);
  fprintf(f, "\n}\n");

  fclose(f);
  return true;
}

const char *persona_get_name(const Persona *persona) {
  if (!persona || persona->name[0] == '\0')
    return "User";
  return persona->name;
}

const char *persona_get_description(const Persona *persona) {
  if (!persona)
    return "";
  return persona->description;
}

void persona_set_name(Persona *persona, const char *name) {
  if (!persona)
    return;
  strncpy(persona->name, name ? name : "User", PERSONA_NAME_MAX - 1);
  persona->name[PERSONA_NAME_MAX - 1] = '\0';
}

void persona_set_description(Persona *persona, const char *description) {
  if (!persona)
    return;
  strncpy(persona->description, description ? description : "",
          PERSONA_DESC_MAX - 1);
  persona->description[PERSONA_DESC_MAX - 1] = '\0';
}

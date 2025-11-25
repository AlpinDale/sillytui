#include "chat.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *get_chats_dir(void) {
  static char path[512] = {0};
  if (path[0] == '\0') {
    const char *home = getenv("HOME");
    if (home) {
      snprintf(path, sizeof(path), "%s/.config/sillytui/chats", home);
    }
  }
  return path;
}

static bool ensure_chats_dir(void) {
  const char *dir = get_chats_dir();
  if (!dir || dir[0] == '\0')
    return false;

  char parent[512];
  snprintf(parent, sizeof(parent), "%s/.config/sillytui", getenv("HOME"));
  mkdir(parent, 0755);
  mkdir(dir, 0755);
  return true;
}

static char *escape_json(const char *str) {
  size_t len = strlen(str);
  char *out = malloc(len * 6 + 1);
  if (!out)
    return NULL;

  size_t j = 0;
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)str[i];
    if (c == '"') {
      out[j++] = '\\';
      out[j++] = '"';
    } else if (c == '\\') {
      out[j++] = '\\';
      out[j++] = '\\';
    } else if (c == '\n') {
      out[j++] = '\\';
      out[j++] = 'n';
    } else if (c == '\r') {
      out[j++] = '\\';
      out[j++] = 'r';
    } else if (c == '\t') {
      out[j++] = '\\';
      out[j++] = 't';
    } else if (c < 32) {
      j += sprintf(out + j, "\\u%04x", c);
    } else {
      out[j++] = c;
    }
  }
  out[j] = '\0';
  return out;
}

static char *read_file_contents(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (len <= 0) {
    fclose(f);
    return NULL;
  }

  char *buf = malloc(len + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t read = fread(buf, 1, len, f);
  fclose(f);
  buf[read] = '\0';
  if (out_len)
    *out_len = read;
  return buf;
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

static long find_json_number(const char *json, const char *key) {
  char search[128];
  snprintf(search, sizeof(search), "\"%s\"", key);
  const char *p = strstr(json, search);
  if (!p)
    return 0;
  p += strlen(search);
  while (*p == ' ' || *p == ':')
    p++;
  return strtol(p, NULL, 10);
}

void chat_list_init(ChatList *list) {
  list->chats = NULL;
  list->count = 0;
  list->capacity = 0;
}

void chat_list_free(ChatList *list) {
  free(list->chats);
  list->chats = NULL;
  list->count = 0;
  list->capacity = 0;
}

bool chat_list_load(ChatList *list) {
  chat_list_free(list);
  chat_list_init(list);

  const char *dir = get_chats_dir();
  if (!dir)
    return false;

  DIR *d = opendir(dir);
  if (!d)
    return false;

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    size_t namelen = strlen(entry->d_name);
    if (namelen < 6 || strcmp(entry->d_name + namelen - 5, ".json") != 0)
      continue;

    char filepath[768];
    snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

    char *content = read_file_contents(filepath, NULL);
    if (!content)
      continue;

    if (list->count >= list->capacity) {
      size_t newcap = list->capacity == 0 ? 8 : list->capacity * 2;
      ChatMeta *tmp = realloc(list->chats, newcap * sizeof(ChatMeta));
      if (!tmp) {
        free(content);
        continue;
      }
      list->chats = tmp;
      list->capacity = newcap;
    }

    ChatMeta *meta = &list->chats[list->count];
    memset(meta, 0, sizeof(ChatMeta));

    char *id = find_json_string(content, "id");
    if (id) {
      strncpy(meta->id, id, CHAT_ID_MAX - 1);
      free(id);
    } else {
      strncpy(meta->id, entry->d_name, CHAT_ID_MAX - 1);
      char *dot = strrchr(meta->id, '.');
      if (dot)
        *dot = '\0';
    }

    char *title = find_json_string(content, "title");
    if (title) {
      strncpy(meta->title, title, CHAT_TITLE_MAX - 1);
      free(title);
    }

    char *char_path = find_json_string(content, "character_path");
    if (char_path) {
      strncpy(meta->character_path, char_path, CHAT_CHAR_PATH_MAX - 1);
      free(char_path);
    }

    meta->created_at = find_json_number(content, "created_at");
    meta->updated_at = find_json_number(content, "updated_at");
    meta->message_count = find_json_number(content, "message_count");

    free(content);
    list->count++;
  }

  closedir(d);

  for (size_t i = 0; i < list->count; i++) {
    for (size_t j = i + 1; j < list->count; j++) {
      if (list->chats[j].updated_at > list->chats[i].updated_at) {
        ChatMeta tmp = list->chats[i];
        list->chats[i] = list->chats[j];
        list->chats[j] = tmp;
      }
    }
  }

  return true;
}

bool chat_save(const ChatHistory *history, const char *id, const char *title,
               const char *character_path) {
  if (!ensure_chats_dir())
    return false;

  const char *dir = get_chats_dir();
  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", dir, id);

  FILE *f = fopen(filepath, "w");
  if (!f)
    return false;

  time_t now = time(NULL);

  char *escaped_title = escape_json(title);
  if (!escaped_title) {
    fclose(f);
    return false;
  }

  fprintf(f, "{\n");
  fprintf(f, "  \"id\": \"%s\",\n", id);
  fprintf(f, "  \"title\": \"%s\",\n", escaped_title);
  if (character_path && character_path[0]) {
    char *escaped_path = escape_json(character_path);
    if (escaped_path) {
      fprintf(f, "  \"character_path\": \"%s\",\n", escaped_path);
      free(escaped_path);
    }
  }
  fprintf(f, "  \"created_at\": %ld,\n", (long)now);
  fprintf(f, "  \"updated_at\": %ld,\n", (long)now);
  fprintf(f, "  \"message_count\": %zu,\n", history->count);
  fprintf(f, "  \"messages\": [\n");

  for (size_t i = 0; i < history->count; i++) {
    char *escaped = escape_json(history->items[i]);
    if (escaped) {
      fprintf(f, "    \"%s\"%s\n", escaped, i < history->count - 1 ? "," : "");
      free(escaped);
    }
  }

  fprintf(f, "  ]\n");
  fprintf(f, "}\n");

  free(escaped_title);
  fclose(f);
  return true;
}

static char *find_chat_id_by_title(const char *title) {
  const char *dir = get_chats_dir();
  if (!dir)
    return NULL;

  DIR *d = opendir(dir);
  if (!d)
    return NULL;

  char *found_id = NULL;
  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    size_t namelen = strlen(entry->d_name);
    if (namelen < 6 || strcmp(entry->d_name + namelen - 5, ".json") != 0)
      continue;

    char filepath[768];
    snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

    char *content = read_file_contents(filepath, NULL);
    if (!content)
      continue;

    char *chat_title = find_json_string(content, "title");
    if (chat_title && strcmp(chat_title, title) == 0) {
      char *chat_id = find_json_string(content, "id");
      if (chat_id) {
        found_id = chat_id;
      }
      free(chat_title);
      free(content);
      break;
    }
    if (chat_title)
      free(chat_title);
    free(content);
  }

  closedir(d);
  return found_id;
}

bool chat_load(ChatHistory *history, const char *id_or_title,
               char *out_character_path, size_t path_size) {
  const char *dir = get_chats_dir();
  if (!dir)
    return false;

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", dir, id_or_title);

  char *content = read_file_contents(filepath, NULL);

  char *found_id = NULL;
  if (!content) {
    found_id = find_chat_id_by_title(id_or_title);
    if (found_id) {
      snprintf(filepath, sizeof(filepath), "%s/%s.json", dir, found_id);
      content = read_file_contents(filepath, NULL);
    }
  }

  if (!content) {
    if (found_id)
      free(found_id);
    return false;
  }

  if (found_id)
    free(found_id);

  if (out_character_path && path_size > 0) {
    out_character_path[0] = '\0';
    char *char_path = find_json_string(content, "character_path");
    if (char_path) {
      strncpy(out_character_path, char_path, path_size - 1);
      out_character_path[path_size - 1] = '\0';
      free(char_path);
    }
  }

  history_free(history);
  history_init(history);

  const char *messages = strstr(content, "\"messages\"");
  if (!messages) {
    free(content);
    return false;
  }

  const char *arr_start = strchr(messages, '[');
  if (!arr_start) {
    free(content);
    return false;
  }
  arr_start++;

  const char *p = arr_start;
  while (*p) {
    while (*p && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
      p++;
    if (*p == ']')
      break;
    if (*p == ',') {
      p++;
      continue;
    }
    if (*p != '"') {
      p++;
      continue;
    }

    p++;
    const char *str_start = p;
    while (*p && !(*p == '"' && *(p - 1) != '\\'))
      p++;
    size_t len = p - str_start;

    char *msg = malloc(len + 1);
    if (msg) {
      size_t j = 0;
      for (size_t i = 0; i < len; i++) {
        if (str_start[i] == '\\' && i + 1 < len) {
          i++;
          if (str_start[i] == 'n')
            msg[j++] = '\n';
          else if (str_start[i] == 't')
            msg[j++] = '\t';
          else if (str_start[i] == 'r')
            msg[j++] = '\r';
          else
            msg[j++] = str_start[i];
        } else {
          msg[j++] = str_start[i];
        }
      }
      msg[j] = '\0';
      history_add(history, msg);
      free(msg);
    }

    if (*p == '"')
      p++;
  }

  free(content);
  return true;
}

bool chat_delete(const char *id) {
  const char *dir = get_chats_dir();
  if (!dir)
    return false;

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", dir, id);
  return unlink(filepath) == 0;
}

char *chat_generate_id(void) {
  static char id[32];
  time_t now = time(NULL);
  unsigned int r = (unsigned int)now ^ (unsigned int)getpid();
  snprintf(id, sizeof(id), "%08x%04x", (unsigned int)now, r & 0xFFFF);
  return id;
}

const char *chat_auto_title(const ChatHistory *history) {
  static char title[CHAT_TITLE_MAX];

  for (size_t i = 0; i < history->count; i++) {
    const char *msg = history->items[i];
    if (strncmp(msg, "You: ", 5) == 0) {
      const char *content = msg + 5;
      size_t len = strlen(content);
      if (len > CHAT_TITLE_MAX - 4) {
        strncpy(title, content, CHAT_TITLE_MAX - 4);
        title[CHAT_TITLE_MAX - 4] = '\0';
        strcat(title, "...");
      } else {
        strncpy(title, content, CHAT_TITLE_MAX - 1);
        title[CHAT_TITLE_MAX - 1] = '\0';
      }
      return title;
    }
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(title, sizeof(title), "Chat %Y-%m-%d %H:%M", t);
  return title;
}

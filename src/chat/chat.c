#include "chat/chat.h"
#include "core/platform.h"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CHAR_DIR "_default"

static const char *get_chats_dir(void) {
  static char path[512] = {0};
  if (path[0] == '\0') {
    const char *home = get_home_dir();
    if (home) {
      snprintf(path, sizeof(path), "%s/.config/sillytui/chats", home);
    }
  }
  return path;
}

void chat_sanitize_dirname(const char *name, char *out, size_t out_size) {
  if (!name || !out || out_size == 0)
    return;

  size_t j = 0;
  for (size_t i = 0; name[i] && j < out_size - 1; i++) {
    unsigned char c = (unsigned char)name[i];
    if (isalnum(c) || c == '-' || c == '_') {
      out[j++] = (char)tolower(c);
    } else if (c == ' ') {
      out[j++] = '_';
    }
  }
  out[j] = '\0';

  if (j == 0) {
    strncpy(out, DEFAULT_CHAR_DIR, out_size - 1);
    out[out_size - 1] = '\0';
  }
}

static bool get_character_chats_dir(const char *character_name, char *out,
                                    size_t out_size) {
  const char *base = get_chats_dir();
  if (!base)
    return false;

  char sanitized[CHAT_CHAR_NAME_MAX];
  if (character_name && character_name[0]) {
    chat_sanitize_dirname(character_name, sanitized, sizeof(sanitized));
  } else {
    strncpy(sanitized, DEFAULT_CHAR_DIR, sizeof(sanitized) - 1);
    sanitized[sizeof(sanitized) - 1] = '\0';
  }

  snprintf(out, out_size, "%s/%s", base, sanitized);
  return true;
}

static bool ensure_chats_dir(void) {
  const char *dir = get_chats_dir();
  if (!dir || dir[0] == '\0')
    return false;

  char parent[512];
  snprintf(parent, sizeof(parent), "%s/.config/sillytui", get_home_dir());
  mkdir(parent, 0755);
  mkdir(dir, 0755);
  return true;
}

static bool ensure_character_chats_dir(const char *character_name) {
  if (!ensure_chats_dir())
    return false;

  char char_dir[768];
  if (!get_character_chats_dir(character_name, char_dir, sizeof(char_dir)))
    return false;

  mkdir(char_dir, 0755);
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

  size_t read_count = fread(buf, 1, len, f);
  fclose(f);
  buf[read_count] = '\0';
  if (out_len)
    *out_len = read_count;
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

static bool load_chat_meta_from_file(const char *filepath,
                                     const char *char_dirname, ChatMeta *meta) {
  char *content = read_file_contents(filepath, NULL);
  if (!content)
    return false;

  memset(meta, 0, sizeof(ChatMeta));

  char *id = find_json_string(content, "id");
  if (id) {
    strncpy(meta->id, id, CHAT_ID_MAX - 1);
    free(id);
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

  char *char_name = find_json_string(content, "character_name");
  if (char_name) {
    strncpy(meta->character_name, char_name, CHAT_CHAR_NAME_MAX - 1);
    free(char_name);
  } else if (char_dirname && strcmp(char_dirname, DEFAULT_CHAR_DIR) != 0) {
    strncpy(meta->character_name, char_dirname, CHAT_CHAR_NAME_MAX - 1);
  }

  meta->created_at = find_json_number(content, "created_at");
  meta->updated_at = find_json_number(content, "updated_at");
  meta->message_count = find_json_number(content, "message_count");

  free(content);
  return true;
}

static bool load_chats_from_directory(ChatList *list, const char *dir_path,
                                      const char *char_dirname) {
  DIR *d = opendir(dir_path);
  if (!d)
    return false;

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    size_t namelen = strlen(entry->d_name);
    if (namelen < 6 || strcmp(entry->d_name + namelen - 5, ".json") != 0)
      continue;

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);

    if (list->count >= list->capacity) {
      size_t newcap = list->capacity == 0 ? 8 : list->capacity * 2;
      ChatMeta *tmp = realloc(list->chats, newcap * sizeof(ChatMeta));
      if (!tmp)
        continue;
      list->chats = tmp;
      list->capacity = newcap;
    }

    ChatMeta *meta = &list->chats[list->count];
    if (load_chat_meta_from_file(filepath, char_dirname, meta)) {
      list->count++;
    }
  }

  closedir(d);
  return true;
}

bool chat_list_load(ChatList *list) {
  return chat_list_load_for_character(list, NULL);
}

bool chat_list_load_for_character(ChatList *list, const char *character_name) {
  chat_list_free(list);
  chat_list_init(list);

  const char *base_dir = get_chats_dir();
  if (!base_dir)
    return false;

  if (character_name && character_name[0]) {
    char char_dir[768];
    if (!get_character_chats_dir(character_name, char_dir, sizeof(char_dir)))
      return false;

    char sanitized[CHAT_CHAR_NAME_MAX];
    chat_sanitize_dirname(character_name, sanitized, sizeof(sanitized));
    load_chats_from_directory(list, char_dir, sanitized);
  } else {
    DIR *d = opendir(base_dir);
    if (!d)
      return false;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
      if (entry->d_name[0] == '.')
        continue;

      char subdir_path[768];
      snprintf(subdir_path, sizeof(subdir_path), "%s/%s", base_dir,
               entry->d_name);

      struct stat st;
      if (stat(subdir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        load_chats_from_directory(list, subdir_path, entry->d_name);
      } else if (S_ISREG(st.st_mode)) {
        size_t namelen = strlen(entry->d_name);
        if (namelen >= 6 && strcmp(entry->d_name + namelen - 5, ".json") == 0) {
          if (list->count >= list->capacity) {
            size_t newcap = list->capacity == 0 ? 8 : list->capacity * 2;
            ChatMeta *tmp = realloc(list->chats, newcap * sizeof(ChatMeta));
            if (tmp) {
              list->chats = tmp;
              list->capacity = newcap;
            }
          }
          if (list->count < list->capacity) {
            ChatMeta *meta = &list->chats[list->count];
            if (load_chat_meta_from_file(subdir_path, NULL, meta)) {
              list->count++;
            }
          }
        }
      }
    }
    closedir(d);
  }

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
               const char *character_path, const char *character_name) {
  if (!ensure_character_chats_dir(character_name))
    return false;

  char char_dir[768];
  if (!get_character_chats_dir(character_name, char_dir, sizeof(char_dir)))
    return false;

  char filepath[1024];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", char_dir, id);

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
  if (character_name && character_name[0]) {
    char *escaped_name = escape_json(character_name);
    if (escaped_name) {
      fprintf(f, "  \"character_name\": \"%s\",\n", escaped_name);
      free(escaped_name);
    }
  }
  fprintf(f, "  \"created_at\": %ld,\n", (long)now);
  fprintf(f, "  \"updated_at\": %ld,\n", (long)now);
  fprintf(f, "  \"message_count\": %zu,\n", history->count);
  fprintf(f, "  \"messages\": [\n");

  for (size_t i = 0; i < history->count; i++) {
    ChatMessage *msg = &history->messages[i];
    fprintf(f, "    {\n");
    fprintf(f, "      \"role\": \"%s\",\n", role_to_string(msg->role));
    fprintf(f, "      \"active\": %zu,\n", msg->active_swipe);
    fprintf(f, "      \"swipes\": [\n");
    for (size_t j = 0; j < msg->swipe_count; j++) {
      char *escaped = escape_json(msg->swipes[j]);
      if (escaped) {
        fprintf(f, "        \"%s\"%s\n", escaped,
                j < msg->swipe_count - 1 ? "," : "");
        free(escaped);
      }
    }
    fprintf(f, "      ]\n");
    fprintf(f, "    }%s\n", i < history->count - 1 ? "," : "");
  }

  fprintf(f, "  ]\n");
  fprintf(f, "}\n");

  free(escaped_title);
  fclose(f);
  return true;
}

bool chat_save_with_note(const ChatHistory *history, const AuthorNote *note,
                         const char *id, const char *title,
                         const char *character_path,
                         const char *character_name) {
  if (!ensure_character_chats_dir(character_name))
    return false;

  char char_dir[768];
  if (!get_character_chats_dir(character_name, char_dir, sizeof(char_dir)))
    return false;

  char filepath[1024];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", char_dir, id);

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
  if (character_name && character_name[0]) {
    char *escaped_name = escape_json(character_name);
    if (escaped_name) {
      fprintf(f, "  \"character_name\": \"%s\",\n", escaped_name);
      free(escaped_name);
    }
  }
  fprintf(f, "  \"created_at\": %ld,\n", (long)now);
  fprintf(f, "  \"updated_at\": %ld,\n", (long)now);
  fprintf(f, "  \"message_count\": %zu,\n", history->count);

  if (note && note->text[0]) {
    char *escaped_note = escape_json(note->text);
    if (escaped_note) {
      fprintf(f, "  \"author_note\": {\n");
      fprintf(f, "    \"text\": \"%s\",\n", escaped_note);
      fprintf(f, "    \"depth\": %d,\n", note->depth);
      fprintf(f, "    \"interval\": %d,\n", note->interval);
      fprintf(f, "    \"position\": \"%s\",\n",
              author_note_position_to_string(note->position));
      fprintf(f, "    \"role\": \"%s\"\n",
              author_note_role_to_string(note->role));
      fprintf(f, "  },\n");
      free(escaped_note);
    }
  }

  fprintf(f, "  \"messages\": [\n");

  for (size_t i = 0; i < history->count; i++) {
    ChatMessage *msg = &history->messages[i];
    fprintf(f, "    {\n");
    fprintf(f, "      \"role\": \"%s\",\n", role_to_string(msg->role));
    fprintf(f, "      \"active\": %zu,\n", msg->active_swipe);
    fprintf(f, "      \"swipes\": [\n");
    for (size_t j = 0; j < msg->swipe_count; j++) {
      char *escaped = escape_json(msg->swipes[j]);
      if (escaped) {
        fprintf(f, "        \"%s\"%s\n", escaped,
                j < msg->swipe_count - 1 ? "," : "");
        free(escaped);
      }
    }
    fprintf(f, "      ]\n");
    fprintf(f, "    }%s\n", i < history->count - 1 ? "," : "");
  }

  fprintf(f, "  ]\n");
  fprintf(f, "}\n");

  free(escaped_title);
  fclose(f);
  return true;
}

static bool try_load_chat_from_path_with_note(ChatHistory *history,
                                              AuthorNote *note,
                                              const char *filepath,
                                              char *out_character_path,
                                              size_t path_size) {
  char *content = read_file_contents(filepath, NULL);
  if (!content)
    return false;

  if (out_character_path && path_size > 0) {
    out_character_path[0] = '\0';
    char *char_path = find_json_string(content, "character_path");
    if (char_path) {
      strncpy(out_character_path, char_path, path_size - 1);
      out_character_path[path_size - 1] = '\0';
      free(char_path);
    }
  }

  if (note) {
    author_note_init(note);
    const char *an_obj = strstr(content, "\"author_note\"");
    if (an_obj) {
      const char *an_start = strchr(an_obj + 13, '{');
      if (an_start) {
        char *text = find_json_string(an_start, "text");
        if (text) {
          author_note_set_text(note, text);
          free(text);
        }
        const char *depth_key = strstr(an_start, "\"depth\"");
        if (depth_key) {
          const char *colon = strchr(depth_key + 7, ':');
          if (colon)
            author_note_set_depth(note, (int)strtol(colon + 1, NULL, 10));
        }
        const char *interval_key = strstr(an_start, "\"interval\"");
        if (interval_key) {
          const char *colon = strchr(interval_key + 10, ':');
          if (colon)
            author_note_set_interval(note, (int)strtol(colon + 1, NULL, 10));
        }
        char *pos_str = find_json_string(an_start, "position");
        if (pos_str) {
          author_note_set_position(note,
                                   author_note_position_from_string(pos_str));
          free(pos_str);
        }
        char *role_str = find_json_string(an_start, "role");
        if (role_str) {
          author_note_set_role(note, author_note_role_from_string(role_str));
          free(role_str);
        }
      }
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

    if (*p == '{') {
      p++;
      long active_swipe = 0;
      MessageRole msg_role = ROLE_USER;

      const char *role_key = strstr(p, "\"role\"");
      if (role_key) {
        const char *colon = strchr(role_key + 6, ':');
        if (colon) {
          const char *q = colon + 1;
          while (*q == ' ' || *q == '\t')
            q++;
          if (*q == '"') {
            q++;
            if (strncmp(q, "assistant", 9) == 0)
              msg_role = ROLE_ASSISTANT;
            else if (strncmp(q, "system", 6) == 0)
              msg_role = ROLE_SYSTEM;
          }
        }
      }

      const char *active_key = strstr(p, "\"active\"");
      if (active_key) {
        const char *colon = strchr(active_key + 8, ':');
        if (colon)
          active_swipe = strtol(colon + 1, NULL, 10);
      }

      const char *swipes_key = strstr(p, "\"swipes\"");
      if (!swipes_key) {
        while (*p && *p != '}')
          p++;
        if (*p == '}')
          p++;
        continue;
      }

      const char *swipes_arr = strchr(swipes_key, '[');
      if (!swipes_arr) {
        while (*p && *p != '}')
          p++;
        if (*p == '}')
          p++;
        continue;
      }
      swipes_arr++;

      size_t msg_idx = SIZE_MAX;
      bool first_swipe = true;

      const char *sp = swipes_arr;
      while (*sp) {
        while (*sp && (*sp == ' ' || *sp == '\n' || *sp == '\r' || *sp == '\t'))
          sp++;
        if (*sp == ']')
          break;
        if (*sp == ',') {
          sp++;
          continue;
        }
        if (*sp != '"') {
          sp++;
          continue;
        }

        sp++;
        const char *str_start = sp;
        while (*sp && !(*sp == '"' && *(sp - 1) != '\\'))
          sp++;
        size_t len = sp - str_start;

        char *swipe_content = malloc(len + 1);
        if (swipe_content) {
          size_t j = 0;
          for (size_t i = 0; i < len; i++) {
            if (str_start[i] == '\\' && i + 1 < len) {
              i++;
              if (str_start[i] == 'n')
                swipe_content[j++] = '\n';
              else if (str_start[i] == 't')
                swipe_content[j++] = '\t';
              else if (str_start[i] == 'r')
                swipe_content[j++] = '\r';
              else
                swipe_content[j++] = str_start[i];
            } else {
              swipe_content[j++] = str_start[i];
            }
          }
          swipe_content[j] = '\0';

          if (first_swipe) {
            msg_idx = history_add(history, swipe_content);
            first_swipe = false;
          } else if (msg_idx != SIZE_MAX) {
            history_add_swipe(history, msg_idx, swipe_content);
          }
          free(swipe_content);
        }

        if (*sp == '"')
          sp++;
      }

      if (msg_idx != SIZE_MAX) {
        if (active_swipe >= 0) {
          history_set_active_swipe(history, msg_idx, (size_t)active_swipe);
        }
        history_set_role(history, msg_idx, msg_role);
      }

      while (*p && *p != '}')
        p++;
      if (*p == '}')
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

static bool try_load_chat_from_path(ChatHistory *history, const char *filepath,
                                    char *out_character_path,
                                    size_t path_size) {
  return try_load_chat_from_path_with_note(history, NULL, filepath,
                                           out_character_path, path_size);
}

bool chat_load(ChatHistory *history, const char *id, const char *character_name,
               char *out_character_path, size_t path_size) {
  if (character_name && character_name[0]) {
    char char_dir[768];
    if (get_character_chats_dir(character_name, char_dir, sizeof(char_dir))) {
      char filepath[1024];
      snprintf(filepath, sizeof(filepath), "%s/%s.json", char_dir, id);
      if (try_load_chat_from_path(history, filepath, out_character_path,
                                  path_size)) {
        return true;
      }
    }
  }

  const char *base_dir = get_chats_dir();
  if (!base_dir)
    return false;

  DIR *d = opendir(base_dir);
  if (!d)
    return false;

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char subdir_path[768];
    snprintf(subdir_path, sizeof(subdir_path), "%s/%s", base_dir,
             entry->d_name);

    struct stat st;
    if (stat(subdir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
      char filepath[1024];
      snprintf(filepath, sizeof(filepath), "%s/%s.json", subdir_path, id);
      if (try_load_chat_from_path(history, filepath, out_character_path,
                                  path_size)) {
        closedir(d);
        return true;
      }
    }
  }
  closedir(d);

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", base_dir, id);
  return try_load_chat_from_path(history, filepath, out_character_path,
                                 path_size);
}

bool chat_load_with_note(ChatHistory *history, AuthorNote *note, const char *id,
                         const char *character_name, char *out_character_path,
                         size_t path_size) {
  if (character_name && character_name[0]) {
    char char_dir[768];
    if (get_character_chats_dir(character_name, char_dir, sizeof(char_dir))) {
      char filepath[1024];
      snprintf(filepath, sizeof(filepath), "%s/%s.json", char_dir, id);
      if (try_load_chat_from_path_with_note(history, note, filepath,
                                            out_character_path, path_size)) {
        return true;
      }
    }
  }

  const char *base_dir = get_chats_dir();
  if (!base_dir)
    return false;

  DIR *d = opendir(base_dir);
  if (!d)
    return false;

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;
    char subdir[768];
    snprintf(subdir, sizeof(subdir), "%s/%s", base_dir, entry->d_name);
    struct stat st;
    if (stat(subdir, &st) != 0 || !S_ISDIR(st.st_mode))
      continue;

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", subdir, id);
    if (try_load_chat_from_path_with_note(history, note, filepath,
                                          out_character_path, path_size)) {
      closedir(d);
      return true;
    }
  }

  closedir(d);

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", base_dir, id);
  return try_load_chat_from_path_with_note(history, note, filepath,
                                           out_character_path, path_size);
}

bool chat_delete(const char *id, const char *character_name) {
  if (character_name && character_name[0]) {
    char char_dir[768];
    if (get_character_chats_dir(character_name, char_dir, sizeof(char_dir))) {
      char filepath[1024];
      snprintf(filepath, sizeof(filepath), "%s/%s.json", char_dir, id);
      if (unlink(filepath) == 0)
        return true;
    }
  }

  const char *base_dir = get_chats_dir();
  if (!base_dir)
    return false;

  DIR *d = opendir(base_dir);
  if (!d)
    return false;

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char subdir_path[768];
    snprintf(subdir_path, sizeof(subdir_path), "%s/%s", base_dir,
             entry->d_name);

    struct stat st;
    if (stat(subdir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
      char filepath[1024];
      snprintf(filepath, sizeof(filepath), "%s/%s.json", subdir_path, id);
      if (unlink(filepath) == 0) {
        closedir(d);
        return true;
      }
    }
  }
  closedir(d);

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/%s.json", base_dir, id);
  return unlink(filepath) == 0;
}

char *chat_generate_id(void) {
  char *id = malloc(32);
  if (!id)
    return NULL;
  time_t now = time(NULL);
  unsigned int r = (unsigned int)now ^ (unsigned int)getpid();
  snprintf(id, 32, "%08x%04x", (unsigned int)now, r & 0xFFFF);
  return id;
}

const char *chat_auto_title(const ChatHistory *history) {
  (void)history; // unused
  static char title[CHAT_TITLE_MAX];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(title, sizeof(title), "Chat %Y-%m-%d %H:%M", t);
  return title;
}

int chat_get_next_index(const char *character_name) {
  ChatList list;
  chat_list_init(&list);
  if (!chat_list_load_for_character(&list, character_name)) {
    return 1;
  }

  int max_index = 0;
  for (size_t i = 0; i < list.count; i++) {
    int idx = 0;
    if (sscanf(list.chats[i].title, "Chat %d", &idx) == 1) {
      if (idx > max_index)
        max_index = idx;
    }
  }

  chat_list_free(&list);
  return max_index + 1;
}

bool chat_find_by_title(const char *title, const char *character_name,
                        char *out_id, size_t id_size) {
  if (!title || !out_id || id_size == 0)
    return false;

  ChatList list;
  chat_list_init(&list);
  if (!chat_list_load_for_character(&list, character_name)) {
    return false;
  }

  bool found = false;
  for (size_t i = 0; i < list.count; i++) {
    if (strcmp(list.chats[i].title, title) == 0) {
      strncpy(out_id, list.chats[i].id, id_size - 1);
      out_id[id_size - 1] = '\0';
      found = true;
      break;
    }
  }

  chat_list_free(&list);
  return found;
}

bool chat_get_title_by_id(const char *id, const char *character_name,
                          char *out_title, size_t title_size) {
  if (!id || !out_title || title_size == 0)
    return false;

  ChatList list;
  chat_list_init(&list);
  if (!chat_list_load_for_character(&list, character_name)) {
    return false;
  }

  bool found = false;
  for (size_t i = 0; i < list.count; i++) {
    if (strcmp(list.chats[i].id, id) == 0) {
      strncpy(out_title, list.chats[i].title, title_size - 1);
      out_title[title_size - 1] = '\0';
      found = true;
      break;
    }
  }

  chat_list_free(&list);
  return found;
}

bool chat_auto_save(const ChatHistory *history, char *chat_id, size_t id_size,
                    const char *character_path, const char *character_name) {
  if (!history || !chat_id || id_size == 0)
    return false;

  if (history->count == 0)
    return false;

  bool is_new = (chat_id[0] == '\0');
  char title[CHAT_TITLE_MAX];

  if (is_new) {
    char *new_id = chat_generate_id();
    if (!new_id)
      return false;
    strncpy(chat_id, new_id, id_size - 1);
    chat_id[id_size - 1] = '\0';
    free(new_id);

    int next_index = chat_get_next_index(character_name);
    snprintf(title, sizeof(title), "Chat %d", next_index);
  } else {
    if (!chat_get_title_by_id(chat_id, character_name, title, sizeof(title))) {
      snprintf(title, sizeof(title), "Chat");
    }
  }

  return chat_save(history, chat_id, title, character_path, character_name);
}

bool chat_auto_save_with_note(const ChatHistory *history,
                              const AuthorNote *note, char *chat_id,
                              size_t id_size, const char *character_path,
                              const char *character_name) {
  if (!history || !chat_id || id_size == 0)
    return false;

  if (history->count == 0)
    return false;

  bool is_new = (chat_id[0] == '\0');
  char title[CHAT_TITLE_MAX];

  if (is_new) {
    char *new_id = chat_generate_id();
    if (!new_id)
      return false;
    strncpy(chat_id, new_id, id_size - 1);
    chat_id[id_size - 1] = '\0';
    free(new_id);

    int next_index = chat_get_next_index(character_name);
    snprintf(title, sizeof(title), "Chat %d", next_index);
  } else {
    if (!chat_get_title_by_id(chat_id, character_name, title, sizeof(title))) {
      snprintf(title, sizeof(title), "Chat");
    }
  }

  return chat_save_with_note(history, note, chat_id, title, character_path,
                             character_name);
}

void chat_character_list_init(ChatCharacterList *list) {
  list->characters = NULL;
  list->count = 0;
  list->capacity = 0;
}

void chat_character_list_free(ChatCharacterList *list) {
  free(list->characters);
  list->characters = NULL;
  list->count = 0;
  list->capacity = 0;
}

bool chat_character_list_load(ChatCharacterList *list) {
  chat_character_list_free(list);
  chat_character_list_init(list);

  const char *base_dir = get_chats_dir();
  if (!base_dir)
    return false;

  DIR *d = opendir(base_dir);
  if (!d)
    return false;

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char subdir_path[768];
    snprintf(subdir_path, sizeof(subdir_path), "%s/%s", base_dir,
             entry->d_name);

    struct stat st;
    if (stat(subdir_path, &st) != 0 || !S_ISDIR(st.st_mode))
      continue;

    DIR *char_d = opendir(subdir_path);
    if (!char_d)
      continue;

    size_t chat_count = 0;
    struct dirent *chat_entry;
    while ((chat_entry = readdir(char_d)) != NULL) {
      if (chat_entry->d_name[0] == '.')
        continue;
      size_t namelen = strlen(chat_entry->d_name);
      if (namelen >= 6 &&
          strcmp(chat_entry->d_name + namelen - 5, ".json") == 0) {
        chat_count++;
      }
    }
    closedir(char_d);

    if (chat_count == 0)
      continue;

    if (list->count >= list->capacity) {
      size_t newcap = list->capacity == 0 ? 8 : list->capacity * 2;
      ChatCharacter *tmp =
          realloc(list->characters, newcap * sizeof(ChatCharacter));
      if (!tmp)
        continue;
      list->characters = tmp;
      list->capacity = newcap;
    }

    ChatCharacter *ch = &list->characters[list->count];
    strncpy(ch->dirname, entry->d_name, CHAT_CHAR_NAME_MAX - 1);
    ch->dirname[CHAT_CHAR_NAME_MAX - 1] = '\0';

    char first_chat_path[1024] = {0};
    char_d = opendir(subdir_path);
    if (char_d) {
      while ((chat_entry = readdir(char_d)) != NULL) {
        if (chat_entry->d_name[0] == '.')
          continue;
        size_t namelen = strlen(chat_entry->d_name);
        if (namelen >= 6 &&
            strcmp(chat_entry->d_name + namelen - 5, ".json") == 0) {
          int written = snprintf(first_chat_path, sizeof(first_chat_path),
                                 "%s/%s", subdir_path, chat_entry->d_name);
          (void)written;
          break;
        }
      }
      closedir(char_d);
    }

    ch->name[0] = '\0';
    if (first_chat_path[0]) {
      char *content = read_file_contents(first_chat_path, NULL);
      if (content) {
        char *name = find_json_string(content, "character_name");
        if (name) {
          strncpy(ch->name, name, CHAT_CHAR_NAME_MAX - 1);
          ch->name[CHAT_CHAR_NAME_MAX - 1] = '\0';
          free(name);
        }
        free(content);
      }
    }

    if (!ch->name[0]) {
      strncpy(ch->name, ch->dirname, CHAT_CHAR_NAME_MAX - 1);
      ch->name[CHAT_CHAR_NAME_MAX - 1] = '\0';
    }

    ch->chat_count = chat_count;
    list->count++;
  }

  closedir(d);

  for (size_t i = 0; i < list->count; i++) {
    for (size_t j = i + 1; j < list->count; j++) {
      if (strcasecmp(list->characters[j].name, list->characters[i].name) < 0) {
        ChatCharacter tmp = list->characters[i];
        list->characters[i] = list->characters[j];
        list->characters[j] = tmp;
      }
    }
  }

  return true;
}

bool chat_load_latest(ChatHistory *history, const char *character_name,
                      char *out_character_path, size_t path_size,
                      char *out_chat_id, size_t id_size) {
  ChatList list;
  chat_list_init(&list);
  if (!chat_list_load_for_character(&list, character_name) || list.count == 0) {
    chat_list_free(&list);
    return false;
  }

  const char *id = list.chats[0].id;
  bool result =
      chat_load(history, id, character_name, out_character_path, path_size);

  if (result && out_chat_id && id_size > 0) {
    strncpy(out_chat_id, id, id_size - 1);
    out_chat_id[id_size - 1] = '\0';
  }

  chat_list_free(&list);
  return result;
}

bool chat_load_by_index(ChatHistory *history, const char *character_name,
                        int index, char *out_character_path, size_t path_size,
                        char *out_chat_id, size_t id_size) {
  if (index < 1)
    return false;

  ChatList list;
  chat_list_init(&list);
  if (!chat_list_load_for_character(&list, character_name)) {
    chat_list_free(&list);
    return false;
  }

  char target_title[32];
  snprintf(target_title, sizeof(target_title), "Chat %d", index);

  const char *id = NULL;
  for (size_t i = 0; i < list.count; i++) {
    if (strcmp(list.chats[i].title, target_title) == 0) {
      id = list.chats[i].id;
      break;
    }
  }

  if (!id) {
    chat_list_free(&list);
    return false;
  }

  bool result =
      chat_load(history, id, character_name, out_character_path, path_size);

  if (result && out_chat_id && id_size > 0) {
    strncpy(out_chat_id, id, id_size - 1);
    out_chat_id[id_size - 1] = '\0';
  }

  chat_list_free(&list);
  return result;
}

#include "lorebook.h"
#include "chat/history.h"
#include "core/platform.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lorebook_init(Lorebook *lb) {
  if (!lb)
    return;
  memset(lb, 0, sizeof(*lb));
  lb->next_uid = 1;
  lb->default_scan_depth = 50;
}

void lorebook_free(Lorebook *lb) {
  if (!lb)
    return;
  for (size_t i = 0; i < lb->entry_count; i++) {
    lore_entry_free(&lb->entries[i]);
  }
  free(lb->entries);
  lb->entries = NULL;
  lb->entry_count = 0;
  lb->entry_capacity = 0;
}

void lore_entry_init(LoreEntry *entry) {
  if (!entry)
    return;
  memset(entry, 0, sizeof(*entry));
  entry->position = LORE_POS_AFTER_CHAR;
  entry->role = LORE_ROLE_SYSTEM;
  entry->depth = 4;
  entry->scan_depth = 50;
  entry->probability = 1.0f;
}

void lore_entry_free(LoreEntry *entry) {
  if (!entry)
    return;
  for (size_t i = 0; i < entry->key_count; i++) {
    free(entry->keys[i]);
  }
  free(entry->keys);
  for (size_t i = 0; i < entry->key_secondary_count; i++) {
    free(entry->keys_secondary[i]);
  }
  free(entry->keys_secondary);
  free(entry->content);
  entry->keys = NULL;
  entry->keys_secondary = NULL;
  entry->content = NULL;
  entry->key_count = 0;
  entry->key_secondary_count = 0;
}

static LoreEntry lore_entry_copy(const LoreEntry *src) {
  LoreEntry dst;
  lore_entry_init(&dst);
  dst.uid = src->uid;
  strncpy(dst.comment, src->comment, LORE_COMMENT_MAX - 1);
  dst.constant = src->constant;
  dst.selective = src->selective;
  dst.order = src->order;
  dst.position = src->position;
  dst.depth = src->depth;
  dst.role = src->role;
  dst.disabled = src->disabled;
  dst.case_sensitive = src->case_sensitive;
  dst.match_whole_words = src->match_whole_words;
  dst.scan_depth = src->scan_depth;
  dst.probability = src->probability;
  if (src->content)
    dst.content = strdup(src->content);
  for (size_t i = 0; i < src->key_count; i++) {
    lore_entry_add_key(&dst, src->keys[i]);
  }
  for (size_t i = 0; i < src->key_secondary_count; i++) {
    lore_entry_add_secondary_key(&dst, src->keys_secondary[i]);
  }
  return dst;
}

int lorebook_add_entry(Lorebook *lb, const LoreEntry *entry) {
  if (!lb || !entry)
    return -1;
  if (lb->entry_count >= lb->entry_capacity) {
    size_t new_cap = lb->entry_capacity == 0 ? 8 : lb->entry_capacity * 2;
    LoreEntry *new_entries = realloc(lb->entries, new_cap * sizeof(LoreEntry));
    if (!new_entries)
      return -1;
    lb->entries = new_entries;
    lb->entry_capacity = new_cap;
  }
  LoreEntry copy = lore_entry_copy(entry);
  copy.uid = lb->next_uid++;
  lb->entries[lb->entry_count++] = copy;
  return copy.uid;
}

bool lorebook_remove_entry(Lorebook *lb, int uid) {
  if (!lb)
    return false;
  for (size_t i = 0; i < lb->entry_count; i++) {
    if (lb->entries[i].uid == uid) {
      lore_entry_free(&lb->entries[i]);
      memmove(&lb->entries[i], &lb->entries[i + 1],
              (lb->entry_count - i - 1) * sizeof(LoreEntry));
      lb->entry_count--;
      return true;
    }
  }
  return false;
}

LoreEntry *lorebook_get_entry(Lorebook *lb, int uid) {
  if (!lb)
    return NULL;
  for (size_t i = 0; i < lb->entry_count; i++) {
    if (lb->entries[i].uid == uid) {
      return &lb->entries[i];
    }
  }
  return NULL;
}

bool lorebook_toggle_entry(Lorebook *lb, int uid) {
  LoreEntry *entry = lorebook_get_entry(lb, uid);
  if (!entry)
    return false;
  entry->disabled = !entry->disabled;
  return true;
}

bool lore_entry_add_key(LoreEntry *entry, const char *key) {
  if (!entry || !key || entry->key_count >= LORE_MAX_KEYS)
    return false;
  char **new_keys =
      realloc(entry->keys, (entry->key_count + 1) * sizeof(char *));
  if (!new_keys)
    return false;
  entry->keys = new_keys;
  entry->keys[entry->key_count] = strdup(key);
  if (!entry->keys[entry->key_count])
    return false;
  entry->key_count++;
  return true;
}

bool lore_entry_add_secondary_key(LoreEntry *entry, const char *key) {
  if (!entry || !key || entry->key_secondary_count >= LORE_MAX_KEYS)
    return false;
  char **new_keys = realloc(entry->keys_secondary,
                            (entry->key_secondary_count + 1) * sizeof(char *));
  if (!new_keys)
    return false;
  entry->keys_secondary = new_keys;
  entry->keys_secondary[entry->key_secondary_count] = strdup(key);
  if (!entry->keys_secondary[entry->key_secondary_count])
    return false;
  entry->key_secondary_count++;
  return true;
}

void lore_entry_set_content(LoreEntry *entry, const char *content) {
  if (!entry)
    return;
  free(entry->content);
  entry->content = content ? strdup(content) : NULL;
}

static char *str_to_lower(const char *s) {
  if (!s)
    return NULL;
  size_t len = strlen(s);
  char *lower = malloc(len + 1);
  if (!lower)
    return NULL;
  for (size_t i = 0; i < len; i++) {
    lower[i] = (char)tolower((unsigned char)s[i]);
  }
  lower[len] = '\0';
  return lower;
}

static bool is_word_boundary(char c) {
  return !isalnum((unsigned char)c) && c != '_';
}

static bool find_word(const char *haystack, const char *needle,
                      bool case_sensitive, bool whole_word) {
  if (!haystack || !needle)
    return false;
  char *h = case_sensitive ? strdup(haystack) : str_to_lower(haystack);
  char *n = case_sensitive ? strdup(needle) : str_to_lower(needle);
  if (!h || !n) {
    free(h);
    free(n);
    return false;
  }
  const char *pos = h;
  size_t needle_len = strlen(n);
  bool found = false;
  while ((pos = strstr(pos, n)) != NULL) {
    if (!whole_word) {
      found = true;
      break;
    }
    bool start_ok = (pos == h) || is_word_boundary(*(pos - 1));
    bool end_ok =
        (pos[needle_len] == '\0') || is_word_boundary(pos[needle_len]);
    if (start_ok && end_ok) {
      found = true;
      break;
    }
    pos++;
  }
  free(h);
  free(n);
  return found;
}

bool lore_entry_matches(const LoreEntry *entry, const char *text) {
  if (!entry)
    return false;
  if (entry->disabled)
    return false;
  if (entry->constant)
    return true;
  bool primary_match = false;
  for (size_t i = 0; i < entry->key_count; i++) {
    if (find_word(text, entry->keys[i], entry->case_sensitive,
                  entry->match_whole_words)) {
      primary_match = true;
      break;
    }
  }
  if (!primary_match)
    return false;
  if (!entry->selective || entry->key_secondary_count == 0)
    return true;
  for (size_t i = 0; i < entry->key_secondary_count; i++) {
    if (find_word(text, entry->keys_secondary[i], entry->case_sensitive,
                  entry->match_whole_words)) {
      return true;
    }
  }
  return false;
}

void lore_match_init(LoreMatchResult *result) {
  if (!result)
    return;
  result->entries = NULL;
  result->count = 0;
  result->capacity = 0;
}

void lore_match_free(LoreMatchResult *result) {
  if (!result)
    return;
  free(result->entries);
  result->entries = NULL;
  result->count = 0;
  result->capacity = 0;
}

static bool lore_match_add(LoreMatchResult *result, LoreEntry *entry) {
  if (!result || !entry)
    return false;
  if (result->count >= result->capacity) {
    size_t new_cap = result->capacity == 0 ? 8 : result->capacity * 2;
    LoreEntry **new_entries =
        realloc(result->entries, new_cap * sizeof(LoreEntry *));
    if (!new_entries)
      return false;
    result->entries = new_entries;
    result->capacity = new_cap;
  }
  result->entries[result->count++] = entry;
  return true;
}

void lorebook_find_matches(const Lorebook *lb, const char *text,
                           LoreMatchResult *result) {
  if (!lb || !text || !result)
    return;
  for (size_t i = 0; i < lb->entry_count; i++) {
    if (lore_entry_matches(&lb->entries[i], text)) {
      lore_match_add(result, &lb->entries[i]);
    }
  }
}

const char *lore_position_to_string(LorePosition pos) {
  switch (pos) {
  case LORE_POS_BEFORE_CHAR:
    return "before_char";
  case LORE_POS_AFTER_CHAR:
    return "after_char";
  case LORE_POS_BEFORE_SCENARIO:
    return "before_scenario";
  case LORE_POS_AFTER_SCENARIO:
    return "after_scenario";
  case LORE_POS_AT_DEPTH:
    return "at_depth";
  default:
    return "after_char";
  }
}

LorePosition lore_position_from_string(const char *str) {
  if (!str)
    return LORE_POS_AFTER_CHAR;
  if (strcmp(str, "before_char") == 0)
    return LORE_POS_BEFORE_CHAR;
  if (strcmp(str, "after_char") == 0)
    return LORE_POS_AFTER_CHAR;
  if (strcmp(str, "before_scenario") == 0)
    return LORE_POS_BEFORE_SCENARIO;
  if (strcmp(str, "after_scenario") == 0)
    return LORE_POS_AFTER_SCENARIO;
  if (strcmp(str, "at_depth") == 0)
    return LORE_POS_AT_DEPTH;
  return LORE_POS_AFTER_CHAR;
}

const char *lore_role_to_string(LoreRole role) {
  switch (role) {
  case LORE_ROLE_SYSTEM:
    return "system";
  case LORE_ROLE_USER:
    return "user";
  case LORE_ROLE_ASSISTANT:
    return "assistant";
  default:
    return "system";
  }
}

LoreRole lore_role_from_string(const char *str) {
  if (!str)
    return LORE_ROLE_SYSTEM;
  if (strcmp(str, "system") == 0)
    return LORE_ROLE_SYSTEM;
  if (strcmp(str, "user") == 0)
    return LORE_ROLE_USER;
  if (strcmp(str, "assistant") == 0)
    return LORE_ROLE_ASSISTANT;
  return LORE_ROLE_SYSTEM;
}

static char *expand_path(const char *path) {
  if (!path)
    return NULL;
  if (path[0] == '~' && (path[1] == '/' || path[1] == '\0')) {
    const char *home = get_home_dir();
    if (!home)
      return strdup(path);
    size_t home_len = strlen(home);
    size_t path_len = strlen(path + 1);
    char *expanded = malloc(home_len + path_len + 1);
    if (!expanded)
      return NULL;
    memcpy(expanded, home, home_len);
    memcpy(expanded + home_len, path + 1, path_len + 1);
    return expanded;
  }
  return strdup(path);
}

static char *read_file_contents(const char *path) {
  char *expanded = expand_path(path);
  if (!expanded)
    return NULL;
  FILE *f = fopen(expanded, "rb");
  free(expanded);
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (size <= 0) {
    fclose(f);
    return NULL;
  }
  char *data = malloc(size + 1);
  if (!data) {
    fclose(f);
    return NULL;
  }
  size_t read = fread(data, 1, size, f);
  fclose(f);
  data[read] = '\0';
  return data;
}

static const char *skip_ws(const char *p) {
  while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    p++;
  return p;
}

static char *parse_json_string(const char **pp) {
  const char *p = *pp;
  p = skip_ws(p);
  if (*p != '"')
    return NULL;
  p++;
  const char *start = p;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p + 1))
      p += 2;
    else
      p++;
  }
  size_t len = p - start;
  char *result = malloc(len + 1);
  if (!result)
    return NULL;
  size_t j = 0;
  for (size_t i = 0; i < len; i++) {
    if (start[i] == '\\' && i + 1 < len) {
      i++;
      if (start[i] == 'n')
        result[j++] = '\n';
      else if (start[i] == 't')
        result[j++] = '\t';
      else if (start[i] == 'r')
        result[j++] = '\r';
      else
        result[j++] = start[i];
    } else {
      result[j++] = start[i];
    }
  }
  result[j] = '\0';
  if (*p == '"')
    p++;
  *pp = p;
  return result;
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
  *pp = p;
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

static bool parse_key_array(const char *json, const char *key, LoreEntry *entry,
                            bool secondary) {
  const char *p = find_key(json, key);
  if (!p || *p != '[')
    return false;
  p++;
  while (*p && *p != ']') {
    p = skip_ws(p);
    if (*p == '"') {
      char *val = parse_json_string(&p);
      if (val) {
        if (secondary)
          lore_entry_add_secondary_key(entry, val);
        else
          lore_entry_add_key(entry, val);
        free(val);
      }
    }
    p = skip_ws(p);
    if (*p == ',')
      p++;
  }
  return true;
}

static bool parse_entry(const char *json, LoreEntry *entry) {
  const char *p;
  p = find_key(json, "uid");
  if (p)
    entry->uid = parse_json_int(&p);
  if (!parse_key_array(json, "key", entry, false))
    parse_key_array(json, "keys", entry, false);
  if (!parse_key_array(json, "keysecondary", entry, true))
    parse_key_array(json, "secondary_keys", entry, true);
  p = find_key(json, "comment");
  if (!p)
    p = find_key(json, "name");
  if (p && *p == '"') {
    char *val = parse_json_string(&p);
    if (val) {
      strncpy(entry->comment, val, LORE_COMMENT_MAX - 1);
      free(val);
    }
  }
  p = find_key(json, "content");
  if (p && *p == '"') {
    char *val = parse_json_string(&p);
    if (val) {
      lore_entry_set_content(entry, val);
      free(val);
    }
  }
  p = find_key(json, "constant");
  if (p)
    entry->constant = parse_json_bool(&p);
  p = find_key(json, "selective");
  if (p)
    entry->selective = parse_json_bool(&p);
  p = find_key(json, "order");
  if (!p)
    p = find_key(json, "insertion_order");
  if (p)
    entry->order = parse_json_int(&p);
  p = find_key(json, "position");
  if (p)
    entry->position = (LorePosition)parse_json_int(&p);
  p = find_key(json, "depth");
  if (p)
    entry->depth = parse_json_int(&p);
  p = find_key(json, "role");
  if (p)
    entry->role = (LoreRole)parse_json_int(&p);
  p = find_key(json, "disable");
  if (p)
    entry->disabled = parse_json_bool(&p);
  p = find_key(json, "enabled");
  if (p)
    entry->disabled = !parse_json_bool(&p);
  p = find_key(json, "caseSensitive");
  if (!p)
    p = find_key(json, "case_sensitive");
  if (p)
    entry->case_sensitive = parse_json_bool(&p);
  p = find_key(json, "matchWholeWords");
  if (p)
    entry->match_whole_words = parse_json_bool(&p);
  p = find_key(json, "scanDepth");
  if (p)
    entry->scan_depth = parse_json_int(&p);
  p = find_key(json, "probability");
  if (p) {
    float prob = parse_json_float(&p);
    entry->probability = prob > 1.0f ? prob / 100.0f : prob;
  }
  return true;
}

bool lorebook_load_json(Lorebook *lb, const char *path) {
  if (!lb || !path)
    return false;
  char *data = read_file_contents(path);
  if (!data)
    return false;
  const char *p;
  p = find_key(data, "name");
  if (p && *p == '"') {
    char *val = parse_json_string(&p);
    if (val) {
      strncpy(lb->name, val, sizeof(lb->name) - 1);
      free(val);
    }
  }
  p = find_key(data, "description");
  if (p && *p == '"') {
    char *val = parse_json_string(&p);
    if (val) {
      strncpy(lb->description, val, sizeof(lb->description) - 1);
      free(val);
    }
  }
  p = find_key(data, "scan_depth");
  if (p)
    lb->default_scan_depth = parse_json_int(&p);
  p = find_key(data, "recursive_scanning");
  if (p)
    lb->recursive_scanning = parse_json_bool(&p);
  p = find_key(data, "entries");
  if (!p || *p != '{') {
    free(data);
    return false;
  }
  p++;
  while (*p && *p != '}') {
    p = skip_ws(p);
    if (*p == '"') {
      char *entry_key = parse_json_string(&p);
      free(entry_key);
      p = skip_ws(p);
      if (*p == ':')
        p++;
      p = skip_ws(p);
      if (*p == '{') {
        const char *entry_start = p;
        int depth = 1;
        p++;
        while (*p && depth > 0) {
          if (*p == '{')
            depth++;
          else if (*p == '}')
            depth--;
          p++;
        }
        size_t entry_len = p - entry_start;
        char *entry_json = malloc(entry_len + 1);
        if (entry_json) {
          memcpy(entry_json, entry_start, entry_len);
          entry_json[entry_len] = '\0';
          LoreEntry entry;
          lore_entry_init(&entry);
          parse_entry(entry_json, &entry);
          int uid = entry.uid;
          lorebook_add_entry(lb, &entry);
          LoreEntry *added = lorebook_get_entry(lb, lb->next_uid - 1);
          if (added)
            added->uid = uid;
          lore_entry_free(&entry);
          free(entry_json);
        }
      }
    }
    p = skip_ws(p);
    if (*p == ',')
      p++;
  }
  free(data);
  return lb->entry_count > 0;
}

bool lorebook_save_json(const Lorebook *lb, const char *path) {
  if (!lb || !path)
    return false;
  FILE *f = fopen(path, "w");
  if (!f)
    return false;
  fprintf(f, "{\n");
  fprintf(f, "  \"name\": \"%s\",\n", lb->name);
  fprintf(f, "  \"description\": \"%s\",\n", lb->description);
  fprintf(f, "  \"scan_depth\": %d,\n", lb->default_scan_depth);
  fprintf(f, "  \"recursive_scanning\": %s,\n",
          lb->recursive_scanning ? "true" : "false");
  fprintf(f, "  \"entries\": {\n");
  for (size_t i = 0; i < lb->entry_count; i++) {
    const LoreEntry *e = &lb->entries[i];
    fprintf(f, "    \"%d\": {\n", e->uid);
    fprintf(f, "      \"uid\": %d,\n", e->uid);
    fprintf(f, "      \"key\": [");
    for (size_t j = 0; j < e->key_count; j++) {
      fprintf(f, "\"%s\"%s", e->keys[j], j + 1 < e->key_count ? ", " : "");
    }
    fprintf(f, "],\n");
    fprintf(f, "      \"keysecondary\": [");
    for (size_t j = 0; j < e->key_secondary_count; j++) {
      fprintf(f, "\"%s\"%s", e->keys_secondary[j],
              j + 1 < e->key_secondary_count ? ", " : "");
    }
    fprintf(f, "],\n");
    fprintf(f, "      \"comment\": \"%s\",\n", e->comment);
    fprintf(f, "      \"content\": \"%s\",\n", e->content ? e->content : "");
    fprintf(f, "      \"constant\": %s,\n", e->constant ? "true" : "false");
    fprintf(f, "      \"selective\": %s,\n", e->selective ? "true" : "false");
    fprintf(f, "      \"order\": %d,\n", e->order);
    fprintf(f, "      \"position\": %d,\n", e->position);
    fprintf(f, "      \"depth\": %d,\n", e->depth);
    fprintf(f, "      \"role\": %d,\n", e->role);
    fprintf(f, "      \"disable\": %s,\n", e->disabled ? "true" : "false");
    fprintf(f, "      \"caseSensitive\": %s,\n",
            e->case_sensitive ? "true" : "false");
    fprintf(f, "      \"matchWholeWords\": %s,\n",
            e->match_whole_words ? "true" : "false");
    fprintf(f, "      \"scanDepth\": %d,\n", e->scan_depth);
    fprintf(f, "      \"probability\": %.0f\n", e->probability * 100);
    fprintf(f, "    }%s\n", i + 1 < lb->entry_count ? "," : "");
  }
  fprintf(f, "  }\n");
  fprintf(f, "}\n");
  fclose(f);
  return true;
}

static int compare_entries_by_order(const void *a, const void *b) {
  const LoreEntry *ea = *(const LoreEntry **)a;
  const LoreEntry *eb = *(const LoreEntry **)b;
  return eb->order - ea->order;
}

char *lorebook_build_context(const Lorebook *lb, const ChatHistory *history,
                             int scan_depth) {
  if (!lb || !history || lb->entry_count == 0)
    return NULL;
  size_t msg_count = history->count;
  if (msg_count == 0)
    return NULL;
  size_t text_cap = 8192;
  char *combined_text = malloc(text_cap);
  if (!combined_text)
    return NULL;
  combined_text[0] = '\0';
  size_t text_len = 0;
  int depth = scan_depth > 0 ? scan_depth : lb->default_scan_depth;
  size_t start = msg_count > (size_t)depth ? msg_count - depth : 0;
  for (size_t i = start; i < msg_count; i++) {
    const char *msg = history_get((ChatHistory *)history, i);
    if (!msg)
      continue;
    size_t msg_len = strlen(msg);
    if (text_len + msg_len + 2 > text_cap) {
      text_cap = text_cap * 2 + msg_len;
      char *tmp = realloc(combined_text, text_cap);
      if (!tmp) {
        free(combined_text);
        return NULL;
      }
      combined_text = tmp;
    }
    if (text_len > 0) {
      combined_text[text_len++] = '\n';
    }
    memcpy(combined_text + text_len, msg, msg_len);
    text_len += msg_len;
    combined_text[text_len] = '\0';
  }
  LoreMatchResult result;
  lore_match_init(&result);
  lorebook_find_matches(lb, combined_text, &result);
  free(combined_text);
  if (result.count == 0) {
    lore_match_free(&result);
    return NULL;
  }
  qsort(result.entries, result.count, sizeof(LoreEntry *),
        compare_entries_by_order);
  size_t out_cap = 4096;
  char *output = malloc(out_cap);
  if (!output) {
    lore_match_free(&result);
    return NULL;
  }
  output[0] = '\0';
  size_t out_len = 0;
  for (size_t i = 0; i < result.count; i++) {
    const LoreEntry *e = result.entries[i];
    if (!e->content)
      continue;
    size_t content_len = strlen(e->content);
    size_t needed = content_len + 3;
    if (out_len + needed > out_cap) {
      out_cap = out_cap * 2 + needed;
      char *tmp = realloc(output, out_cap);
      if (!tmp) {
        free(output);
        lore_match_free(&result);
        return NULL;
      }
      output = tmp;
    }
    if (out_len > 0) {
      output[out_len++] = '\n';
      output[out_len++] = '\n';
    }
    memcpy(output + out_len, e->content, content_len);
    out_len += content_len;
    output[out_len] = '\0';
  }
  lore_match_free(&result);
  return output;
}

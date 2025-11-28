#ifndef LOREBOOK_H
#define LOREBOOK_H

#include <stdbool.h>
#include <stddef.h>

#define LORE_COMMENT_MAX 128
#define LORE_MAX_KEYS 64
#define LORE_KEY_MAX_LEN 256

typedef enum {
  LORE_POS_BEFORE_CHAR = 0,
  LORE_POS_AFTER_CHAR = 1,
  LORE_POS_BEFORE_SCENARIO = 2,
  LORE_POS_AFTER_SCENARIO = 3,
  LORE_POS_AT_DEPTH = 4,
} LorePosition;

typedef enum {
  LORE_ROLE_SYSTEM = 0,
  LORE_ROLE_USER = 1,
  LORE_ROLE_ASSISTANT = 2,
} LoreRole;

typedef struct {
  int uid;
  char **keys;
  size_t key_count;
  char **keys_secondary;
  size_t key_secondary_count;
  char comment[LORE_COMMENT_MAX];
  char *content;
  bool constant;
  bool selective;
  int order;
  LorePosition position;
  int depth;
  LoreRole role;
  bool disabled;
  bool case_sensitive;
  bool match_whole_words;
  int scan_depth;
  float probability;
} LoreEntry;

typedef struct {
  char name[128];
  char description[256];
  LoreEntry *entries;
  size_t entry_count;
  size_t entry_capacity;
  int next_uid;
  bool recursive_scanning;
  int default_scan_depth;
} Lorebook;

void lorebook_init(Lorebook *lb);
void lorebook_free(Lorebook *lb);

void lore_entry_init(LoreEntry *entry);
void lore_entry_free(LoreEntry *entry);

int lorebook_add_entry(Lorebook *lb, const LoreEntry *entry);
bool lorebook_remove_entry(Lorebook *lb, int uid);
LoreEntry *lorebook_get_entry(Lorebook *lb, int uid);
bool lorebook_toggle_entry(Lorebook *lb, int uid);

bool lore_entry_add_key(LoreEntry *entry, const char *key);
bool lore_entry_add_secondary_key(LoreEntry *entry, const char *key);
void lore_entry_set_content(LoreEntry *entry, const char *content);

typedef struct {
  LoreEntry **entries;
  size_t count;
  size_t capacity;
} LoreMatchResult;

void lore_match_init(LoreMatchResult *result);
void lore_match_free(LoreMatchResult *result);

bool lore_entry_matches(const LoreEntry *entry, const char *text);
void lorebook_find_matches(const Lorebook *lb, const char *text,
                           LoreMatchResult *result);

bool lorebook_load_json(Lorebook *lb, const char *path);
bool lorebook_save_json(const Lorebook *lb, const char *path);

const char *lore_position_to_string(LorePosition pos);
LorePosition lore_position_from_string(const char *str);
const char *lore_role_to_string(LoreRole role);
LoreRole lore_role_from_string(const char *str);

#include "chat/history.h"
char *lorebook_build_context(const Lorebook *lb, const ChatHistory *history,
                             int scan_depth);

#endif

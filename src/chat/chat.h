#ifndef CHAT_H
#define CHAT_H

#include "chat/history.h"
#include <stdbool.h>
#include <time.h>

#define CHAT_ID_MAX 64
#define CHAT_TITLE_MAX 128
#define CHAT_CHAR_PATH_MAX 512
#define CHAT_CHAR_NAME_MAX 128

typedef struct {
  char id[CHAT_ID_MAX];
  char title[CHAT_TITLE_MAX];
  char character_path[CHAT_CHAR_PATH_MAX];
  char character_name[CHAT_CHAR_NAME_MAX];
  time_t created_at;
  time_t updated_at;
  size_t message_count;
} ChatMeta;

typedef struct {
  ChatMeta *chats;
  size_t count;
  size_t capacity;
} ChatList;

void chat_list_init(ChatList *list);
void chat_list_free(ChatList *list);

bool chat_list_load(ChatList *list);
bool chat_list_load_for_character(ChatList *list, const char *character_name);

bool chat_save(const ChatHistory *history, const char *id, const char *title,
               const char *character_path, const char *character_name);
bool chat_load(ChatHistory *history, const char *id, const char *character_name,
               char *out_character_path, size_t path_size);
bool chat_delete(const char *id, const char *character_name);

char *chat_generate_id(void);
const char *chat_auto_title(const ChatHistory *history);

bool chat_find_by_title(const char *title, const char *character_name,
                        char *out_id, size_t id_size);
bool chat_get_title_by_id(const char *id, const char *character_name,
                          char *out_title, size_t title_size);

void chat_sanitize_dirname(const char *name, char *out, size_t out_size);

bool chat_auto_save(const ChatHistory *history, char *chat_id, size_t id_size,
                    const char *character_path, const char *character_name);

typedef struct {
  char name[CHAT_CHAR_NAME_MAX];
  char dirname[CHAT_CHAR_NAME_MAX];
  size_t chat_count;
} ChatCharacter;

typedef struct {
  ChatCharacter *characters;
  size_t count;
  size_t capacity;
} ChatCharacterList;

void chat_character_list_init(ChatCharacterList *list);
void chat_character_list_free(ChatCharacterList *list);
bool chat_character_list_load(ChatCharacterList *list);

int chat_get_next_index(const char *character_name);

bool chat_load_latest(ChatHistory *history, const char *character_name,
                      char *out_character_path, size_t path_size,
                      char *out_chat_id, size_t id_size);

bool chat_load_by_index(ChatHistory *history, const char *character_name,
                        int index, char *out_character_path, size_t path_size,
                        char *out_chat_id, size_t id_size);

#endif

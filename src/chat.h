#ifndef CHAT_H
#define CHAT_H

#include "history.h"
#include <stdbool.h>
#include <time.h>

#define CHAT_ID_MAX 64
#define CHAT_TITLE_MAX 128
#define CHAT_CHAR_PATH_MAX 512

typedef struct {
  char id[CHAT_ID_MAX];
  char title[CHAT_TITLE_MAX];
  char character_path[CHAT_CHAR_PATH_MAX];
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

bool chat_save(const ChatHistory *history, const char *id, const char *title,
               const char *character_path);
bool chat_load(ChatHistory *history, const char *id, char *out_character_path,
               size_t path_size);
bool chat_delete(const char *id);
char *chat_generate_id(void);
const char *chat_auto_title(const ChatHistory *history);

#endif

#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdbool.h>
#include <stddef.h>

#define CHAR_NAME_MAX 128
#define CHAR_FIELD_MAX 131072
#define CHAR_GREETINGS_MAX 16

typedef struct {
  char name[CHAR_NAME_MAX];
  char *description;
  char *personality;
  char *scenario;
  char *first_mes;
  char *mes_example;
  char *system_prompt;
  char *post_history_instructions;
  char *creator_notes;
  char *creator;
  char *character_version;
  char **alternate_greetings;
  size_t alternate_greetings_count;
  char **tags;
  size_t tags_count;
} CharacterCard;

bool character_load(CharacterCard *card, const char *path);
bool character_load_json(CharacterCard *card, const char *path);
bool character_load_png(CharacterCard *card, const char *path);
void character_free(CharacterCard *card);
const char *character_get_greeting(const CharacterCard *card, size_t index);
char *character_copy_to_config(const char *src_path);

#endif

#ifndef MODAL_H
#define MODAL_H

#include "character.h"
#include "chat.h"
#include "config.h"
#include "persona.h"
#include <ncurses.h>
#include <stdbool.h>

typedef enum {
  MODAL_NONE,
  MODAL_MODEL_SET,
  MODAL_MODEL_LIST,
  MODAL_MESSAGE,
  MODAL_CHAT_LIST,
  MODAL_CHAT_SAVE,
  MODAL_EXIT_CONFIRM,
  MODAL_PERSONA_EDIT,
  MODAL_CHARACTER_INFO,
  MODAL_GREETING_SELECT
} ModalType;

typedef struct {
  ModalType type;
  WINDOW *win;
  int width;
  int height;
  int start_y;
  int start_x;

  int field_index;
  char fields[4][256];
  int field_cursor[4];
  int field_len[4];

  int list_selection;
  int list_scroll;

  char message[512];
  bool message_is_error;

  ChatList chat_list;
  char current_chat_id[CHAT_ID_MAX];
  char character_path[CHAT_CHAR_PATH_MAX];

  bool exit_dont_ask;

  const CharacterCard *character;
  size_t greeting_selection;
} Modal;

typedef enum {
  MODAL_RESULT_NONE,
  MODAL_RESULT_CHAT_LOADED,
  MODAL_RESULT_CHAT_DELETED,
  MODAL_RESULT_CHAT_SAVED,
  MODAL_RESULT_CHAT_NEW,
  MODAL_RESULT_EXIT_CONFIRMED,
  MODAL_RESULT_EXIT_CANCELLED,
  MODAL_RESULT_PERSONA_SAVED,
  MODAL_RESULT_GREETING_SELECTED
} ModalResult;

void modal_init(Modal *m);
void modal_open_model_set(Modal *m);
void modal_open_model_list(Modal *m, const ModelsFile *mf);
void modal_open_message(Modal *m, const char *msg, bool is_error);
void modal_open_chat_list(Modal *m);
void modal_open_chat_save(Modal *m, const char *current_id,
                          const char *character_path);
void modal_open_exit_confirm(Modal *m);
void modal_open_persona_edit(Modal *m, const Persona *persona);
void modal_open_character_info(Modal *m, const CharacterCard *card);
void modal_open_greeting_select(Modal *m, const CharacterCard *card);
void modal_close(Modal *m);
void modal_draw(Modal *m, const ModelsFile *mf);
ModalResult modal_handle_key(Modal *m, int ch, ModelsFile *mf,
                             ChatHistory *history, char *loaded_chat_id,
                             char *loaded_char_path, size_t char_path_size,
                             Persona *persona, size_t *selected_greeting);
bool modal_is_open(const Modal *m);
bool modal_get_exit_dont_ask(const Modal *m);

#endif

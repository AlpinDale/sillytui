#ifndef MODAL_H
#define MODAL_H

#include "character/character.h"
#include "character/persona.h"
#include "chat/chat.h"
#include "core/config.h"
#include "inference/tokenizer/selector.h"
#include "llm/sampler.h"
#include <curses.h>
#include <stdbool.h>

typedef enum {
  MODAL_NONE,
  MODAL_MODEL_SET,
  MODAL_MODEL_LIST,
  MODAL_MODEL_EDIT,
  MODAL_MESSAGE,
  MODAL_CHAT_LIST,
  MODAL_CHAT_SAVE,
  MODAL_CHAT_OVERWRITE_CONFIRM,
  MODAL_EXIT_CONFIRM,
  MODAL_PERSONA_EDIT,
  MODAL_CHARACTER_INFO,
  MODAL_GREETING_SELECT,
  MODAL_MESSAGE_EDIT,
  MODAL_MESSAGE_DELETE_CONFIRM,
  MODAL_SAMPLER_SETTINGS,
  MODAL_SAMPLER_YAML,
  MODAL_TOKENIZE
} ModalType;

typedef struct {
  ModalType type;
  WINDOW *win;
  int width;
  int height;
  int start_y;
  int start_x;

  int field_index;
  char fields[6][256];
  int field_cursor[6];
  int field_len[6];
  ApiType api_type_selection;
  TokenizerSelection tokenizer_selection;

  char **fetched_models;
  size_t fetched_models_count;
  int fetched_model_index;

  int list_selection;
  int list_scroll;

  char message[512];
  bool message_is_error;

  ChatList chat_list;
  char current_chat_id[CHAT_ID_MAX];
  char character_path[CHAT_CHAR_PATH_MAX];
  char character_name[CHAT_CHAR_NAME_MAX];

  char pending_save_title[CHAT_TITLE_MAX];
  char existing_chat_id[CHAT_ID_MAX];

  int edit_msg_index;
  char edit_buffer[4096];
  int edit_cursor;
  int edit_len;
  int edit_scroll;

  int edit_model_index; // Index of model being edited

  bool exit_dont_ask;

  const CharacterCard *character;
  size_t greeting_selection;

  SamplerSettings sampler;
  ApiType sampler_api_type;
  int sampler_field_index;
  int sampler_scroll;
  char sampler_input[32];
  int sampler_input_cursor;
  bool sampler_adding_custom;
  int sampler_custom_field;
  char sampler_custom_name[64];
  char sampler_custom_value[256];
  char sampler_custom_min[32];
  char sampler_custom_max[32];
  char sampler_custom_step[32];
  SamplerValueType sampler_custom_type;
  int sampler_custom_cursor;
  bool sampler_editing_list;
  int sampler_list_index;
  int sampler_list_scroll;
  char sampler_list_input[256];
  int sampler_list_input_cursor;
  bool sampler_editing_dict;
  int sampler_dict_index;
  int sampler_dict_scroll;
  int sampler_dict_field;
  char sampler_dict_key[32];
  char sampler_dict_val[64];
  int sampler_dict_cursor;
  bool sampler_dict_val_is_str;
  char sampler_yaml_buffer[4096];
  int sampler_yaml_cursor;
  int sampler_yaml_scroll;

  char tokenize_buffer[4096];
  int tokenize_cursor;
  int tokenize_len;
  int tokenize_scroll;
  int tokenize_count;
  void *tokenizer_ctx;
  uint32_t *tokenize_ids;
  size_t *tokenize_offsets;
  size_t tokenize_ids_count;
  size_t tokenize_ids_cap;
  int tokenize_ids_scroll;
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
  MODAL_RESULT_GREETING_SELECTED,
  MODAL_RESULT_MESSAGE_EDITED,
  MODAL_RESULT_MESSAGE_DELETED,
  MODAL_RESULT_SAMPLER_SAVED
} ModalResult;

void modal_init(Modal *m);
void modal_open_model_set(Modal *m);
void modal_open_model_list(Modal *m, const ModelsFile *mf);
void modal_open_model_edit(Modal *m, const ModelsFile *mf, int model_index);
void modal_open_message(Modal *m, const char *msg, bool is_error);
void modal_open_chat_list(Modal *m);
void modal_open_chat_save(Modal *m, const char *current_id,
                          const char *character_path,
                          const char *character_name);
void modal_open_exit_confirm(Modal *m);
void modal_open_persona_edit(Modal *m, const Persona *persona);
void modal_open_character_info(Modal *m, const CharacterCard *card);
void modal_open_greeting_select(Modal *m, const CharacterCard *card);
void modal_open_message_edit(Modal *m, int msg_index, const char *content);
void modal_open_message_delete(Modal *m, int msg_index);
void modal_open_sampler_settings(Modal *m, ApiType api_type);
void modal_open_sampler_yaml(Modal *m, SamplerSettings *sampler,
                             ApiType api_type);
void modal_open_tokenize(Modal *m, void *tokenizer_ctx);
int modal_get_edit_msg_index(const Modal *m);
const char *modal_get_edit_content(const Modal *m);
void modal_close(Modal *m);
void modal_draw(Modal *m, const ModelsFile *mf);
ModalResult modal_handle_key(Modal *m, int ch, ModelsFile *mf,
                             ChatHistory *history, char *loaded_chat_id,
                             char *loaded_char_path, size_t char_path_size,
                             Persona *persona, size_t *selected_greeting);
bool modal_is_open(const Modal *m);
bool modal_get_exit_dont_ask(const Modal *m);

#endif

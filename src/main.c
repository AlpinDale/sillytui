#include <ctype.h>
#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "character.h"
#include "chat.h"
#include "config.h"
#include "history.h"
#include "llm.h"
#include "macros.h"
#include "markdown.h"
#include "modal.h"
#include "persona.h"
#include "ui.h"

#define INPUT_MAX 4096

static const char *SPINNER_FRAMES[] = {"thinking", "thinking.", "thinking..",
                                       "thinking..."};
#define SPINNER_FRAME_COUNT 4

typedef struct {
  ChatHistory *history;
  WINDOW *chat_win;
  WINDOW *input_win;
  size_t msg_index;
  char *buffer;
  size_t buf_cap;
  size_t buf_len;
  int spinner_frame;
  long long last_spinner_update;
  int *selected_msg;
  const char *model_name;
  const char *user_name;
  const char *bot_name;
} StreamContext;

static long long get_time_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static bool is_only_whitespace(const char *text) {
  while (*text) {
    if (!isspace((unsigned char)*text)) {
      return false;
    }
    text++;
  }
  return true;
}

static void stream_callback(const char *chunk, void *userdata) {
  StreamContext *ctx = userdata;

  const char *to_append = chunk;
  if (ctx->buf_len == 0) {
    while (*to_append == '\n' || *to_append == '\r' || *to_append == ' ')
      to_append++;
  }
  if (*to_append == '\0')
    return;

  size_t chunk_len = strlen(to_append);
  if (ctx->buf_len + chunk_len + 1 > ctx->buf_cap) {
    size_t newcap = ctx->buf_cap == 0 ? 1024 : ctx->buf_cap * 2;
    while (newcap < ctx->buf_len + chunk_len + 1)
      newcap *= 2;
    char *tmp = realloc(ctx->buffer, newcap);
    if (!tmp)
      return;
    ctx->buffer = tmp;
    ctx->buf_cap = newcap;
  }

  memcpy(ctx->buffer + ctx->buf_len, to_append, chunk_len);
  ctx->buf_len += chunk_len;
  ctx->buffer[ctx->buf_len] = '\0';

  char display[8192];
  snprintf(display, sizeof(display), "Bot: %s", ctx->buffer);
  history_update(ctx->history, ctx->msg_index, display);
  *ctx->selected_msg = MSG_SELECT_NONE;
  ui_draw_chat(ctx->chat_win, ctx->history, *ctx->selected_msg, ctx->model_name,
               ctx->user_name, ctx->bot_name);
  ui_draw_input_multiline(ctx->input_win, "", 0, true, 0);
}

static void progress_callback(void *userdata) {
  StreamContext *ctx = userdata;

  long long now = get_time_ms();
  if (now - ctx->last_spinner_update < 200) {
    return;
  }
  ctx->last_spinner_update = now;

  ctx->spinner_frame = (ctx->spinner_frame + 1) % SPINNER_FRAME_COUNT;

  char display[128];
  snprintf(display, sizeof(display), "Bot: *%s*",
           SPINNER_FRAMES[ctx->spinner_frame]);
  history_update(ctx->history, ctx->msg_index, display);
  ui_draw_chat(ctx->chat_win, ctx->history, *ctx->selected_msg, ctx->model_name,
               ctx->user_name, ctx->bot_name);
  ui_draw_input_multiline(ctx->input_win, "", 0, true, 0);
}

static const char *get_model_name(ModelsFile *mf) {
  ModelConfig *model = config_get_active(mf);
  return model ? model->name : NULL;
}

static const char *get_user_display_name(const Persona *persona) {
  const char *name = persona_get_name(persona);
  return (name && name[0]) ? name : "You";
}

static const char *get_bot_display_name(const CharacterCard *character,
                                        bool loaded) {
  return (loaded && character->name[0]) ? character->name : "Bot";
}

static void do_llm_reply(ChatHistory *history, WINDOW *chat_win,
                         WINDOW *input_win, const char *user_input,
                         ModelsFile *mf, int *selected_msg,
                         const LLMContext *llm_ctx, const char *user_name,
                         const char *bot_name) {
  ModelConfig *model = config_get_active(mf);
  const char *model_name = get_model_name(mf);
  if (!model) {
    history_add(history, "Bot: *looks confused* \"No model configured. Use "
                         "/model set to add one.\"");
    *selected_msg = MSG_SELECT_NONE;
    ui_draw_chat(chat_win, history, *selected_msg, NULL, user_name, bot_name);
    return;
  }

  size_t msg_index = history_add(history, "Bot: *thinking*");
  if (msg_index == SIZE_MAX)
    return;
  *selected_msg = MSG_SELECT_NONE;
  ui_draw_chat(chat_win, history, *selected_msg, model_name, user_name,
               bot_name);
  ui_draw_input_multiline(input_win, "", 0, true, 0);

  StreamContext ctx = {.history = history,
                       .chat_win = chat_win,
                       .input_win = input_win,
                       .msg_index = msg_index,
                       .buffer = NULL,
                       .buf_cap = 0,
                       .buf_len = 0,
                       .spinner_frame = 0,
                       .last_spinner_update = get_time_ms(),
                       .selected_msg = selected_msg,
                       .model_name = model_name,
                       .user_name = user_name,
                       .bot_name = bot_name};

  ChatHistory hist_for_llm = {.messages = history->messages,
                              .count = history->count - 1,
                              .capacity = history->capacity};

  LLMResponse resp = llm_chat(model, &hist_for_llm, llm_ctx, stream_callback,
                              progress_callback, &ctx);

  if (!resp.success) {
    char err_msg[512];
    snprintf(err_msg, sizeof(err_msg), "Bot: *frowns* \"Error: %s\"",
             resp.error);
    history_update(history, msg_index, err_msg);
    *selected_msg = MSG_SELECT_NONE;
    ui_draw_chat(chat_win, history, *selected_msg, model_name, user_name,
                 bot_name);
  } else if (ctx.buf_len == 0) {
    history_update(history, msg_index, "Bot: *stays silent*");
    *selected_msg = MSG_SELECT_NONE;
    ui_draw_chat(chat_win, history, *selected_msg, model_name, user_name,
                 bot_name);
  }

  free(ctx.buffer);
  llm_response_free(&resp);
}

static const SlashCommand SLASH_COMMANDS[] = {
    {"model set", "Add a new model configuration"},
    {"model list", "Select from saved models"},
    {"chat save", "Save current chat"},
    {"chat load", "Load a saved chat"},
    {"chat new", "Start a new chat"},
    {"char load", "Load a character card"},
    {"char info", "Show character info"},
    {"char greetings", "Select alternate greeting"},
    {"char unload", "Unload current character"},
    {"persona set", "Edit your persona"},
    {"persona info", "Show your persona"},
    {"help", "Show available commands"},
    {"clear", "Clear chat history"},
    {"quit", "Exit the application"},
};
#define SLASH_COMMAND_COUNT (sizeof(SLASH_COMMANDS) / sizeof(SLASH_COMMANDS[0]))

static bool handle_slash_command(const char *input, Modal *modal,
                                 ModelsFile *mf, ChatHistory *history,
                                 char *current_chat_id, char *current_char_path,
                                 CharacterCard *character, Persona *persona,
                                 bool *char_loaded) {
  if (strcmp(input, "/model set") == 0) {
    modal_open_model_set(modal);
    return true;
  }
  if (strcmp(input, "/model list") == 0) {
    modal_open_model_list(modal, mf);
    return true;
  }
  if (strcmp(input, "/chat save") == 0) {
    modal_open_chat_save(modal, current_chat_id, current_char_path);
    return true;
  }
  if (strcmp(input, "/chat load") == 0) {
    modal_open_chat_list(modal);
    return true;
  }
  if (strncmp(input, "/chat load ", 11) == 0) {
    const char *id = input + 11;
    while (*id == ' ')
      id++;
    if (*id) {
      char loaded_char_path[CHAT_CHAR_PATH_MAX] = {0};
      if (chat_load(history, id, loaded_char_path, sizeof(loaded_char_path))) {
        strncpy(current_chat_id, id, CHAT_ID_MAX - 1);
        current_chat_id[CHAT_ID_MAX - 1] = '\0';
        if (loaded_char_path[0]) {
          if (*char_loaded) {
            character_free(character);
          }
          if (character_load(character, loaded_char_path)) {
            *char_loaded = true;
            strncpy(current_char_path, loaded_char_path,
                    CHAT_CHAR_PATH_MAX - 1);
            current_char_path[CHAT_CHAR_PATH_MAX - 1] = '\0';
          } else {
            *char_loaded = false;
            current_char_path[0] = '\0';
          }
        } else {
          if (*char_loaded) {
            character_free(character);
            *char_loaded = false;
          }
          current_char_path[0] = '\0';
        }
      } else {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Chat '%s' not found", id);
        modal_open_message(modal, err_msg, true);
      }
    }
    return true;
  }
  if (strcmp(input, "/chat new") == 0) {
    history_free(history);
    history_init(history);
    if (*char_loaded && character->first_mes && character->first_mes[0]) {
      char *substituted = macro_substitute(
          character->first_mes, character->name, persona_get_name(persona));
      if (substituted) {
        char first_msg[4096];
        snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
        history_add(history, first_msg);
        free(substituted);
      }
    } else {
      history_add(history, "Bot: *waves* \"New chat started!\"");
    }
    current_chat_id[0] = '\0';
    return true;
  }
  if (strcmp(input, "/char load") == 0 ||
      strncmp(input, "/char load ", 11) == 0) {
    const char *path = NULL;
    if (strlen(input) > 11) {
      path = input + 11;
      while (*path == ' ')
        path++;
    }
    if (path && *path) {
      if (character_load(character, path)) {
        *char_loaded = true;
        char *config_path = character_copy_to_config(path);
        if (config_path) {
          strncpy(current_char_path, config_path, CHAT_CHAR_PATH_MAX - 1);
          current_char_path[CHAT_CHAR_PATH_MAX - 1] = '\0';
          free(config_path);
        } else {
          strncpy(current_char_path, path, CHAT_CHAR_PATH_MAX - 1);
          current_char_path[CHAT_CHAR_PATH_MAX - 1] = '\0';
        }
        history_free(history);
        history_init(history);
        current_chat_id[0] = '\0';
        if (character->first_mes && character->first_mes[0]) {
          char *substituted = macro_substitute(
              character->first_mes, character->name, persona_get_name(persona));
          if (substituted) {
            char first_msg[4096];
            snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
            history_add(history, first_msg);
            free(substituted);
          }
        } else {
          char welcome[256];
          snprintf(welcome, sizeof(welcome), "Bot: *%s appears* \"Hello!\"",
                   character->name);
          history_add(history, welcome);
        }
      } else {
        char err_msg[512];
        snprintf(
            err_msg, sizeof(err_msg),
            "Failed to load character card:\n%s\n\nMake sure the file exists "
            "and is a valid character card (.json or .png with embedded data).",
            path);
        modal_open_message(modal, err_msg, true);
      }
    } else {
      modal_open_message(modal,
                         "Usage: /char load <path>\n\nExample:\n/char load "
                         "~/cards/char.json\n/char load ~/cards/char.png",
                         false);
    }
    return true;
  }
  if (strcmp(input, "/char info") == 0) {
    if (*char_loaded) {
      modal_open_character_info(modal, character);
    } else {
      modal_open_message(modal, "No character loaded", true);
    }
    return true;
  }
  if (strcmp(input, "/char greetings") == 0) {
    if (*char_loaded) {
      modal_open_greeting_select(modal, character);
    } else {
      modal_open_message(modal, "No character loaded", true);
    }
    return true;
  }
  if (strcmp(input, "/char unload") == 0) {
    if (*char_loaded) {
      character_free(character);
      *char_loaded = false;
      current_char_path[0] = '\0';
      history_add(history, "Bot: *Character unloaded*");
    } else {
      modal_open_message(modal, "No character loaded", true);
    }
    return true;
  }
  if (strcmp(input, "/persona set") == 0) {
    modal_open_persona_edit(modal, persona);
    return true;
  }
  if (strcmp(input, "/persona info") == 0) {
    char info[512];
    snprintf(info, sizeof(info), "Name: %s\n\nDescription:\n%s",
             persona_get_name(persona), persona_get_description(persona));
    modal_open_message(modal, info, false);
    return true;
  }
  if (strcmp(input, "/help") == 0) {
    modal_open_message(modal,
                       "/model set - Add a new model\n"
                       "/model list - Select a model\n"
                       "/chat save - Save current chat\n"
                       "/chat load - Load a saved chat\n"
                       "/chat new - Start new chat\n"
                       "/char load - Load character\n"
                       "/char info - Character info\n"
                       "/char greetings - Greetings\n"
                       "/persona set - Edit persona\n"
                       "/clear - Clear chat history\n"
                       "/quit - Exit\n"
                       "\n"
                       "Shortcuts:\n"
                       "Up/Down - Scroll chat\n"
                       "Esc - Close / Exit",
                       false);
    return true;
  }
  if (strcmp(input, "/clear") == 0) {
    history_free(history);
    history_init(history);
    if (*char_loaded && character->first_mes && character->first_mes[0]) {
      char *substituted = macro_substitute(
          character->first_mes, character->name, persona_get_name(persona));
      if (substituted) {
        char first_msg[4096];
        snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
        history_add(history, first_msg);
        free(substituted);
      }
    } else {
      history_add(history, "Bot: *clears throat* \"Fresh start!\"");
    }
    return true;
  }
  return false;
}

int main(void) {
  ChatHistory history;
  history_init(&history);

  ModelsFile models;
  config_load_models(&models);

  AppSettings settings;
  config_load_settings(&settings);

  Persona persona;
  persona_load(&persona);

  CharacterCard character;
  memset(&character, 0, sizeof(character));
  bool character_loaded = false;

  llm_init();

  setlocale(LC_ALL, "");

  if (initscr() == NULL) {
    fprintf(stderr, "Failed to initialize ncurses.\n");
    return EXIT_FAILURE;
  }

  set_escdelay(1);
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  markdown_init_colors();
  ui_init_colors();

  int current_input_height = 3;

  WINDOW *chat_win = NULL;
  WINDOW *input_win = NULL;
  ui_layout_windows_with_input(&chat_win, &input_win, current_input_height);

  Modal modal;
  modal_init(&modal);

  SuggestionBox suggestions;
  suggestion_box_init(&suggestions, SLASH_COMMANDS, SLASH_COMMAND_COUNT);

  char input_buffer[INPUT_MAX] = {0};
  int input_len = 0;
  int cursor_pos = 0;
  int input_scroll_line = 0;
  int selected_msg = MSG_SELECT_NONE;
  bool input_focused = true;
  bool running = true;
  char current_chat_id[CHAT_ID_MAX] = {0};
  char current_char_path[CHAT_CHAR_PATH_MAX] = {0};

  if (models.count == 0) {
    history_add(&history, "Bot: *waves* \"Welcome! Use /model set to configure "
                          "an LLM, or /help for commands.\"");
  } else {
    history_add(&history, "Bot: *waves* \"Ready to chat!\"");
  }

  const char *user_disp = get_user_display_name(&persona);
  const char *bot_disp = get_bot_display_name(&character, character_loaded);
  ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
               user_disp, bot_disp);
  ui_draw_input_multiline(input_win, input_buffer, cursor_pos, input_focused,
                          input_scroll_line);

  while (running) {
    user_disp = get_user_display_name(&persona);
    bot_disp = get_bot_display_name(&character, character_loaded);
    WINDOW *active_win = modal_is_open(&modal) ? modal.win : input_win;
    int ch = wgetch(active_win);

    if (ch == KEY_RESIZE) {
      ui_layout_windows_with_input(&chat_win, &input_win, current_input_height);
      selected_msg = MSG_SELECT_NONE;
      input_focused = true;
      ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                   user_disp, bot_disp);
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line);
      if (modal_is_open(&modal)) {
        modal_close(&modal);
      }
      continue;
    }

    if (modal_is_open(&modal)) {
      size_t selected_greeting = 0;
      ModalResult result = modal_handle_key(
          &modal, ch, &models, &history, current_chat_id, current_char_path,
          sizeof(current_char_path), &persona, &selected_greeting);
      if (result == MODAL_RESULT_CHAT_LOADED) {
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
        if (current_char_path[0]) {
          if (character_loaded) {
            character_free(&character);
          }
          if (character_load(&character, current_char_path)) {
            character_loaded = true;
          } else {
            current_char_path[0] = '\0';
            character_loaded = false;
          }
        } else {
          if (character_loaded) {
            character_free(&character);
            character_loaded = false;
          }
        }
        user_disp = get_user_display_name(&persona);
        bot_disp = get_bot_display_name(&character, character_loaded);
      }
      if (result == MODAL_RESULT_CHAT_NEW) {
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
      }
      if (result == MODAL_RESULT_EXIT_CONFIRMED) {
        if (modal_get_exit_dont_ask(&modal)) {
          settings.skip_exit_confirm = true;
          config_save_settings(&settings);
        }
        running = false;
      }
      if (result == MODAL_RESULT_GREETING_SELECTED && character_loaded) {
        const char *greeting =
            character_get_greeting(&character, selected_greeting);
        if (greeting) {
          history_free(&history);
          history_init(&history);
          current_chat_id[0] = '\0';
          char *substituted = macro_substitute(greeting, character.name,
                                               persona_get_name(&persona));
          if (substituted) {
            char first_msg[4096];
            snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
            history_add(&history, first_msg);
            free(substituted);
          }
          selected_msg = MSG_SELECT_NONE;
          input_focused = true;
        }
      }
      if (modal_is_open(&modal)) {
        modal_draw(&modal, &models);
      } else {
        touchwin(chat_win);
        touchwin(input_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp);
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);
      }
      continue;
    }

    if (ch == 27) {
      if (suggestion_box_is_open(&suggestions)) {
        suggestion_box_close(&suggestions);
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp);
        continue;
      }
      if (settings.skip_exit_confirm) {
        break;
      }
      modal_open_exit_confirm(&modal);
      modal_draw(&modal, &models);
      continue;
    }

    if (suggestion_box_is_open(&suggestions)) {
      if (ch == KEY_UP) {
        suggestion_box_navigate(&suggestions, -1);
        suggestion_box_draw(&suggestions);
        continue;
      }
      if (ch == KEY_DOWN) {
        suggestion_box_navigate(&suggestions, 1);
        suggestion_box_draw(&suggestions);
        continue;
      }
      if (ch == '\t' || ch == '\n' || ch == '\r') {
        const char *selected = suggestion_box_get_selected(&suggestions);
        if (selected) {
          if (suggestions.showing_dynamic) {
            snprintf(input_buffer, INPUT_MAX, "/chat load %s", selected);
          } else {
            snprintf(input_buffer, INPUT_MAX, "/%s", selected);
          }
          input_len = (int)strlen(input_buffer);
          cursor_pos = input_len;
        }
        suggestion_box_close(&suggestions);
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp);
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);
        if (ch == '\n' || ch == '\r') {
          goto process_enter;
        }
        continue;
      }
    }

    if (ch == KEY_UP) {
      if (input_focused && input_len > 0) {
        int text_width = getmaxx(input_win) - 6;
        if (text_width < 10)
          text_width = 10;
        InputCursorPos cur =
            ui_cursor_to_line_col(input_buffer, cursor_pos, text_width);
        if (cur.line > 0) {
          cursor_pos = ui_line_col_to_cursor(input_buffer, cur.line - 1,
                                             cur.col, text_width);
          if (cur.line - 1 < input_scroll_line)
            input_scroll_line = cur.line - 1;
          ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                  input_focused, input_scroll_line);
          continue;
        }
      }
      if (history.count == 0)
        continue;
      if (input_focused) {
        selected_msg = (int)history.count - 1;
        input_focused = false;
      } else if (selected_msg > 0) {
        selected_msg--;
      }
      ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                   user_disp, bot_disp);
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line);
      continue;
    }

    if (ch == KEY_DOWN) {
      if (input_focused && input_len > 0) {
        int text_width = getmaxx(input_win) - 6;
        if (text_width < 10)
          text_width = 10;
        InputCursorPos cur =
            ui_cursor_to_line_col(input_buffer, cursor_pos, text_width);
        int total_lines = ui_get_input_line_count(input_buffer, text_width);
        if (cur.line < total_lines - 1) {
          cursor_pos = ui_line_col_to_cursor(input_buffer, cur.line + 1,
                                             cur.col, text_width);
          int visible_lines = getmaxy(input_win) - 2;
          if (cur.line + 1 >= input_scroll_line + visible_lines)
            input_scroll_line = cur.line + 2 - visible_lines;
          ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                  input_focused, input_scroll_line);
          continue;
        }
      }
      if (selected_msg == MSG_SELECT_NONE)
        continue;
      if (selected_msg < (int)history.count - 1) {
        selected_msg++;
      } else {
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
      }
      ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                   user_disp, bot_disp);
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line);
      continue;
    }

    if (ch == KEY_LEFT) {
      if (input_focused && cursor_pos > 0) {
        cursor_pos--;
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);
      } else if (!input_focused && selected_msg >= 0) {
        const char *msg = history_get(&history, selected_msg);
        if (msg && (strncmp(msg, "Bot:", 4) == 0)) {
          size_t swipe_count = history_get_swipe_count(&history, selected_msg);
          size_t active = history_get_active_swipe(&history, selected_msg);
          if (swipe_count > 1 && active > 0) {
            history_set_active_swipe(&history, selected_msg, active - 1);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);
          }
        }
      }
      continue;
    }

    if (ch == KEY_RIGHT) {
      if (input_focused && cursor_pos < input_len) {
        cursor_pos++;
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);
      } else if (!input_focused && selected_msg >= 0) {
        const char *msg = history_get(&history, selected_msg);
        bool is_last_bot = (selected_msg == (int)history.count - 1) && msg &&
                           (strncmp(msg, "Bot:", 4) == 0);
        if (is_last_bot) {
          size_t swipe_count = history_get_swipe_count(&history, selected_msg);
          size_t active = history_get_active_swipe(&history, selected_msg);
          if (active + 1 < swipe_count) {
            history_set_active_swipe(&history, selected_msg, active + 1);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);
          } else {
            LLMContext llm_ctx = {.character =
                                      character_loaded ? &character : NULL,
                                  .persona = &persona};
            size_t user_msg_idx = selected_msg > 0 ? selected_msg - 1 : 0;
            const char *user_msg = history_get(&history, user_msg_idx);
            const char *user_input = "";
            if (user_msg && strncmp(user_msg, "You: ", 5) == 0)
              user_input = user_msg + 5;

            history_add_swipe(&history, selected_msg, "Bot: *thinking*");
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);

            ModelConfig *model = config_get_active(&models);
            const char *model_name = get_model_name(&models);
            if (model) {
              StreamContext ctx = {.history = &history,
                                   .chat_win = chat_win,
                                   .input_win = input_win,
                                   .msg_index = (size_t)selected_msg,
                                   .buffer = NULL,
                                   .buf_cap = 0,
                                   .buf_len = 0,
                                   .spinner_frame = 0,
                                   .last_spinner_update = get_time_ms(),
                                   .selected_msg = &selected_msg,
                                   .model_name = model_name,
                                   .user_name = user_disp,
                                   .bot_name = bot_disp};

              ChatHistory hist_for_llm = {.messages = history.messages,
                                          .count = history.count - 1,
                                          .capacity = history.capacity};

              LLMResponse resp =
                  llm_chat(model, &hist_for_llm, &llm_ctx, stream_callback,
                           progress_callback, &ctx);

              if (!resp.success) {
                char err_msg[512];
                snprintf(err_msg, sizeof(err_msg),
                         "Bot: *frowns* \"Error: %s\"", resp.error);
                history_update(&history, selected_msg, err_msg);
              } else if (ctx.buf_len == 0) {
                history_update(&history, selected_msg, "Bot: *stays silent*");
              }

              free(ctx.buffer);
              llm_response_free(&resp);
            }
            selected_msg = MSG_SELECT_NONE;
            input_focused = true;
            touchwin(chat_win);
            touchwin(input_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);
            ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                    input_focused, input_scroll_line);
          }
        } else if (msg && strncmp(msg, "Bot:", 4) == 0) {
          size_t swipe_count = history_get_swipe_count(&history, selected_msg);
          size_t active = history_get_active_swipe(&history, selected_msg);
          if (active + 1 < swipe_count) {
            history_set_active_swipe(&history, selected_msg, active + 1);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);
          }
        }
      }
      continue;
    }

    if (ch == KEY_HOME || ch == 1) {
      cursor_pos = 0;
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line);
      continue;
    }

    if (ch == KEY_END || ch == 5) {
      cursor_pos = input_len;
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line);
      continue;
    }

    if (ch == '\n' || ch == '\r') {
    process_enter:
      suggestion_box_close(&suggestions);

      if (!input_focused) {
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp);
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);
        continue;
      }

      if (input_len == 0 || is_only_whitespace(input_buffer)) {
        input_buffer[0] = '\0';
        input_len = 0;
        cursor_pos = 0;
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);
        continue;
      }

      input_buffer[input_len] = '\0';

      if (strcmp(input_buffer, "/quit") == 0) {
        running = false;
        break;
      }

      if (input_buffer[0] == '/') {
        if (handle_slash_command(input_buffer, &modal, &models, &history,
                                 current_chat_id, current_char_path, &character,
                                 &persona, &character_loaded)) {
          input_buffer[0] = '\0';
          input_len = 0;
          cursor_pos = 0;
          input_scroll_line = 0;
          if (current_input_height != 3) {
            current_input_height = 3;
            ui_layout_windows_with_input(&chat_win, &input_win,
                                         current_input_height);
          }
          user_disp = get_user_display_name(&persona);
          bot_disp = get_bot_display_name(&character, character_loaded);
          if (modal_is_open(&modal)) {
            modal_draw(&modal, &models);
            ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                    input_focused, input_scroll_line);
          } else {
            selected_msg = MSG_SELECT_NONE;
            input_focused = true;
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);
            ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                    input_focused, input_scroll_line);
          }
          continue;
        }
      }

      char user_line[INPUT_MAX + 6];
      snprintf(user_line, sizeof(user_line), "You: %s", input_buffer);

      char saved_input[INPUT_MAX];
      strncpy(saved_input, input_buffer, INPUT_MAX);

      input_buffer[0] = '\0';
      input_len = 0;
      cursor_pos = 0;
      input_scroll_line = 0;
      if (current_input_height != 3) {
        current_input_height = 3;
        ui_layout_windows_with_input(&chat_win, &input_win,
                                     current_input_height);
      }

      if (history_add(&history, user_line) != SIZE_MAX) {
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp);
      }
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line);

      LLMContext llm_ctx = {.character = character_loaded ? &character : NULL,
                            .persona = &persona};
      do_llm_reply(&history, chat_win, input_win, saved_input, &models,
                   &selected_msg, &llm_ctx, user_disp, bot_disp);
      continue;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (!input_focused)
        continue;
      if (cursor_pos > 0) {
        memmove(&input_buffer[cursor_pos - 1], &input_buffer[cursor_pos],
                input_len - cursor_pos + 1);
        input_len--;
        cursor_pos--;

        int new_height = ui_calc_input_height(input_buffer, getmaxx(input_win));
        if (new_height != current_input_height) {
          current_input_height = new_height;
          ui_layout_windows_with_input(&chat_win, &input_win,
                                       current_input_height);
          input_scroll_line = 0;
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp);
        }

        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);

        int input_y = getbegy(input_win);
        int input_x = getbegx(input_win);
        bool needs_chat_redraw = suggestion_box_update(
            &suggestions, input_buffer, input_win, input_y, input_x);
        if (suggestion_box_is_open(&suggestions)) {
          if (needs_chat_redraw) {
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);
          }
          suggestion_box_draw(&suggestions);
        } else {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp);
        }
      }
      continue;
    }

    if (ch == KEY_DC) {
      if (!input_focused)
        continue;
      if (cursor_pos < input_len) {
        memmove(&input_buffer[cursor_pos], &input_buffer[cursor_pos + 1],
                input_len - cursor_pos);
        input_len--;

        int new_height = ui_calc_input_height(input_buffer, getmaxx(input_win));
        if (new_height != current_input_height) {
          current_input_height = new_height;
          ui_layout_windows_with_input(&chat_win, &input_win,
                                       current_input_height);
          input_scroll_line = 0;
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp);
        }

        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line);

        int input_y = getbegy(input_win);
        int input_x = getbegx(input_win);
        bool needs_chat_redraw = suggestion_box_update(
            &suggestions, input_buffer, input_win, input_y, input_x);
        if (suggestion_box_is_open(&suggestions)) {
          if (needs_chat_redraw) {
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp);
          }
          suggestion_box_draw(&suggestions);
        } else {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp);
        }
      }
      continue;
    }

    if (!input_focused)
      continue;

    if (isprint(ch) && input_len < INPUT_MAX - 1) {
      memmove(&input_buffer[cursor_pos + 1], &input_buffer[cursor_pos],
              input_len - cursor_pos + 1);
      input_buffer[cursor_pos] = (char)ch;
      input_len++;
      cursor_pos++;

      int new_height = ui_calc_input_height(input_buffer, getmaxx(input_win));
      if (new_height != current_input_height) {
        current_input_height = new_height;
        ui_layout_windows_with_input(&chat_win, &input_win,
                                     current_input_height);
        int text_width = getmaxx(input_win) - 6;
        InputCursorPos cur =
            ui_cursor_to_line_col(input_buffer, cursor_pos, text_width);
        int visible_lines = getmaxy(input_win) - 2;
        if (cur.line >= input_scroll_line + visible_lines)
          input_scroll_line = cur.line - visible_lines + 1;
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp);
      }

      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line);

      int input_y = getbegy(input_win);
      int input_x = getbegx(input_win);
      bool needs_chat_redraw = suggestion_box_update(
          &suggestions, input_buffer, input_win, input_y, input_x);
      if (suggestion_box_is_open(&suggestions)) {
        if (needs_chat_redraw) {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp);
        }
        suggestion_box_draw(&suggestions);
      } else {
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp);
      }
    }
  }

  suggestion_box_free(&suggestions);
  modal_close(&modal);
  delwin(chat_win);
  delwin(input_win);
  endwin();
  history_free(&history);
  if (character_loaded) {
    character_free(&character);
  }
  llm_cleanup();
  return EXIT_SUCCESS;
}

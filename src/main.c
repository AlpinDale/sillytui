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

#define INPUT_MAX 256

static const char *SPINNER_FRAMES[] = {"thinking", "thinking.", "thinking..",
                                       "thinking..."};
#define SPINNER_FRAME_COUNT 4

typedef struct {
  ChatHistory *history;
  WINDOW *chat_win;
  size_t msg_index;
  char *buffer;
  size_t buf_cap;
  size_t buf_len;
  int spinner_frame;
  long long last_spinner_update;
  int *scroll_offset;
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
  *ctx->scroll_offset = -1;
  ui_draw_chat(ctx->chat_win, ctx->history, *ctx->scroll_offset,
               ctx->model_name, ctx->user_name, ctx->bot_name);
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
  ui_draw_chat(ctx->chat_win, ctx->history, *ctx->scroll_offset,
               ctx->model_name, ctx->user_name, ctx->bot_name);
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
                         const char *user_input, ModelsFile *mf,
                         int *scroll_offset, const LLMContext *llm_ctx,
                         const char *user_name, const char *bot_name) {
  ModelConfig *model = config_get_active(mf);
  const char *model_name = get_model_name(mf);
  if (!model) {
    history_add(history, "Bot: *looks confused* \"No model configured. Use "
                         "/model set to add one.\"");
    *scroll_offset = -1;
    ui_draw_chat(chat_win, history, *scroll_offset, NULL, user_name, bot_name);
    return;
  }

  size_t msg_index = history_add(history, "Bot: *thinking*");
  if (msg_index == SIZE_MAX)
    return;
  *scroll_offset = -1;
  ui_draw_chat(chat_win, history, *scroll_offset, model_name, user_name,
               bot_name);

  StreamContext ctx = {.history = history,
                       .chat_win = chat_win,
                       .msg_index = msg_index,
                       .buffer = NULL,
                       .buf_cap = 0,
                       .buf_len = 0,
                       .spinner_frame = 0,
                       .last_spinner_update = get_time_ms(),
                       .scroll_offset = scroll_offset,
                       .model_name = model_name,
                       .user_name = user_name,
                       .bot_name = bot_name};

  ChatHistory hist_for_llm = {.items = history->items,
                              .count = history->count - 1,
                              .capacity = history->capacity};

  LLMResponse resp = llm_chat(model, &hist_for_llm, llm_ctx, stream_callback,
                              progress_callback, &ctx);

  if (!resp.success) {
    char err_msg[512];
    snprintf(err_msg, sizeof(err_msg), "Bot: *frowns* \"Error: %s\"",
             resp.error);
    history_update(history, msg_index, err_msg);
    *scroll_offset = -1;
    ui_draw_chat(chat_win, history, *scroll_offset, model_name, user_name,
                 bot_name);
  } else if (ctx.buf_len == 0) {
    history_update(history, msg_index, "Bot: *stays silent*");
    *scroll_offset = -1;
    ui_draw_chat(chat_win, history, *scroll_offset, model_name, user_name,
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

  WINDOW *chat_win = NULL;
  WINDOW *input_win = NULL;
  ui_layout_windows(&chat_win, &input_win);

  Modal modal;
  modal_init(&modal);

  SuggestionBox suggestions;
  suggestion_box_init(&suggestions, SLASH_COMMANDS, SLASH_COMMAND_COUNT);

  char input_buffer[INPUT_MAX] = {0};
  int input_len = 0;
  int cursor_pos = 0;
  int scroll_offset = -1;
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
  ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
               user_disp, bot_disp);
  ui_draw_input(input_win, input_buffer, cursor_pos);

  while (running) {
    user_disp = get_user_display_name(&persona);
    bot_disp = get_bot_display_name(&character, character_loaded);
    WINDOW *active_win = modal_is_open(&modal) ? modal.win : input_win;
    int ch = wgetch(active_win);

    if (ch == KEY_RESIZE) {
      ui_layout_windows(&chat_win, &input_win);
      scroll_offset = -1;
      ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
                   user_disp, bot_disp);
      ui_draw_input(input_win, input_buffer, cursor_pos);
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
        scroll_offset = -1;
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
        scroll_offset = -1;
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
          scroll_offset = -1;
        }
      }
      if (modal_is_open(&modal)) {
        modal_draw(&modal, &models);
      } else {
        touchwin(chat_win);
        touchwin(input_win);
        ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
                     user_disp, bot_disp);
        ui_draw_input(input_win, input_buffer, cursor_pos);
      }
      continue;
    }

    if (ch == 27) {
      if (suggestion_box_is_open(&suggestions)) {
        suggestion_box_close(&suggestions);
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
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
          snprintf(input_buffer, INPUT_MAX, "/%s", selected);
          input_len = (int)strlen(input_buffer);
          cursor_pos = input_len;
        }
        suggestion_box_close(&suggestions);
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
                     user_disp, bot_disp);
        ui_draw_input(input_win, input_buffer, cursor_pos);
        if (ch == '\n' || ch == '\r') {
          goto process_enter;
        }
        continue;
      }
    }

    if (ch == KEY_UP) {
      int height = getmaxy(chat_win) - 2;
      int total = ui_get_total_lines(chat_win, &history);
      int max_scroll = total - height;
      if (max_scroll < 0)
        max_scroll = 0;

      if (scroll_offset < 0) {
        scroll_offset = max_scroll;
      }
      scroll_offset -= 5;
      if (scroll_offset < 0)
        scroll_offset = 0;
      ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
                   user_disp, bot_disp);
      continue;
    }

    if (ch == KEY_DOWN) {
      int height = getmaxy(chat_win) - 2;
      int total = ui_get_total_lines(chat_win, &history);
      int max_scroll = total - height;
      if (max_scroll < 0)
        max_scroll = 0;

      if (scroll_offset < 0) {
        continue;
      }
      scroll_offset += 5;
      if (scroll_offset >= max_scroll) {
        scroll_offset = -1;
      }
      ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
                   user_disp, bot_disp);
      continue;
    }

    if (ch == KEY_LEFT) {
      if (cursor_pos > 0) {
        cursor_pos--;
        ui_draw_input(input_win, input_buffer, cursor_pos);
      }
      continue;
    }

    if (ch == KEY_RIGHT) {
      if (cursor_pos < input_len) {
        cursor_pos++;
        ui_draw_input(input_win, input_buffer, cursor_pos);
      }
      continue;
    }

    if (ch == KEY_HOME || ch == 1) {
      cursor_pos = 0;
      ui_draw_input(input_win, input_buffer, cursor_pos);
      continue;
    }

    if (ch == KEY_END || ch == 5) {
      cursor_pos = input_len;
      ui_draw_input(input_win, input_buffer, cursor_pos);
      continue;
    }

    if (ch == '\n' || ch == '\r') {
    process_enter:
      suggestion_box_close(&suggestions);

      if (input_len == 0 || is_only_whitespace(input_buffer)) {
        input_buffer[0] = '\0';
        input_len = 0;
        cursor_pos = 0;
        ui_draw_input(input_win, input_buffer, cursor_pos);
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
          user_disp = get_user_display_name(&persona);
          bot_disp = get_bot_display_name(&character, character_loaded);
          if (modal_is_open(&modal)) {
            modal_draw(&modal, &models);
            ui_draw_input(input_win, input_buffer, cursor_pos);
          } else {
            scroll_offset = -1;
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, scroll_offset,
                         get_model_name(&models), user_disp, bot_disp);
            ui_draw_input(input_win, input_buffer, cursor_pos);
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

      if (history_add(&history, user_line) != SIZE_MAX) {
        scroll_offset = -1;
        ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
                     user_disp, bot_disp);
      }
      ui_draw_input(input_win, input_buffer, cursor_pos);

      LLMContext llm_ctx = {.character = character_loaded ? &character : NULL,
                            .persona = &persona};
      do_llm_reply(&history, chat_win, saved_input, &models, &scroll_offset,
                   &llm_ctx, user_disp, bot_disp);
      continue;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (cursor_pos > 0) {
        memmove(&input_buffer[cursor_pos - 1], &input_buffer[cursor_pos],
                input_len - cursor_pos + 1);
        input_len--;
        cursor_pos--;
        ui_draw_input(input_win, input_buffer, cursor_pos);

        int input_y = getbegy(input_win);
        int input_x = getbegx(input_win);
        suggestion_box_update(&suggestions, input_buffer, input_win, input_y,
                              input_x);
        if (suggestion_box_is_open(&suggestions)) {
          suggestion_box_draw(&suggestions);
        } else {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, scroll_offset,
                       get_model_name(&models), user_disp, bot_disp);
        }
      }
      continue;
    }

    if (ch == KEY_DC) {
      if (cursor_pos < input_len) {
        memmove(&input_buffer[cursor_pos], &input_buffer[cursor_pos + 1],
                input_len - cursor_pos);
        input_len--;
        ui_draw_input(input_win, input_buffer, cursor_pos);

        int input_y = getbegy(input_win);
        int input_x = getbegx(input_win);
        suggestion_box_update(&suggestions, input_buffer, input_win, input_y,
                              input_x);
        if (suggestion_box_is_open(&suggestions)) {
          suggestion_box_draw(&suggestions);
        } else {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, scroll_offset,
                       get_model_name(&models), user_disp, bot_disp);
        }
      }
      continue;
    }

    if (isprint(ch) && input_len < INPUT_MAX - 1) {
      memmove(&input_buffer[cursor_pos + 1], &input_buffer[cursor_pos],
              input_len - cursor_pos + 1);
      input_buffer[cursor_pos] = (char)ch;
      input_len++;
      cursor_pos++;
      ui_draw_input(input_win, input_buffer, cursor_pos);

      int input_y = getbegy(input_win);
      int input_x = getbegx(input_win);
      suggestion_box_update(&suggestions, input_buffer, input_win, input_y,
                            input_x);
      if (suggestion_box_is_open(&suggestions)) {
        suggestion_box_draw(&suggestions);
      } else {
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, scroll_offset, get_model_name(&models),
                     user_disp, bot_disp);
      }
    }
  }

  suggestion_box_close(&suggestions);
  free(suggestions.matched_indices);
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

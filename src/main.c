#include <ctype.h>
#include <curses.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "character/character.h"
#include "character/persona.h"
#include "chat/author_note.h"
#include "chat/chat.h"
#include "chat/history.h"
#include "core/config.h"
#include "core/macros.h"
#include "llm/llm.h"
#include "llm/sampler.h"
#include "lore/lorebook.h"
#include "ui/markdown.h"
#include "ui/modal.h"
#include "ui/ui.h"

#define INPUT_MAX 8192

static const char *SPINNER_FRAMES[] = {"thinking", "thinking.", "thinking..",
                                       "thinking..."};
#define SPINNER_FRAME_COUNT 4

static bool ensure_attachments_dir(void) {
  const char *home = getenv("HOME");
  if (!home)
    return false;
  char dir[512];
  snprintf(dir, sizeof(dir), "%s/.config/sillytui/attachments", home);

  char tmp[512];
  snprintf(tmp, sizeof(tmp), "%s/.config/sillytui", home);
  mkdir(tmp, 0755);
  snprintf(tmp, sizeof(tmp), "%s/.config/sillytui/attachments", home);
  mkdir(tmp, 0755);
  return true;
}

static char *read_all_paste_input(WINDOW *win, int first_ch, size_t *out_len) {
  size_t cap = 4096;
  size_t len = 0;
  char *buf = malloc(cap);
  if (!buf)
    return NULL;

  nodelay(win, TRUE);
  
  if (isprint(first_ch) || first_ch == '\n') {
    if (len >= cap) {
      cap *= 2;
      char *tmp = realloc(buf, cap);
      if (!tmp) {
        free(buf);
        return NULL;
      }
      buf = tmp;
    }
    buf[len++] = (char)first_ch;
  }

  int ch = wgetch(win);
  int consecutive_empty = 0;
  while (consecutive_empty < 5) {
    if (ch != ERR) {
      consecutive_empty = 0;
      if (isprint(ch) || ch == '\n') {
        if (len >= cap) {
          cap *= 2;
          char *tmp = realloc(buf, cap);
          if (!tmp) {
            free(buf);
            return NULL;
          }
          buf = tmp;
        }
        buf[len++] = (char)ch;
      }
      ch = wgetch(win);
    } else {
      consecutive_empty++;
      if (consecutive_empty < 5) {
        usleep(5000);
        ch = wgetch(win);
      }
    }
  }

  flushinp();
  while (wgetch(win) != ERR) { }
  nodelay(win, FALSE);

  if (len == 0) {
    free(buf);
    return NULL;
  }

  buf[len] = '\0';
  *out_len = len;
  return buf;
}

static char *save_attachment(const char *text, size_t text_len) {
  if (!text || text_len == 0)
    return NULL;

  if (!ensure_attachments_dir())
    return NULL;

  const char *home = getenv("HOME");
  if (!home)
    return NULL;

  time_t now = time(NULL);
  char filename[256];
  snprintf(filename, sizeof(filename), "attachment_%ld.txt", (long)now);

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/.config/sillytui/attachments/%s",
           home, filename);

  FILE *f = fopen(filepath, "w");
  if (!f)
    return NULL;

  size_t written = fwrite(text, 1, text_len, f);
  fclose(f);

  if (written != text_len) {
    unlink(filepath);
    return NULL;
  }

  char *ref = malloc(256);
  if (ref) {
    snprintf(ref, 256, "[Attachment: %s]", filename);
  }
  return ref;
}

static char *load_attachment(const char *ref) {
  if (!ref || strncmp(ref, "[Attachment: ", 13) != 0)
    return NULL;

  const char *filename_start = ref + 13;
  const char *filename_end = strchr(filename_start, ']');
  if (!filename_end)
    return NULL;

  size_t filename_len = filename_end - filename_start;
  if (filename_len == 0 || filename_len >= 256)
    return NULL;

  const char *home = getenv("HOME");
  if (!home)
    return NULL;

  char filename[256];
  strncpy(filename, filename_start, filename_len);
  filename[filename_len] = '\0';

  char filepath[768];
  snprintf(filepath, sizeof(filepath), "%s/.config/sillytui/attachments/%s",
           home, filename);

  FILE *f = fopen(filepath, "r");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (len <= 0) {
    fclose(f);
    return NULL;
  }

  char *content = malloc(len + 1);
  if (!content) {
    fclose(f);
    return NULL;
  }

  size_t read_len = fread(content, 1, len, f);
  fclose(f);
  content[read_len] = '\0';

  return content;
}

typedef struct {
  ChatHistory *history;
  WINDOW *chat_win;
  WINDOW *input_win;
  size_t msg_index;
  char *buffer;
  size_t buf_cap;
  size_t buf_len;
  char *reasoning_buffer;
  size_t reasoning_cap;
  size_t reasoning_len;
  bool in_reasoning;
  long long reasoning_start_time;
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
               ctx->user_name, ctx->bot_name, false);
  ui_draw_input_multiline(ctx->input_win, "", 0, true, 0, false);
}

static void progress_callback(void *userdata) {
  StreamContext *ctx = userdata;

  long long now = get_time_ms();
  if (now - ctx->last_spinner_update < 200) {
    return;
  }
  ctx->last_spinner_update = now;

  ctx->spinner_frame = (ctx->spinner_frame + 1) % SPINNER_FRAME_COUNT;

  char display[256];
  if (ctx->in_reasoning) {
    double elapsed_s = (now - ctx->reasoning_start_time) / 1000.0;
    snprintf(display, sizeof(display), "Bot: *💭 thinking... %.1fs*",
             elapsed_s);
  } else {
    snprintf(display, sizeof(display), "Bot: *%s*",
             SPINNER_FRAMES[ctx->spinner_frame]);
  }
  history_update(ctx->history, ctx->msg_index, display);
  ui_draw_chat(ctx->chat_win, ctx->history, *ctx->selected_msg, ctx->model_name,
               ctx->user_name, ctx->bot_name, false);
  ui_draw_input_multiline(ctx->input_win, "", 0, true, 0, false);
}

static void reasoning_callback(const char *chunk, double elapsed_ms,
                               void *userdata) {
  StreamContext *ctx = userdata;

  if (!ctx->in_reasoning) {
    ctx->in_reasoning = true;
    ctx->reasoning_start_time = get_time_ms();
  }

  size_t chunk_len = strlen(chunk);
  if (ctx->reasoning_len + chunk_len + 1 > ctx->reasoning_cap) {
    size_t new_cap = ctx->reasoning_cap == 0 ? 1024 : ctx->reasoning_cap * 2;
    while (new_cap < ctx->reasoning_len + chunk_len + 1)
      new_cap *= 2;
    char *new_buf = realloc(ctx->reasoning_buffer, new_cap);
    if (!new_buf)
      return;
    ctx->reasoning_buffer = new_buf;
    ctx->reasoning_cap = new_cap;
  }

  memcpy(ctx->reasoning_buffer + ctx->reasoning_len, chunk, chunk_len);
  ctx->reasoning_len += chunk_len;
  ctx->reasoning_buffer[ctx->reasoning_len] = '\0';

  char display[256];
  double elapsed_s = elapsed_ms / 1000.0;
  snprintf(display, sizeof(display), "Bot: *💭 thinking... %.1fs*", elapsed_s);
  history_update(ctx->history, ctx->msg_index, display);
  ui_draw_chat(ctx->chat_win, ctx->history, *ctx->selected_msg, ctx->model_name,
               ctx->user_name, ctx->bot_name, false);
  ui_draw_input_multiline(ctx->input_win, "", 0, true, 0, false);
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
  (void)user_input; // Reserved for future use
  ModelConfig *model = config_get_active(mf);
  const char *model_name = get_model_name(mf);
  if (!model) {
    history_add_with_role(history,
                          "Bot: *looks confused* \"No model configured. Use "
                          "/model set to add one.\"",
                          ROLE_ASSISTANT);
    *selected_msg = MSG_SELECT_NONE;
    ui_draw_chat(chat_win, history, *selected_msg, NULL, user_name, bot_name,
                 false);
    return;
  }

  size_t msg_index =
      history_add_with_role(history, "Bot: *thinking*", ROLE_ASSISTANT);
  if (msg_index == SIZE_MAX)
    return;
  *selected_msg = MSG_SELECT_NONE;
  ui_draw_chat(chat_win, history, *selected_msg, model_name, user_name,
               bot_name, false);
  ui_draw_input_multiline(input_win, "", 0, true, 0, false);

  StreamContext ctx = {.history = history,
                       .chat_win = chat_win,
                       .input_win = input_win,
                       .msg_index = msg_index,
                       .buffer = NULL,
                       .buf_cap = 0,
                       .buf_len = 0,
                       .reasoning_buffer = NULL,
                       .reasoning_cap = 0,
                       .reasoning_len = 0,
                       .in_reasoning = false,
                       .reasoning_start_time = 0,
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
                              reasoning_callback, progress_callback, &ctx);

  if (!resp.success) {
    char err_msg[512];
    snprintf(err_msg, sizeof(err_msg), "Bot: *frowns* \"Error: %s\"",
             resp.error);
    history_update(history, msg_index, err_msg);
    *selected_msg = MSG_SELECT_NONE;
    ui_draw_chat(chat_win, history, *selected_msg, model_name, user_name,
                 bot_name, false);
  } else if (ctx.buf_len == 0) {
    history_update(history, msg_index, "Bot: *stays silent*");
  } else {
    size_t active_swipe = history_get_active_swipe(history, msg_index);
    history_set_token_count(history, msg_index, active_swipe,
                            resp.completion_tokens);
    history_set_gen_time(history, msg_index, active_swipe, resp.elapsed_ms);
    history_set_output_tps(history, msg_index, active_swipe, resp.output_tps);
    if (ctx.reasoning_len > 0) {
      history_set_reasoning(history, msg_index, active_swipe,
                            ctx.reasoning_buffer, resp.reasoning_ms);
    }
  }

  *selected_msg = MSG_SELECT_NONE;
  ui_draw_chat(chat_win, history, *selected_msg, model_name, user_name,
               bot_name, false);

  free(ctx.buffer);
  free(ctx.reasoning_buffer);
  llm_response_free(&resp);
}

static const SlashCommand SLASH_COMMANDS[] = {
    {"model set", "Add a new model configuration"},
    {"model list", "Select from saved models"},
    {"sampler", "Configure sampler settings"},
    {"chat save", "Save current chat"},
    {"chat load", "Load a saved chat"},
    {"chat new", "Start a new chat"},
    {"char load", "Load a character card"},
    {"char info", "Show character info"},
    {"char greetings", "Select alternate greeting"},
    {"char unload", "Unload current character"},
    {"persona set", "Edit your persona"},
    {"persona info", "Show your persona"},
    {"sys", "Insert a system message"},
    {"note", "Set/show author's note"},
    {"note-depth", "Set author's note depth"},
    {"note-pos", "Set author's note position"},
    {"note-role", "Set author's note role"},
    {"lore load", "Load a lorebook/world info"},
    {"lore info", "Show loaded lorebook info"},
    {"lore list", "List lorebook entries"},
    {"lore toggle", "Toggle lorebook entry"},
    {"lore clear", "Unload lorebook"},
    {"help", "Show available commands"},
    {"clear", "Clear chat history"},
    {"quit", "Exit the application"},
};
#define SLASH_COMMAND_COUNT (sizeof(SLASH_COMMANDS) / sizeof(SLASH_COMMANDS[0]))

static bool handle_slash_command(const char *input, Modal *modal,
                                 ModelsFile *mf, ChatHistory *history,
                                 char *current_chat_id, char *current_char_path,
                                 CharacterCard *character, Persona *persona,
                                 bool *char_loaded, AuthorNote *author_note,
                                 Lorebook *lorebook) {
  if (strcmp(input, "/model set") == 0) {
    modal_open_model_set(modal);
    return true;
  }
  if (strcmp(input, "/model list") == 0) {
    modal_open_model_list(modal, mf);
    return true;
  }
  if (strcmp(input, "/chat save") == 0) {
    const char *char_name =
        (*char_loaded && character->name[0]) ? character->name : NULL;
    modal_open_chat_save(modal, current_chat_id, current_char_path, char_name);
    return true;
  }
  if (strcmp(input, "/chat load") == 0) {
    modal_open_chat_list(modal);
    return true;
  }
  if (strncmp(input, "/chat load ", 11) == 0) {
    const char *args = input + 11;
    while (*args == ' ')
      args++;
    if (*args) {
      // Parse: <character> [chat_id]
      char char_name[CHAT_CHAR_NAME_MAX] = {0};
      char chat_id[CHAT_ID_MAX] = {0};

      const char *space = strchr(args, ' ');
      if (space) {
        size_t char_len = space - args;
        if (char_len < sizeof(char_name)) {
          strncpy(char_name, args, char_len);
          char_name[char_len] = '\0';
        }
        const char *id_start = space + 1;
        while (*id_start == ' ')
          id_start++;
        if (*id_start) {
          strncpy(chat_id, id_start, CHAT_ID_MAX - 1);
          chat_id[CHAT_ID_MAX - 1] = '\0';
        }
      } else {
        snprintf(char_name, sizeof(char_name), "%.127s", args);
      }

      char loaded_char_path[CHAT_CHAR_PATH_MAX] = {0};
      bool loaded = false;

      if (chat_id[0]) {
        loaded =
            chat_load_with_note(history, author_note, chat_id, char_name,
                                loaded_char_path, sizeof(loaded_char_path));
        if (loaded) {
          snprintf(current_chat_id, CHAT_ID_MAX, "%s", chat_id);
        }
      } else {
        loaded = chat_load_latest(history, char_name, loaded_char_path,
                                  sizeof(loaded_char_path), current_chat_id,
                                  CHAT_ID_MAX);
        if (loaded && author_note)
          author_note_init(author_note);
      }

      if (loaded) {
        if (loaded_char_path[0]) {
          if (*char_loaded) {
            character_free(character);
          }
          if (character_load(character, loaded_char_path)) {
            *char_loaded = true;
            snprintf(current_char_path, CHAT_CHAR_PATH_MAX, "%s",
                     loaded_char_path);
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
        if (chat_id[0]) {
          snprintf(err_msg, sizeof(err_msg), "Chat '%s' not found for %s",
                   chat_id, char_name);
        } else {
          snprintf(err_msg, sizeof(err_msg), "No chats found for '%s'",
                   char_name);
        }
        modal_open_message(modal, err_msg, true);
      }
    }
    return true;
  }
  if (strcmp(input, "/chat new") == 0) {
    history_free(history);
    history_init(history);
    ui_reset_reasoning_state();
    if (*char_loaded && character->first_mes && character->first_mes[0]) {
      char *substituted = macro_substitute(
          character->first_mes, character->name, persona_get_name(persona));
      if (substituted) {
        char first_msg[4096];
        snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
        history_add_with_role(history, first_msg, ROLE_ASSISTANT);
        free(substituted);
      }
    } else {
      history_add_with_role(history, "Bot: *waves* \"New chat started!\"",
                            ROLE_ASSISTANT);
    }
    current_chat_id[0] = '\0';
    const char *char_name =
        (*char_loaded && character->name[0]) ? character->name : NULL;
    chat_auto_save(history, current_chat_id, CHAT_ID_MAX, current_char_path,
                   char_name);
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
          snprintf(current_char_path, CHAT_CHAR_PATH_MAX, "%s", config_path);
          free(config_path);
        } else {
          snprintf(current_char_path, CHAT_CHAR_PATH_MAX, "%.511s", path);
        }
        history_free(history);
        history_init(history);
        ui_reset_reasoning_state();
        current_chat_id[0] = '\0';
        if (character->first_mes && character->first_mes[0]) {
          char *substituted = macro_substitute(
              character->first_mes, character->name, persona_get_name(persona));
          if (substituted) {
            char first_msg[4096];
            snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
            history_add_with_role(history, first_msg, ROLE_ASSISTANT);
            free(substituted);
          }
        } else {
          char welcome[256];
          snprintf(welcome, sizeof(welcome), "Bot: *%s appears* \"Hello!\"",
                   character->name);
          history_add_with_role(history, welcome, ROLE_ASSISTANT);
        }
        chat_auto_save(history, current_chat_id, CHAT_ID_MAX, current_char_path,
                       character->name);
      } else {
        char err_msg[1024];
        snprintf(
            err_msg, sizeof(err_msg),
            "Failed to load character card:\n%.512s\n\nMake sure the file "
            "exists "
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
      history_add_with_role(history, "Bot: *Character unloaded*",
                            ROLE_ASSISTANT);
    } else {
      modal_open_message(modal, "No character loaded", true);
    }
    return true;
  }
  if (strcmp(input, "/persona set") == 0) {
    modal_open_persona_edit(modal, persona);
    return true;
  }
  if (strcmp(input, "/sampler") == 0) {
    ModelConfig *model = config_get_active(mf);
    if (model) {
      modal_open_sampler_settings(modal, model->api_type);
    } else {
      modal_open_message(modal, "No model configured. Use /model set first.",
                         false);
    }
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
                       "/model set         - Add a new model\n"
                       "/model list        - Select a model\n"
                       "/sampler           - Configure samplers\n"
                       "/chat save         - Save current chat\n"
                       "/chat load         - Load a saved chat\n"
                       "/chat new          - Start new chat\n"
                       "/char load         - Load character\n"
                       "/char info         - Character info\n"
                       "/char greetings    - Select greeting\n"
                       "/persona set       - Edit persona\n"
                       "/sys <msg>         - Insert system message\n"
                       "/note <text>       - Set author's note\n"
                       "/note-depth <n>    - Set note depth\n"
                       "/lore load <file>  - Load lorebook\n"
                       "/lore info         - Lorebook info\n"
                       "/lore list         - List entries\n"
                       "/lore toggle <id>  - Toggle entry\n"
                       "/clear             - Clear chat history\n"
                       "/quit              - Exit\n"
                       "\n"
                       "↑/↓ navigate, m move, e edit, d delete",
                       false);
    return true;
  }
  if (strcmp(input, "/clear") == 0) {
    history_free(history);
    history_init(history);
    ui_reset_reasoning_state();
    if (*char_loaded && character->first_mes && character->first_mes[0]) {
      char *substituted = macro_substitute(
          character->first_mes, character->name, persona_get_name(persona));
      if (substituted) {
        char first_msg[4096];
        snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
        history_add_with_role(history, first_msg, ROLE_ASSISTANT);
        free(substituted);
      }
    } else {
      history_add_with_role(history, "Bot: *clears throat* \"Fresh start!\"",
                            ROLE_ASSISTANT);
    }
    const char *char_name =
        (*char_loaded && character->name[0]) ? character->name : NULL;
    chat_auto_save(history, current_chat_id, CHAT_ID_MAX, current_char_path,
                   char_name);
    return true;
  }
  if (strncmp(input, "/sys ", 5) == 0) {
    const char *sys_msg = input + 5;
    while (*sys_msg == ' ')
      sys_msg++;
    if (*sys_msg) {
      history_add_with_role(history, sys_msg, ROLE_SYSTEM);
      const char *char_name =
          (*char_loaded && character->name[0]) ? character->name : NULL;
      chat_auto_save(history, current_chat_id, CHAT_ID_MAX, current_char_path,
                     char_name);
    }
    return true;
  }
  if (strncmp(input, "/note ", 6) == 0) {
    const char *note_text = input + 6;
    while (*note_text == ' ')
      note_text++;
    author_note_set_text(author_note, note_text);
    char msg[256];
    if (note_text[0]) {
      snprintf(msg, sizeof(msg), "Author's Note set:\n\n\"%s\"", note_text);
    } else {
      snprintf(msg, sizeof(msg), "Author's Note cleared.");
    }
    modal_open_message(modal, msg, false);
    return true;
  }
  if (strcmp(input, "/note") == 0) {
    if (author_note->text[0]) {
      char msg[4200];
      snprintf(msg, sizeof(msg),
               "Author's Note:\n\n\"%s\"\n\n"
               "Depth: %d  |  Position: %s  |  Role: %s",
               author_note->text, author_note->depth,
               author_note_position_to_string(author_note->position),
               author_note_role_to_string(author_note->role));
      modal_open_message(modal, msg, false);
    } else {
      modal_open_message(
          modal, "No Author's Note set.\n\nUse /note <text> to set one.",
          false);
    }
    return true;
  }
  if (strncmp(input, "/note-depth ", 12) == 0) {
    int depth = atoi(input + 12);
    author_note_set_depth(author_note, depth);
    char msg[64];
    snprintf(msg, sizeof(msg), "Author's Note depth set to %d",
             author_note->depth);
    modal_open_message(modal, msg, false);
    return true;
  }
  if (strncmp(input, "/note-pos ", 10) == 0) {
    const char *pos_str = input + 10;
    while (*pos_str == ' ')
      pos_str++;
    AuthorNotePosition pos = author_note_position_from_string(pos_str);
    author_note_set_position(author_note, pos);
    char msg[64];
    snprintf(msg, sizeof(msg), "Author's Note position set to %s",
             author_note_position_to_string(author_note->position));
    modal_open_message(modal, msg, false);
    return true;
  }
  if (strncmp(input, "/note-role ", 11) == 0) {
    const char *role_str = input + 11;
    while (*role_str == ' ')
      role_str++;
    AuthorNoteRole role = author_note_role_from_string(role_str);
    author_note_set_role(author_note, role);
    char msg[64];
    snprintf(msg, sizeof(msg), "Author's Note role set to %s",
             author_note_role_to_string(author_note->role));
    modal_open_message(modal, msg, false);
    return true;
  }
  if (strncmp(input, "/lore load ", 11) == 0) {
    const char *path = input + 11;
    while (*path == ' ')
      path++;
    if (*path) {
      lorebook_free(lorebook);
      lorebook_init(lorebook);
      if (lorebook_load_json(lorebook, path)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Loaded lorebook: %s (%zu entries)",
                 lorebook->name, lorebook->entry_count);
        modal_open_message(modal, msg, false);
      } else {
        modal_open_message(modal, "Failed to load lorebook", false);
      }
      return true;
    }
  }
  if (strcmp(input, "/lore info") == 0) {
    if (lorebook->entry_count == 0) {
      modal_open_message(modal, "No lorebook loaded. Use /lore load <path>",
                         false);
    } else {
      char msg[512];
      snprintf(msg, sizeof(msg),
               "Lorebook: %s\n"
               "Description: %s\n"
               "Entries: %zu\n"
               "Scan Depth: %d\n"
               "Recursive: %s",
               lorebook->name, lorebook->description, lorebook->entry_count,
               lorebook->default_scan_depth,
               lorebook->recursive_scanning ? "Yes" : "No");
      modal_open_message(modal, msg, false);
    }
    return true;
  }
  if (strcmp(input, "/lore list") == 0) {
    if (lorebook->entry_count == 0) {
      modal_open_message(modal, "No lorebook loaded", false);
    } else {
      size_t msg_size = 4096;
      char *msg = malloc(msg_size);
      if (msg) {
        size_t pos = 0;
        pos += snprintf(msg + pos, msg_size - pos, "Lorebook entries:\n\n");
        for (size_t i = 0; i < lorebook->entry_count && pos < msg_size - 128;
             i++) {
          const LoreEntry *e = &lorebook->entries[i];
          pos += snprintf(msg + pos, msg_size - pos, "%d. %s%s [", e->uid,
                          e->comment, e->disabled ? " (OFF)" : "");
          for (size_t j = 0; j < e->key_count && j < 3; j++) {
            pos += snprintf(msg + pos, msg_size - pos, "%s%s", e->keys[j],
                            j + 1 < e->key_count && j + 1 < 3 ? ", " : "");
          }
          if (e->key_count > 3)
            pos += snprintf(msg + pos, msg_size - pos, "...");
          pos += snprintf(msg + pos, msg_size - pos, "]\n");
        }
        modal_open_message(modal, msg, false);
        free(msg);
      }
    }
    return true;
  }
  if (strncmp(input, "/lore toggle ", 13) == 0) {
    int uid = atoi(input + 13);
    if (uid > 0 && lorebook_toggle_entry(lorebook, uid)) {
      LoreEntry *e = lorebook_get_entry(lorebook, uid);
      char msg[256];
      snprintf(msg, sizeof(msg), "Entry %d (%.64s) is now %s", uid,
               e ? e->comment : "?", e && e->disabled ? "disabled" : "enabled");
      modal_open_message(modal, msg, false);
    } else {
      modal_open_message(modal, "Entry not found", false);
    }
    return true;
  }
  if (strcmp(input, "/lore clear") == 0) {
    lorebook_free(lorebook);
    lorebook_init(lorebook);
    modal_open_message(modal, "Lorebook cleared", false);
    return true;
  }
  return false;
}

int main(void) {
  ChatHistory history;
  history_init(&history);
  ui_reset_reasoning_state();

  ModelsFile models;
  config_load_models(&models);

  AppSettings settings;
  config_load_settings(&settings);

  Persona persona;
  persona_load(&persona);

  SamplerSettings current_samplers;
  sampler_init_defaults(&current_samplers);

  CharacterCard character;
  memset(&character, 0, sizeof(character));
  bool character_loaded = false;

  Lorebook lorebook;
  lorebook_init(&lorebook);

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
  bool move_mode = false;
  int last_input_len = 0;
  long long last_input_time = 0;
  bool running = true;
  char current_chat_id[CHAT_ID_MAX] = {0};
  char current_char_path[CHAT_CHAR_PATH_MAX] = {0};

  AuthorNote author_note;
  author_note_init(&author_note);

  InPlaceEdit in_place_edit = {0};
  in_place_edit.buf_cap = INPUT_MAX;
  in_place_edit.buffer = malloc(in_place_edit.buf_cap);
  if (in_place_edit.buffer)
    in_place_edit.buffer[0] = '\0';

  if (models.count == 0) {
    history_add_with_role(
        &history,
        "Bot: *Welcome to SillyTUI!*\n\n"
        "To get started:\n"
        "  • /model set - Configure an LLM (API URL, key, model)\n"
        "  • /help - View all available commands\n\n"
        "Once configured, load a character card with /char load <path>\n"
        "or simply type a message here!",
        ROLE_ASSISTANT);
  } else {
    history_add_with_role(
        &history,
        "Bot: *Welcome back!*\n\n"
        "Quick tips:\n"
        "  • /char load <path> - Load a character card\n"
        "  • /chat load - Resume a saved conversation\n"
        "  • /help - View all commands\n"
        "  • ↑↓ arrows - Navigate messages, ←→ on bot msgs to swipe\n\n"
        "Type a message to begin!",
        ROLE_ASSISTANT);
  }

  const char *user_disp = get_user_display_name(&persona);
  const char *bot_disp = get_bot_display_name(&character, character_loaded);
  ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
               user_disp, bot_disp, false);
  ui_draw_input_multiline(input_win, input_buffer, cursor_pos, input_focused,
                          input_scroll_line, false);

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
                   user_disp, bot_disp, false);
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line, false);
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
          ui_reset_reasoning_state();
          current_chat_id[0] = '\0';
          char *substituted = macro_substitute(greeting, character.name,
                                               persona_get_name(&persona));
          if (substituted) {
            char first_msg[4096];
            snprintf(first_msg, sizeof(first_msg), "Bot: %s", substituted);
            history_add_with_role(&history, first_msg, ROLE_ASSISTANT);
            free(substituted);
          }
          selected_msg = MSG_SELECT_NONE;
          input_focused = true;

          chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                   sizeof(current_chat_id), current_char_path,
                                   character.name);
        }
      }
      if (result == MODAL_RESULT_MESSAGE_EDITED) {
        int msg_idx = modal_get_edit_msg_index(&modal);
        const char *new_content = modal_get_edit_content(&modal);
        if (msg_idx >= 0 && msg_idx < (int)history.count && new_content) {
          const char *old_msg = history_get(&history, msg_idx);
          char new_msg[4096];
          if (old_msg && strncmp(old_msg, "You: ", 5) == 0) {
            snprintf(new_msg, sizeof(new_msg), "You: %s", new_content);
          } else {
            snprintf(new_msg, sizeof(new_msg), "Bot: %s", new_content);
          }
          history_update(&history, msg_idx, new_msg);

          const char *char_name =
              (character_loaded && character.name[0]) ? character.name : NULL;
          chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                   sizeof(current_chat_id), current_char_path,
                                   char_name);
        }
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
      }
      if (result == MODAL_RESULT_MESSAGE_DELETED) {
        int msg_idx = modal_get_edit_msg_index(&modal);
        if (msg_idx >= 0 && msg_idx < (int)history.count) {
          history_delete(&history, msg_idx);

          const char *char_name =
              (character_loaded && character.name[0]) ? character.name : NULL;
          chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                   sizeof(current_chat_id), current_char_path,
                                   char_name);
        }
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
      }
      if (modal_is_open(&modal)) {
        modal_draw(&modal, &models);
      } else {
        touchwin(chat_win);
        touchwin(input_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, false);
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);
      }
      continue;
    }

    if (ch == 27) {
      nodelay(input_win, TRUE);
      int next_ch = wgetch(input_win);
      nodelay(input_win, FALSE);

      if (next_ch == '\n' || next_ch == '\r' || next_ch == KEY_ENTER) {
        if (input_focused && input_len < INPUT_MAX - 1) {
          memmove(&input_buffer[cursor_pos + 1], &input_buffer[cursor_pos],
                  input_len - cursor_pos + 1);
          input_buffer[cursor_pos] = '\n';
          input_len++;
          cursor_pos++;
          int new_height =
              ui_calc_input_height(input_buffer, getmaxx(input_win));
          if (new_height != current_input_height) {
            current_input_height = new_height;
            ui_layout_windows_with_input(&chat_win, &input_win,
                                         current_input_height);
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);
          }
          ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                  input_focused, input_scroll_line, false);
        }
        continue;
      }

      if (next_ch != ERR) {
        ungetch(next_ch);
      }

      if (in_place_edit.active) {
        in_place_edit.active = false;
        ui_draw_chat_ex(chat_win, &history, selected_msg,
                        get_model_name(&models), user_disp, bot_disp, true,
                        move_mode, NULL);
        continue;
      }
      if (move_mode) {
        move_mode = false;
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, !input_focused);
        continue;
      }
      if (suggestion_box_is_open(&suggestions)) {
        suggestion_box_close(&suggestions);
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, false);
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
        const char *selected_id = suggestion_box_get_selected_id(&suggestions);
        if (selected) {
          if (suggestions.showing_dynamic) {
            if (suggestions.showing_characters) {
              if (ch == '\t') {
                snprintf(input_buffer, INPUT_MAX, "/chat load %s ",
                         selected_id ? selected_id : selected);
                input_len = (int)strlen(input_buffer);
                cursor_pos = input_len;
                suggestion_box_close(&suggestions);
                touchwin(chat_win);
                ui_draw_chat(chat_win, &history, selected_msg,
                             get_model_name(&models), user_disp, bot_disp,
                             false);
                ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                        input_focused, input_scroll_line,
                                        false);
                int input_y = getbegy(input_win);
                int input_x = getbegx(input_win);
                suggestion_box_update(&suggestions, input_buffer, input_win,
                                      input_y, input_x);
                if (suggestion_box_is_open(&suggestions)) {
                  suggestion_box_draw(&suggestions);
                }
                continue;
              } else {
                char loaded_char_path[CHAT_CHAR_PATH_MAX] = {0};
                const char *char_name = selected_id ? selected_id : selected;
                author_note_init(&author_note);
                if (chat_load_latest(&history, char_name, loaded_char_path,
                                     sizeof(loaded_char_path), current_chat_id,
                                     sizeof(current_chat_id))) {
                  if (loaded_char_path[0]) {
                    if (character_loaded) {
                      character_free(&character);
                    }
                    if (character_load(&character, loaded_char_path)) {
                      character_loaded = true;
                      snprintf(current_char_path, CHAT_CHAR_PATH_MAX, "%s",
                               loaded_char_path);
                    } else {
                      character_loaded = false;
                      current_char_path[0] = '\0';
                    }
                  }
                  input_buffer[0] = '\0';
                  input_len = 0;
                  cursor_pos = 0;
                  selected_msg = MSG_SELECT_NONE;
                  input_focused = true;
                } else {
                  snprintf(input_buffer, INPUT_MAX, "No chats found for %s",
                           selected);
                  input_len = 0;
                  cursor_pos = 0;
                }
                suggestion_box_close(&suggestions);
                touchwin(chat_win);
                user_disp = get_user_display_name(&persona);
                bot_disp = get_bot_display_name(&character, character_loaded);
                ui_draw_chat(chat_win, &history, selected_msg,
                             get_model_name(&models), user_disp, bot_disp,
                             false);
                ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                        input_focused, input_scroll_line,
                                        false);
                continue;
              }
            } else {
              snprintf(input_buffer, INPUT_MAX, "/chat load %s %s",
                       suggestions.current_character,
                       selected_id ? selected_id : selected);
            }
          } else {
            snprintf(input_buffer, INPUT_MAX, "/%s", selected);
          }
          input_len = (int)strlen(input_buffer);
          cursor_pos = input_len;
        }
        suggestion_box_close(&suggestions);
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, false);
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);
        if (ch == '\n' || ch == '\r') {
          goto process_enter;
        }
        continue;
      }
    }

    if (ch == KEY_UP) {
      if (in_place_edit.active) {
        int text_width = getmaxx(chat_win) - 10;
        if (text_width < 10)
          text_width = 10;
        int line = 0, col = 0;
        for (int i = 0; i < in_place_edit.cursor_pos; i++) {
          if (in_place_edit.buffer[i] == '\n') {
            line++;
            col = 0;
          } else {
            col++;
            if (col >= text_width) {
              line++;
              col = 0;
            }
          }
        }
        if (line > 0) {
          int target_line = line - 1;
          int cur_line = 0, cur_col = 0;
          for (int i = 0; i <= in_place_edit.buf_len; i++) {
            if (cur_line == target_line && cur_col == col) {
              in_place_edit.cursor_pos = i;
              break;
            }
            if (cur_line > target_line) {
              in_place_edit.cursor_pos = i > 0 ? i - 1 : 0;
              break;
            }
            if (i < in_place_edit.buf_len) {
              if (in_place_edit.buffer[i] == '\n') {
                if (cur_line == target_line) {
                  in_place_edit.cursor_pos = i;
                  break;
                }
                cur_line++;
                cur_col = 0;
              } else {
                cur_col++;
                if (cur_col >= text_width) {
                  if (cur_line == target_line) {
                    in_place_edit.cursor_pos = i + 1;
                    break;
                  }
                  cur_line++;
                  cur_col = 0;
                }
              }
            }
          }
          ui_draw_chat_ex(chat_win, &history, selected_msg,
                          get_model_name(&models), user_disp, bot_disp, false,
                          move_mode, &in_place_edit);
        }
        continue;
      }
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
                                  input_focused, input_scroll_line, false);
          continue;
        }
      }
      if (history.count == 0)
        continue;
      if (move_mode && selected_msg >= 0) {
        if (history_move_up(&history, selected_msg)) {
          selected_msg--;
          const char *char_name =
              (character_loaded && character.name[0]) ? character.name : NULL;
          chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                   CHAT_ID_MAX, current_char_path, char_name);
        }
      } else if (input_focused) {
        selected_msg = (int)history.count - 1;
        input_focused = false;
      } else if (selected_msg > 0) {
        selected_msg--;
      }
      ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                   user_disp, bot_disp, !input_focused);
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line, false);
      continue;
    }

    if (ch == KEY_DOWN) {
      if (in_place_edit.active) {
        int text_width = getmaxx(chat_win) - 10;
        if (text_width < 10)
          text_width = 10;
        int line = 0, col = 0;
        for (int i = 0; i < in_place_edit.cursor_pos; i++) {
          if (in_place_edit.buffer[i] == '\n') {
            line++;
            col = 0;
          } else {
            col++;
            if (col >= text_width) {
              line++;
              col = 0;
            }
          }
        }
        int target_line = line + 1;
        int cur_line = 0, cur_col = 0;
        for (int i = 0; i <= in_place_edit.buf_len; i++) {
          if (cur_line == target_line && cur_col == col) {
            in_place_edit.cursor_pos = i;
            break;
          }
          if (cur_line > target_line) {
            in_place_edit.cursor_pos = i > 0 ? i - 1 : 0;
            break;
          }
          if (i < in_place_edit.buf_len) {
            if (in_place_edit.buffer[i] == '\n') {
              if (cur_line == target_line) {
                in_place_edit.cursor_pos = i;
                break;
              }
              cur_line++;
              cur_col = 0;
            } else {
              cur_col++;
              if (cur_col >= text_width) {
                if (cur_line == target_line) {
                  in_place_edit.cursor_pos = i + 1;
                  break;
                }
                cur_line++;
                cur_col = 0;
              }
            }
          }
        }
        ui_draw_chat_ex(chat_win, &history, selected_msg,
                        get_model_name(&models), user_disp, bot_disp, false,
                        move_mode, &in_place_edit);
        continue;
      }
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
                                  input_focused, input_scroll_line, false);
          continue;
        }
      }
      if (selected_msg == MSG_SELECT_NONE)
        continue;
      if (move_mode && selected_msg >= 0) {
        if (history_move_down(&history, selected_msg)) {
          selected_msg++;
          const char *char_name =
              (character_loaded && character.name[0]) ? character.name : NULL;
          chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                   CHAT_ID_MAX, current_char_path, char_name);
        }
      } else if (selected_msg < (int)history.count - 1) {
        selected_msg++;
      } else {
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
      }
      ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                   user_disp, bot_disp, !input_focused);
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line, false);
      continue;
    }

    if (ch == KEY_LEFT) {
      if (in_place_edit.active) {
        if (in_place_edit.cursor_pos > 0) {
          in_place_edit.cursor_pos--;
          ui_draw_chat_ex(chat_win, &history, selected_msg,
                          get_model_name(&models), user_disp, bot_disp, false,
                          move_mode, &in_place_edit);
        }
        continue;
      }
      if (input_focused && cursor_pos > 0) {
        cursor_pos--;
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);
      } else if (!input_focused && selected_msg >= 0) {
        const char *msg = history_get(&history, selected_msg);
        if (msg && (strncmp(msg, "Bot:", 4) == 0)) {
          size_t swipe_count = history_get_swipe_count(&history, selected_msg);
          size_t active = history_get_active_swipe(&history, selected_msg);
          if (swipe_count > 1 && active > 0) {
            history_set_active_swipe(&history, selected_msg, active - 1);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);

            const char *char_name =
                (character_loaded && character.name[0]) ? character.name : NULL;
            chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                     sizeof(current_chat_id), current_char_path,
                                     char_name);
          }
        }
      }
      continue;
    }

    if (ch == KEY_RIGHT) {
      if (in_place_edit.active) {
        if (in_place_edit.cursor_pos < in_place_edit.buf_len) {
          in_place_edit.cursor_pos++;
          ui_draw_chat_ex(chat_win, &history, selected_msg,
                          get_model_name(&models), user_disp, bot_disp, false,
                          move_mode, &in_place_edit);
        }
        continue;
      }
      if (input_focused && cursor_pos < input_len) {
        cursor_pos++;
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);
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
                         get_model_name(&models), user_disp, bot_disp, false);
          } else {
            ModelConfig *regen_model = config_get_active(&models);
            if (regen_model)
              sampler_load(&current_samplers, regen_model->api_type);
            LLMContext llm_ctx = {.character =
                                      character_loaded ? &character : NULL,
                                  .persona = &persona,
                                  .samplers = &current_samplers,
                                  .author_note = &author_note,
                                  .lorebook = &lorebook};

            history_add_swipe(&history, selected_msg, "Bot: *thinking*");
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);

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
                                   .reasoning_buffer = NULL,
                                   .reasoning_cap = 0,
                                   .reasoning_len = 0,
                                   .in_reasoning = false,
                                   .reasoning_start_time = 0,
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
                           reasoning_callback, progress_callback, &ctx);

              if (!resp.success) {
                char err_msg[512];
                snprintf(err_msg, sizeof(err_msg),
                         "Bot: *frowns* \"Error: %s\"", resp.error);
                history_update(&history, selected_msg, err_msg);
              } else if (ctx.buf_len == 0) {
                history_update(&history, selected_msg, "Bot: *stays silent*");
              } else {
                size_t active_swipe =
                    history_get_active_swipe(&history, selected_msg);
                history_set_token_count(&history, selected_msg, active_swipe,
                                        resp.completion_tokens);
                history_set_gen_time(&history, selected_msg, active_swipe,
                                     resp.elapsed_ms);
                history_set_output_tps(&history, selected_msg, active_swipe,
                                       resp.output_tps);
                if (ctx.reasoning_len > 0) {
                  history_set_reasoning(&history, selected_msg, active_swipe,
                                        ctx.reasoning_buffer,
                                        resp.reasoning_ms);
                }
              }

              free(ctx.buffer);
              free(ctx.reasoning_buffer);
              llm_response_free(&resp);
            }
            selected_msg = MSG_SELECT_NONE;
            input_focused = true;
            touchwin(chat_win);
            touchwin(input_win);
            ui_draw_chat(chat_win, &history, MSG_SELECT_NONE,
                         get_model_name(&models), user_disp, bot_disp, false);
            ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                    input_focused, input_scroll_line, false);

            const char *char_name =
                (character_loaded && character.name[0]) ? character.name : NULL;
            chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                     sizeof(current_chat_id), current_char_path,
                                     char_name);
          }
        } else if (msg && strncmp(msg, "Bot:", 4) == 0) {
          size_t swipe_count = history_get_swipe_count(&history, selected_msg);
          size_t active = history_get_active_swipe(&history, selected_msg);
          if (active + 1 < swipe_count) {
            history_set_active_swipe(&history, selected_msg, active + 1);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);

            const char *char_name =
                (character_loaded && character.name[0]) ? character.name : NULL;
            chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                     sizeof(current_chat_id), current_char_path,
                                     char_name);
          }
        }
      }
      continue;
    }

    if (ch == KEY_HOME || ch == 1) {
      cursor_pos = 0;
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line, false);
      continue;
    }

    if (ch == KEY_END || ch == 5) {
      cursor_pos = input_len;
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line, false);
      continue;
    }

    if (ch == 'e' && !input_focused && selected_msg >= 0 &&
        selected_msg < (int)history.count && !in_place_edit.active) {
      size_t current_swipe = history_get_active_swipe(&history, selected_msg);
      const char *msg =
          history_get_swipe(&history, selected_msg, current_swipe);
      if (msg && in_place_edit.buffer) {
        const char *content = msg;
        if (strncmp(msg, "You: ", 5) == 0) {
          content = msg + 5;
        } else if (strncmp(msg, "Bot: ", 5) == 0) {
          content = msg + 5;
        } else if (strncmp(msg, "Bot:", 4) == 0) {
          content = msg + 4;
          while (*content == ' ')
            content++;
        }

        strncpy(in_place_edit.buffer, content, in_place_edit.buf_cap - 1);
        in_place_edit.buffer[in_place_edit.buf_cap - 1] = '\0';
        in_place_edit.buf_len = (int)strlen(in_place_edit.buffer);
        in_place_edit.cursor_pos = in_place_edit.buf_len;
        in_place_edit.msg_index = selected_msg;
        in_place_edit.swipe_index = (int)current_swipe;
        in_place_edit.active = true;
        in_place_edit.scroll_offset = 0;

        ui_draw_chat_ex(chat_win, &history, selected_msg,
                        get_model_name(&models), user_disp, bot_disp, false,
                        move_mode, &in_place_edit);
      }
      continue;
    }

    if (in_place_edit.active) {
      if (ch == '\n' || ch == '\r') {
        const char *old_msg = history_get_swipe(
            &history, in_place_edit.msg_index, in_place_edit.swipe_index);
        char new_msg[INPUT_MAX + 6];
        if (old_msg && strncmp(old_msg, "You: ", 5) == 0) {
          snprintf(new_msg, sizeof(new_msg), "You: %s", in_place_edit.buffer);
        } else {
          snprintf(new_msg, sizeof(new_msg), "Bot: %s", in_place_edit.buffer);
        }
        history_update_swipe(&history, in_place_edit.msg_index,
                             in_place_edit.swipe_index, new_msg);
        in_place_edit.active = false;
        ui_draw_chat_ex(chat_win, &history, selected_msg,
                        get_model_name(&models), user_disp, bot_disp, true,
                        move_mode, NULL);

        const char *char_name =
            (character_loaded && character.name[0]) ? character.name : NULL;
        chat_auto_save_with_note(&history, &author_note, current_chat_id,
                                 sizeof(current_chat_id), current_char_path,
                                 char_name);
        continue;
      }
      if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (in_place_edit.cursor_pos > 0) {
          memmove(&in_place_edit.buffer[in_place_edit.cursor_pos - 1],
                  &in_place_edit.buffer[in_place_edit.cursor_pos],
                  in_place_edit.buf_len - in_place_edit.cursor_pos + 1);
          in_place_edit.cursor_pos--;
          in_place_edit.buf_len--;
          ui_draw_chat_ex(chat_win, &history, selected_msg,
                          get_model_name(&models), user_disp, bot_disp, false,
                          move_mode, &in_place_edit);
        }
        continue;
      }
      if (ch == KEY_DC) {
        if (in_place_edit.cursor_pos < in_place_edit.buf_len) {
          memmove(&in_place_edit.buffer[in_place_edit.cursor_pos],
                  &in_place_edit.buffer[in_place_edit.cursor_pos + 1],
                  in_place_edit.buf_len - in_place_edit.cursor_pos);
          in_place_edit.buf_len--;
          ui_draw_chat_ex(chat_win, &history, selected_msg,
                          get_model_name(&models), user_disp, bot_disp, false,
                          move_mode, &in_place_edit);
        }
        continue;
      }
      if (ch >= 32 && ch < 127) {
        if (in_place_edit.buf_len < in_place_edit.buf_cap - 1) {
          memmove(&in_place_edit.buffer[in_place_edit.cursor_pos + 1],
                  &in_place_edit.buffer[in_place_edit.cursor_pos],
                  in_place_edit.buf_len - in_place_edit.cursor_pos + 1);
          in_place_edit.buffer[in_place_edit.cursor_pos] = (char)ch;
          in_place_edit.cursor_pos++;
          in_place_edit.buf_len++;
          ui_draw_chat_ex(chat_win, &history, selected_msg,
                          get_model_name(&models), user_disp, bot_disp, false,
                          move_mode, &in_place_edit);
        }
        continue;
      }
      continue;
    }

    if (ch == 'd' && !input_focused && selected_msg >= 0 &&
        selected_msg < (int)history.count && !in_place_edit.active &&
        !move_mode) {
      modal_open_message_delete(&modal, selected_msg);
      modal_draw(&modal, &models);
      continue;
    }

    if (ch == 'm' && !input_focused && selected_msg >= 0 &&
        selected_msg < (int)history.count && !in_place_edit.active) {
      move_mode = !move_mode;
      ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                   user_disp, bot_disp, !input_focused);
      continue;
    }

    if (ch == 't' && !input_focused && selected_msg >= 0 &&
        selected_msg < (int)history.count) {
      size_t active_swipe = history_get_active_swipe(&history, selected_msg);
      const char *reasoning =
          history_get_reasoning(&history, selected_msg, active_swipe);
      if (reasoning && reasoning[0]) {
        ui_toggle_reasoning((size_t)selected_msg);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, !input_focused);
      }
      continue;
    }

    if (ch == '\n' || ch == '\r') {
      if (input_focused) {
        nodelay(input_win, TRUE);
        int peek_ch = wgetch(input_win);
        bool has_more_input = (peek_ch != ERR);
        
        if (has_more_input) {
          if (settings.paste_attachment_threshold > 0) {
            size_t paste_len = 0;
            char *paste_buf = read_all_paste_input(input_win, '\n', &paste_len);
            
            if (paste_buf) {
              if (paste_len >= (size_t)settings.paste_attachment_threshold &&
                  strncmp(input_buffer, "[Attachment: ", 13) != 0) {
                size_t total_len = input_len + paste_len;
                size_t combined_cap = input_len + paste_len + 1;
                char *combined = malloc(combined_cap);
                if (combined) {
                  memcpy(combined, input_buffer, input_len);
                  memcpy(combined + input_len, paste_buf, paste_len);
                  combined[total_len] = '\0';
                  
                  char *attachment_ref = save_attachment(combined, total_len);
                  if (attachment_ref) {
                    strncpy(input_buffer, attachment_ref, INPUT_MAX - 1);
                    input_buffer[INPUT_MAX - 1] = '\0';
                    input_len = (int)strlen(input_buffer);
                    cursor_pos = input_len;
                    free(attachment_ref);
                  }
                  free(combined);
                }
              } else {
                int read_limit = INPUT_MAX - 2;
                if (input_len < read_limit) {
                  input_buffer[input_len++] = '\n';
                }
                
                size_t copy_len = paste_len;
                if (input_len + copy_len > (size_t)read_limit) {
                  copy_len = read_limit - input_len;
                }
                
                memcpy(input_buffer + input_len, paste_buf, copy_len);
                input_len += copy_len;
                cursor_pos = input_len;
                input_buffer[input_len] = '\0';
              }
              
              free(paste_buf);
            } else {
              if (input_len < INPUT_MAX - 2) {
                input_buffer[input_len++] = '\n';
              }
            }
          } else {
            if (input_len < INPUT_MAX - 2) {
              input_buffer[input_len++] = '\n';
            }
            
            int read_limit = INPUT_MAX - 2;
            
            while (peek_ch != ERR && input_len < read_limit) {
              if (isprint(peek_ch) || peek_ch == '\n') {
                input_buffer[input_len++] = (char)peek_ch;
              }
              peek_ch = wgetch(input_win);
            }
            
            flushinp();
            while (wgetch(input_win) != ERR) { }
            cursor_pos = input_len;
            input_buffer[input_len] = '\0';
          }
          
          nodelay(input_win, FALSE);
          last_input_len = input_len;
          last_input_time = get_time_ms();
          
          int new_height = ui_calc_input_height(input_buffer, getmaxx(input_win));
          if (new_height != current_input_height) {
            current_input_height = new_height;
            ui_layout_windows_with_input(&chat_win, &input_win, current_input_height);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);
          }
          ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                  input_focused, input_scroll_line, false);
          continue;
        }
        nodelay(input_win, FALSE);
      }
      
    process_enter:
      suggestion_box_close(&suggestions);

      if (!input_focused) {
        if (move_mode) {
          move_mode = false;
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp, true);
          continue;
        }
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, false);
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);
        continue;
      }

      if (input_len == 0 || is_only_whitespace(input_buffer)) {
        input_buffer[0] = '\0';
        input_len = 0;
        cursor_pos = 0;
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);
        continue;
      }

      input_buffer[input_len] = '\0';

      if (settings.paste_attachment_threshold > 0 &&
          input_len >= settings.paste_attachment_threshold &&
          strncmp(input_buffer, "[Attachment: ", 13) != 0) {
        char *attachment_ref = save_attachment(input_buffer, input_len);
        if (attachment_ref) {
          strncpy(input_buffer, attachment_ref, INPUT_MAX - 1);
          input_buffer[INPUT_MAX - 1] = '\0';
          input_len = (int)strlen(input_buffer);
          cursor_pos = input_len;
          free(attachment_ref);
          int new_height = ui_calc_input_height(input_buffer, getmaxx(input_win));
          if (new_height != current_input_height) {
            current_input_height = new_height;
            ui_layout_windows_with_input(&chat_win, &input_win,
                                         current_input_height);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);
          }
          ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                  input_focused, input_scroll_line, false);
          continue;
        }
      }

      if (strcmp(input_buffer, "/quit") == 0) {
        running = false;
        break;
      }

      if (input_buffer[0] == '/') {
        if (handle_slash_command(input_buffer, &modal, &models, &history,
                                 current_chat_id, current_char_path, &character,
                                 &persona, &character_loaded, &author_note,
                                 &lorebook)) {
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
                                    input_focused, input_scroll_line, false);
          } else {
            selected_msg = MSG_SELECT_NONE;
            input_focused = true;
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);
            ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                    input_focused, input_scroll_line, false);
          }
          continue;
        }
      }

      char *attachment_content = NULL;
      char *saved_input = NULL;
      bool saved_input_is_malloced = false;

      if (strncmp(input_buffer, "[Attachment: ", 13) == 0) {
        attachment_content = load_attachment(input_buffer);
        if (attachment_content) {
          size_t len = strlen(attachment_content);
          saved_input = malloc(len + 1);
          if (saved_input) {
            memcpy(saved_input, attachment_content, len + 1);
            saved_input_is_malloced = true;
          } else {
            saved_input = input_buffer;
          }
        } else {
          saved_input = input_buffer;
        }
      } else {
        size_t len = strlen(input_buffer);
        if (len < INPUT_MAX) {
          saved_input = input_buffer;
        } else {
          saved_input = malloc(len + 1);
          if (saved_input) {
            memcpy(saved_input, input_buffer, len + 1);
            saved_input_is_malloced = true;
          } else {
            saved_input = input_buffer;
          }
        }
      }

      char user_line[INPUT_MAX + 6];
      snprintf(user_line, sizeof(user_line), "You: %s", input_buffer);

      input_buffer[0] = '\0';
      input_len = 0;
      cursor_pos = 0;
      input_scroll_line = 0;

      if (current_input_height != 3) {
        current_input_height = 3;
        ui_layout_windows_with_input(&chat_win, &input_win,
                                     current_input_height);
      }

      size_t user_msg_idx = history_add(&history, user_line);
      if (user_msg_idx != SIZE_MAX) {
        ModelConfig *model = config_get_active(&models);
        if (model) {
          int user_tokens = llm_tokenize(model, saved_input);
          history_set_token_count(&history, user_msg_idx, 0, user_tokens);
        }
        selected_msg = MSG_SELECT_NONE;
        input_focused = true;
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, false);
      }
      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line, false);

      ModelConfig *active_model = config_get_active(&models);
      if (active_model)
        sampler_load(&current_samplers, active_model->api_type);
      LLMContext llm_ctx = {.character = character_loaded ? &character : NULL,
                            .persona = &persona,
                            .samplers = &current_samplers,
                            .author_note = &author_note,
                            .lorebook = &lorebook};
      
      do_llm_reply(&history, chat_win, input_win, saved_input, &models,
                   &selected_msg, &llm_ctx, user_disp, bot_disp);
      
      if (attachment_content) {
        free(attachment_content);
        attachment_content = NULL;
      }

      if (saved_input_is_malloced) {
        free(saved_input);
        saved_input = NULL;
      }

      const char *char_name =
          (character_loaded && character.name[0]) ? character.name : NULL;
      chat_auto_save_with_note(&history, &author_note, current_chat_id,
                               sizeof(current_chat_id), current_char_path,
                               char_name);
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
                       get_model_name(&models), user_disp, bot_disp, false);
        }

        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);

        int input_y = getbegy(input_win);
        int input_x = getbegx(input_win);
        bool needs_chat_redraw = suggestion_box_update(
            &suggestions, input_buffer, input_win, input_y, input_x);
        if (suggestion_box_is_open(&suggestions)) {
          if (needs_chat_redraw) {
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);
          }
          suggestion_box_draw(&suggestions);
        } else {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp, false);
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
                       get_model_name(&models), user_disp, bot_disp, false);
        }

        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);

        int input_y = getbegy(input_win);
        int input_x = getbegx(input_win);
        bool needs_chat_redraw = suggestion_box_update(
            &suggestions, input_buffer, input_win, input_y, input_x);
        if (suggestion_box_is_open(&suggestions)) {
          if (needs_chat_redraw) {
            touchwin(chat_win);
            ui_draw_chat(chat_win, &history, selected_msg,
                         get_model_name(&models), user_disp, bot_disp, false);
          }
          suggestion_box_draw(&suggestions);
        } else {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp, false);
        }
      }
      continue;
    }

    if (!input_focused)
      continue;

    if ((isprint(ch) || ch == '\n') && input_len < INPUT_MAX - 1) {
      long long now = get_time_ms();
      int input_growth = input_len - last_input_len;
      bool rapid_input = (now - last_input_time < 200 && input_growth > 3);
      
      nodelay(input_win, TRUE);
      int peek_ch = wgetch(input_win);
      bool has_more = (peek_ch != ERR);
      
      if (has_more) {
        if (settings.paste_attachment_threshold > 0) {
          size_t paste_len = 0;
          char *paste_buf = read_all_paste_input(input_win, ch, &paste_len);
          
          if (paste_buf) {
            if (paste_len >= (size_t)settings.paste_attachment_threshold &&
                strncmp(input_buffer, "[Attachment: ", 13) != 0) {
              size_t total_len = input_len + paste_len;
              size_t combined_cap = input_len + paste_len + 1;
              char *combined = malloc(combined_cap);
              if (combined) {
                memcpy(combined, input_buffer, input_len);
                memcpy(combined + input_len, paste_buf, paste_len);
                combined[total_len] = '\0';
                
                char *attachment_ref = save_attachment(combined, total_len);
                if (attachment_ref) {
                  strncpy(input_buffer, attachment_ref, INPUT_MAX - 1);
                  input_buffer[INPUT_MAX - 1] = '\0';
                  input_len = (int)strlen(input_buffer);
                  cursor_pos = input_len;
                  free(attachment_ref);
                }
                free(combined);
              }
            } else {
              int read_limit = INPUT_MAX - 2;
              if (input_len < read_limit) {
                input_buffer[input_len++] = (char)ch;
              }
              
              size_t copy_len = paste_len;
              if (input_len + copy_len > (size_t)read_limit) {
                copy_len = read_limit - input_len;
              }
              
              memcpy(input_buffer + input_len, paste_buf, copy_len);
              input_len += copy_len;
              cursor_pos = input_len;
              input_buffer[input_len] = '\0';
            }
            
            free(paste_buf);
          } else {
            if (input_len < INPUT_MAX - 2) {
              input_buffer[input_len++] = (char)ch;
            }
          }
        } else {
          int read_limit = INPUT_MAX - 2;
          
          if (input_len < read_limit) {
            input_buffer[input_len++] = (char)ch;
          }
          
          while (peek_ch != ERR && input_len < read_limit) {
            if (isprint(peek_ch) || peek_ch == '\n') {
              input_buffer[input_len++] = (char)peek_ch;
            }
            peek_ch = wgetch(input_win);
          }
          
          flushinp();
          while (wgetch(input_win) != ERR) { }
          cursor_pos = input_len;
          input_buffer[input_len] = '\0';
        }
        
        nodelay(input_win, FALSE);
        last_input_len = input_len;
        last_input_time = get_time_ms();
        
        int new_height = ui_calc_input_height(input_buffer, getmaxx(input_win));
        if (new_height != current_input_height) {
          current_input_height = new_height;
          ui_layout_windows_with_input(&chat_win, &input_win, current_input_height);
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp, false);
        }
        ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                input_focused, input_scroll_line, false);
        continue;
      }
      
      nodelay(input_win, FALSE);
      
      if (input_len < INPUT_MAX - 2) {
        memmove(&input_buffer[cursor_pos + 1], &input_buffer[cursor_pos],
                input_len - cursor_pos + 1);
        input_buffer[cursor_pos] = (char)ch;
        input_len++;
        cursor_pos++;
      }
      
      last_input_len = input_len;
      last_input_time = now;
      
      if (settings.paste_attachment_threshold > 0 &&
          input_len >= settings.paste_attachment_threshold &&
          strncmp(input_buffer, "[Attachment: ", 13) != 0) {
        input_buffer[input_len] = '\0';
        char *attachment_ref = save_attachment(input_buffer, input_len);
        if (attachment_ref) {
          strncpy(input_buffer, attachment_ref, INPUT_MAX - 1);
          input_buffer[INPUT_MAX - 1] = '\0';
          input_len = (int)strlen(input_buffer);
          cursor_pos = input_len;
          free(attachment_ref);
          ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                                  input_focused, input_scroll_line, false);
          continue;
        }
      }
      
      if (rapid_input) {
        continue;
      }

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
                     user_disp, bot_disp, false);
      }

      ui_draw_input_multiline(input_win, input_buffer, cursor_pos,
                              input_focused, input_scroll_line, false);

      int input_y = getbegy(input_win);
      int input_x = getbegx(input_win);
      bool needs_chat_redraw = suggestion_box_update(
          &suggestions, input_buffer, input_win, input_y, input_x);
      if (suggestion_box_is_open(&suggestions)) {
        if (needs_chat_redraw) {
          touchwin(chat_win);
          ui_draw_chat(chat_win, &history, selected_msg,
                       get_model_name(&models), user_disp, bot_disp, false);
        }
        suggestion_box_draw(&suggestions);
      } else {
        touchwin(chat_win);
        ui_draw_chat(chat_win, &history, selected_msg, get_model_name(&models),
                     user_disp, bot_disp, false);
      }
    }
  }

  suggestion_box_free(&suggestions);
  modal_close(&modal);
  delwin(chat_win);
  delwin(input_win);
  endwin();
  history_free(&history);
  lorebook_free(&lorebook);
  if (character_loaded) {
    character_free(&character);
  }
  free(in_place_edit.buffer);
  llm_cleanup();
  return EXIT_SUCCESS;
}

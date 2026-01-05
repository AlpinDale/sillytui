/*
 * Ncurses wrapper for syntax highlighters
 * Uses kat for C, ulight for everything else
 */

#include "syntax_ncurses.h"
#include "c/highlight.h"
#include "ulight/ulight.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool g_syntax_initialized = false;
static bool g_kat_initialized = false;
static ulight_lang g_current_lang = ULIGHT_LANG_NONE;
static bool g_use_kat = false;

void syntax_init_colors(void) {
  if (!has_colors())
    return;

  init_pair(SYN_PAIR_DEFAULT, -1, -1);
  init_pair(SYN_PAIR_PREPROCESSOR, 63, -1);
  init_pair(SYN_PAIR_TYPE, 83, -1);
  init_pair(SYN_PAIR_KEYWORD, 227, -1);
  init_pair(SYN_PAIR_NUMBER, 214, -1);
  init_pair(SYN_PAIR_STRING, 207, -1);
  init_pair(SYN_PAIR_COMMENT, 102, -1);
  init_pair(SYN_PAIR_FUNCTION, 193, -1);
  init_pair(SYN_PAIR_SYMBOL, 101, -1);
}

int syntax_highlight_init(void) {
  if (g_syntax_initialized)
    return 0;

  if (highlight_init(NULL) == 0)
    g_kat_initialized = true;

  g_syntax_initialized = true;
  return 0;
}

void syntax_highlight_finish(void) {
  if (g_kat_initialized) {
    highlight_finish();
    g_kat_initialized = false;
  }
  g_syntax_initialized = false;
}

void syntax_state_init(SyntaxHighlightState *state) {
  state->state = 0;
  if (g_kat_initialized)
    highlight_reset_state();
}

void syntax_set_language(const char *lang_name) {
  if (!lang_name) {
    g_current_lang = ULIGHT_LANG_NONE;
    g_use_kat = false;
    return;
  }

  if (strcmp(lang_name, "c") == 0 || strcmp(lang_name, "h") == 0) {
    g_use_kat = true;
    g_current_lang = ULIGHT_LANG_NONE;
  } else {
    g_use_kat = false;
    g_current_lang = ulight_get_lang(lang_name, strlen(lang_name));
  }
}

static int datah_to_color_pair(const char *datah) {
  if (strncmp(datah, "kw", 2) == 0)
    return SYN_PAIR_KEYWORD;

  if (strncmp(datah, "cmt", 3) == 0)
    return SYN_PAIR_COMMENT;

  if (strncmp(datah, "str", 3) == 0)
    return SYN_PAIR_STRING;

  if (strncmp(datah, "num", 3) == 0)
    return SYN_PAIR_NUMBER;

  if (strncmp(datah, "sym_", 4) == 0)
    return SYN_PAIR_SYMBOL;

  if (strncmp(datah, "name_mac", 8) == 0)
    return SYN_PAIR_PREPROCESSOR;

  if (strncmp(datah, "name_function", 13) == 0)
    return SYN_PAIR_FUNCTION;

  if (strncmp(datah, "name_type", 9) == 0)
    return SYN_PAIR_TYPE;

  if (strcmp(datah, "bool") == 0 || strcmp(datah, "null") == 0)
    return SYN_PAIR_KEYWORD;

  if (strncmp(datah, "name", 4) == 0)
    return SYN_PAIR_DEFAULT;

  return SYN_PAIR_DEFAULT;
}

typedef struct {
  WINDOW *win;
  int row;
  int col;
  int max_width;
  int cursor;
  int current_pair;
} RenderContext;

static char decode_html_entity(const char *text, size_t length,
                               size_t *consumed) {
  if (length < 4)
    return 0;

  if (strncmp(text, "&lt;", 4) == 0) {
    *consumed = 4;
    return '<';
  }
  if (strncmp(text, "&gt;", 4) == 0) {
    *consumed = 4;
    return '>';
  }
  if (length >= 5 && strncmp(text, "&amp;", 5) == 0) {
    *consumed = 5;
    return '&';
  }
  if (length >= 6 && strncmp(text, "&quot;", 6) == 0) {
    *consumed = 6;
    return '"';
  }
  if (length >= 6 && strncmp(text, "&apos;", 6) == 0) {
    *consumed = 6;
    return '\'';
  }

  return 0;
}

static void flush_callback(const void *userdata, char *text, size_t length) {
  RenderContext *ctx = (RenderContext *)userdata;

  for (size_t i = 0; i < length && ctx->cursor < ctx->max_width; i++) {
    char ch = text[i];

    if (ch == '<' && i + 2 < length && text[i + 1] == 'h' &&
        text[i + 2] == '-') {
      i += 3;
      while (i < length && text[i] == ' ')
        i++;

      if (i + 6 < length && strncmp(&text[i], "data-h", 6) == 0) {
        i += 6;
        while (i < length && text[i] != '=')
          i++;
        if (i < length)
          i++;

        size_t start = i;
        while (i < length && text[i] != '>')
          i++;

        if (i > start) {
          char datah[64];
          size_t len = i - start;
          if (len > 63)
            len = 63;
          memcpy(datah, &text[start], len);
          datah[len] = '\0';

          int pair = datah_to_color_pair(datah);
          if (pair != ctx->current_pair) {
            if (has_colors()) {
              wattrset(ctx->win, A_NORMAL);
              if (pair > 0)
                wattron(ctx->win, COLOR_PAIR(pair));
            }
            ctx->current_pair = pair;
          }
        }
      }

      while (i < length && text[i] != '>')
        i++;
      continue;
    }

    if (ch == '<' && i + 3 < length && text[i + 1] == '/' &&
        text[i + 2] == 'h' && text[i + 3] == '-') {
      while (i < length && text[i] != '>')
        i++;
      if (has_colors()) {
        wattrset(ctx->win, A_NORMAL);
        wattron(ctx->win, COLOR_PAIR(SYN_PAIR_DEFAULT));
      }
      ctx->current_pair = SYN_PAIR_DEFAULT;
      continue;
    }

    if (ch == '\n' || ch == '\r')
      continue;

    if (ch == '&') {
      size_t consumed = 0;
      char decoded = decode_html_entity(&text[i], length - i, &consumed);
      if (decoded) {
        mvwaddch(ctx->win, ctx->row, ctx->col + ctx->cursor, decoded);
        ctx->cursor++;
        i += consumed - 1;
        continue;
      }
    }

    mvwaddch(ctx->win, ctx->row, ctx->col + ctx->cursor, ch);
    ctx->cursor++;
  }
}

static int ansi_to_color_pair(const char *ansi_code) {
  if (strncmp(ansi_code, "\033[38;5;63m", 10) == 0)
    return SYN_PAIR_PREPROCESSOR;
  if (strncmp(ansi_code, "\033[38;5;83m", 10) == 0)
    return SYN_PAIR_TYPE;
  if (strncmp(ansi_code, "\033[38;5;227m", 11) == 0)
    return SYN_PAIR_KEYWORD;
  if (strncmp(ansi_code, "\033[38;5;214m", 11) == 0)
    return SYN_PAIR_NUMBER;
  if (strncmp(ansi_code, "\033[38;5;207m", 11) == 0)
    return SYN_PAIR_STRING;
  if (strncmp(ansi_code, "\033[38;5;102m", 11) == 0)
    return SYN_PAIR_COMMENT;
  if (strncmp(ansi_code, "\033[38;5;193m", 11) == 0)
    return SYN_PAIR_FUNCTION;
  if (strncmp(ansi_code, "\033[38;5;101m", 11) == 0)
    return SYN_PAIR_SYMBOL;
  return 0;
}

static void render_line_kat(WINDOW *win, int row, int col, int max_width,
                            const char *line) {
  if (!g_kat_initialized)
    return;

  char *highlighted = highlight_line(line, NULL, 0);
  if (!highlighted)
    return;

  int cursor = 0;
  size_t len = strlen(highlighted);

  if (has_colors())
    wattron(win, COLOR_PAIR(SYN_PAIR_DEFAULT));

  for (size_t i = 0; i < len && cursor < max_width;) {
    if (highlighted[i] == '\033' && i + 1 < len && highlighted[i + 1] == '[') {
      size_t end = i + 2;
      while (end < len && highlighted[end] != 'm')
        end++;

      if (end < len) {
        if (strncmp(&highlighted[i], "\033[0m", 4) == 0) {
          if (has_colors()) {
            wattrset(win, A_NORMAL);
            wattron(win, COLOR_PAIR(SYN_PAIR_DEFAULT));
          }
        } else {
          int pair = ansi_to_color_pair(&highlighted[i]);
          if (pair > 0 && has_colors())
            wattron(win, COLOR_PAIR(pair));
        }
        i = end + 1;
        continue;
      }
    }

    if (highlighted[i] == '\0')
      break;

    mvwaddch(win, row, col + cursor, highlighted[i]);
    cursor++;
    i++;
  }

  wattrset(win, A_NORMAL);
  highlight_free(highlighted);
}

void syntax_render_line(WINDOW *win, int row, int col, int max_width,
                        const char *line, SyntaxHighlightState *state) {
  (void)state;

  if (!g_syntax_initialized)
    return;

  if (g_use_kat) {
    render_line_kat(win, row, col, max_width, line);
    return;
  }

  if (g_current_lang == ULIGHT_LANG_NONE)
    return;

  ulight_state ustate;
  ulight_init(&ustate);

  ustate.source = line;
  ustate.source_length = strlen(line);
  ustate.lang = g_current_lang;

  ulight_token token_buffer[512];
  char text_buffer[4096];
  ustate.token_buffer = token_buffer;
  ustate.token_buffer_length = 512;
  ustate.text_buffer = text_buffer;
  ustate.text_buffer_length = 4096;

  RenderContext ctx = {.win = win,
                       .row = row,
                       .col = col,
                       .max_width = max_width,
                       .cursor = 0,
                       .current_pair = SYN_PAIR_DEFAULT};

  ustate.flush_text = flush_callback;
  ustate.flush_text_data = &ctx;

  if (has_colors())
    wattron(win, COLOR_PAIR(SYN_PAIR_DEFAULT));

  ulight_status status = ulight_source_to_html(&ustate);
  if (status != ULIGHT_STATUS_OK) {
    for (size_t i = 0; i < ustate.source_length && ctx.cursor < max_width;
         i++) {
      if (line[i] != '\n' && line[i] != '\r') {
        mvwaddch(win, row, col + ctx.cursor, line[i]);
        ctx.cursor++;
      }
    }
  }

  wattrset(win, A_NORMAL);
  ulight_destroy(&ustate);
}

/*
 * Ncurses wrapper for syntax highlighters
 * Uses Kat for C syntax highlighting only
 */

#include "syntax_ncurses.h"
#include "c/highlight.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool g_syntax_initialized = false;
static bool g_kat_initialized = false;
static bool g_highlighting_enabled = false;

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
  g_highlighting_enabled = false;
}

void syntax_state_init(SyntaxHighlightState *state) {
  state->state = 0;
  if (g_kat_initialized)
    highlight_reset_state();
}

void syntax_set_language(const char *lang_name) {
  if (!lang_name) {
    g_highlighting_enabled = false;
    return;
  }

  /* Only C and header files get syntax highlighting */
  if (strcmp(lang_name, "c") == 0 || strcmp(lang_name, "h") == 0) {
    g_highlighting_enabled = true;
  } else {
    g_highlighting_enabled = false;
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

  if (!g_syntax_initialized || !g_highlighting_enabled)
    return;

  render_line_kat(win, row, col, max_width, line);
}

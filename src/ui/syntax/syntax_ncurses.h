/*
 * Ncurses wrapper for Kat syntax highlighter
 * Adapts Kat's ANSI-based highlighter for ncurses color pairs
 */

#ifndef SYNTAX_NCURSES_H
#define SYNTAX_NCURSES_H

#include <curses.h>
#include <stdbool.h>
#include <stddef.h>

#define SYN_PAIR_DEFAULT 40
#define SYN_PAIR_PREPROCESSOR 41
#define SYN_PAIR_TYPE 42
#define SYN_PAIR_KEYWORD 43
#define SYN_PAIR_NUMBER 44
#define SYN_PAIR_STRING 45
#define SYN_PAIR_COMMENT 46
#define SYN_PAIR_FUNCTION 47
#define SYN_PAIR_SYMBOL 48

typedef struct {
  int state;
} SyntaxHighlightState;

void syntax_init_colors(void);
int syntax_highlight_init(void);
void syntax_highlight_finish(void);
void syntax_state_init(SyntaxHighlightState *state);
void syntax_set_language(const char *lang_name);

void syntax_render_line(WINDOW *win, int row, int col, int max_width,
                        const char *line, SyntaxHighlightState *state);

#endif

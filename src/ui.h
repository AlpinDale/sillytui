#ifndef UI_H
#define UI_H

#include "history.h"
#include <ncurses.h>

#define COLOR_PAIR_BORDER 10
#define COLOR_PAIR_TITLE 11
#define COLOR_PAIR_PROMPT 12
#define COLOR_PAIR_USER 13
#define COLOR_PAIR_BOT 14
#define COLOR_PAIR_HINT 15
#define COLOR_PAIR_LOADING 16
#define COLOR_PAIR_SUGGEST_ACTIVE 17
#define COLOR_PAIR_SUGGEST_DESC 18

typedef struct {
  const char *name;
  const char *description;
} SlashCommand;

typedef struct {
  WINDOW *win;
  int active_index;
  int scroll_offset;
  int visible_count;
  const SlashCommand *commands;
  int command_count;
  const char *filter;
  int *matched_indices;
  int matched_count;
} SuggestionBox;

void ui_init_colors(void);
void ui_layout_windows(WINDOW **chat_win, WINDOW **input_win);
void ui_draw_chat(WINDOW *chat_win, const ChatHistory *history,
                  int scroll_offset, const char *model_name);
void ui_draw_input(WINDOW *input_win, const char *buffer, int cursor_pos);
int ui_get_total_lines(WINDOW *chat_win, const ChatHistory *history);

void suggestion_box_init(SuggestionBox *sb, const SlashCommand *commands,
                         int count);
void suggestion_box_update(SuggestionBox *sb, const char *filter,
                           WINDOW *parent, int parent_y, int parent_x);
void suggestion_box_draw(SuggestionBox *sb);
void suggestion_box_navigate(SuggestionBox *sb, int direction);
const char *suggestion_box_get_selected(SuggestionBox *sb);
void suggestion_box_close(SuggestionBox *sb);
bool suggestion_box_is_open(const SuggestionBox *sb);

#endif

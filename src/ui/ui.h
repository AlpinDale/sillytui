#ifndef UI_H
#define UI_H

#include "chat/history.h"
#include <curses.h>

#define COLOR_PAIR_BORDER 10
#define COLOR_PAIR_TITLE 11
#define COLOR_PAIR_PROMPT 12
#define COLOR_PAIR_USER 13
#define COLOR_PAIR_BOT 14
#define COLOR_PAIR_HINT 15
#define COLOR_PAIR_LOADING 16
#define COLOR_PAIR_SUGGEST_ACTIVE 17
#define COLOR_PAIR_SUGGEST_DESC 18
#define COLOR_PAIR_MSG_SELECTED 19
#define COLOR_PAIR_SWIPE 20
#define COLOR_PAIR_SWIPE_SEL 21
#define COLOR_PAIR_MODAL_BG 28

#define MAX_DYNAMIC_ITEMS 32
#define MSG_SELECT_NONE -1
#define INPUT_MAX_LINES 8

typedef struct {
  const char *name;
  const char *description;
} SlashCommand;

typedef struct {
  char name[128];
  char description[128];
  char id[128];
} DynamicItem;

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
  DynamicItem *dynamic_items;
  int dynamic_count;
  bool showing_dynamic;
  bool showing_characters;
  char current_character[128];
} SuggestionBox;

typedef struct {
  int line;
  int col;
} InputCursorPos;

typedef struct {
  bool active;
  int msg_index;
  int swipe_index;
  char *buffer;
  int buf_len;
  int buf_cap;
  int cursor_pos;
  int scroll_offset;
} InPlaceEdit;

void ui_init_colors(void);
int ui_calc_input_height(const char *buffer, int win_width);
void ui_layout_windows_with_input(WINDOW **chat_win, WINDOW **input_win,
                                  int input_height);
void ui_draw_chat_ex(WINDOW *chat_win, const ChatHistory *history,
                     int selected_msg, const char *model_name,
                     const char *user_name, const char *bot_name,
                     bool show_edit_hints, bool move_mode, InPlaceEdit *edit);
#define ui_draw_chat(w, h, s, m, u, b, e)                                      \
  ui_draw_chat_ex(w, h, s, m, u, b, e, false, NULL)
void ui_draw_input_multiline(WINDOW *input_win, const char *buffer,
                             int cursor_pos, bool focused, int scroll_line,
                             bool editing_mode);
int ui_get_total_lines(WINDOW *chat_win, const ChatHistory *history);
int ui_get_msg_scroll_offset(WINDOW *chat_win, const ChatHistory *history,
                             int selected_msg, const InPlaceEdit *edit);
InputCursorPos ui_cursor_to_line_col(const char *buffer, int cursor_pos,
                                     int text_width);
int ui_line_col_to_cursor(const char *buffer, int line, int col,
                          int text_width);
int ui_get_input_line_count(const char *buffer, int text_width);

void suggestion_box_init(SuggestionBox *sb, const SlashCommand *commands,
                         int count);
void suggestion_box_free(SuggestionBox *sb);
bool suggestion_box_update(SuggestionBox *sb, const char *filter,
                           WINDOW *parent, int parent_y, int parent_x);
void suggestion_box_draw(SuggestionBox *sb);
void suggestion_box_navigate(SuggestionBox *sb, int direction);
const char *suggestion_box_get_selected(SuggestionBox *sb);
const char *suggestion_box_get_selected_id(SuggestionBox *sb);
void suggestion_box_close(SuggestionBox *sb);
bool suggestion_box_is_open(const SuggestionBox *sb);

#endif

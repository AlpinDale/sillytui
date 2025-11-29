#include "ui/ui.h"
#include "chat/chat.h"
#include "chat/history.h"
#include "core/macros.h"
#include "ui/markdown.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool g_ui_colors = false;

#define MAX_EXPANDED_REASONING 256
static bool g_reasoning_expanded[MAX_EXPANDED_REASONING] = {false};

bool ui_is_reasoning_expanded(size_t msg_index) {
  if (msg_index >= MAX_EXPANDED_REASONING)
    return false;
  return g_reasoning_expanded[msg_index];
}

void ui_toggle_reasoning(size_t msg_index) {
  if (msg_index >= MAX_EXPANDED_REASONING)
    return;
  g_reasoning_expanded[msg_index] = !g_reasoning_expanded[msg_index];
}

void ui_reset_reasoning_state(void) {
  memset(g_reasoning_expanded, 0, sizeof(g_reasoning_expanded));
}

#define COLOR_PAIR_BORDER_DIM 22
#define COLOR_PAIR_ACCENT 23
#define COLOR_PAIR_STATUS 24
#define COLOR_PAIR_USER_SEL 25
#define COLOR_PAIR_BOT_SEL 26
#define COLOR_PAIR_HINT_SEL 27
#define COLOR_PAIR_MODAL_BG 28
#define COLOR_PAIR_SYSTEM 29
#define COLOR_PAIR_SYSTEM_SEL 30

#define SEL_BG 236

void ui_init_colors(void) {
  if (!has_colors()) {
    g_ui_colors = false;
    return;
  }
  init_pair(COLOR_PAIR_BORDER, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_TITLE, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_PROMPT, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_USER, COLOR_GREEN, -1);
  init_pair(COLOR_PAIR_BOT, COLOR_MAGENTA, -1);
  init_pair(COLOR_PAIR_HINT, 8, -1);
  init_pair(COLOR_PAIR_LOADING, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_SUGGEST_ACTIVE, COLOR_BLACK, COLOR_CYAN);
  init_pair(COLOR_PAIR_SUGGEST_DESC, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_MSG_SELECTED, -1, SEL_BG);
  init_pair(COLOR_PAIR_SWIPE, 8, -1);
  init_pair(COLOR_PAIR_SWIPE_SEL, 8, SEL_BG);
  init_pair(COLOR_PAIR_BORDER_DIM, 8, -1);
  init_pair(COLOR_PAIR_ACCENT, COLOR_MAGENTA, -1);
  init_pair(COLOR_PAIR_STATUS, 8, -1);
  init_pair(COLOR_PAIR_USER_SEL, COLOR_GREEN, SEL_BG);
  init_pair(COLOR_PAIR_BOT_SEL, COLOR_MAGENTA, SEL_BG);
  init_pair(COLOR_PAIR_HINT_SEL, 8, SEL_BG);
  init_pair(COLOR_PAIR_MODAL_BG, COLOR_WHITE, COLOR_BLACK);
  init_pair(COLOR_PAIR_SYSTEM, COLOR_YELLOW, -1);
  init_pair(COLOR_PAIR_SYSTEM_SEL, COLOR_YELLOW, SEL_BG);
  init_pair(COLOR_PAIR_TOKEN1, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_TOKEN2, COLOR_MAGENTA, -1);
  init_pair(COLOR_PAIR_TOKEN3, COLOR_YELLOW, -1);
  g_ui_colors = true;
}

static void draw_rounded_box(WINDOW *win, int color_pair, bool focused) {
  int h, w;
  getmaxyx(win, h, w);
  if (h < 2 || w < 2)
    return;

  int actual_color = focused ? color_pair : COLOR_PAIR_BORDER_DIM;
  if (g_ui_colors)
    wattron(win, COLOR_PAIR(actual_color));

  mvwaddstr(win, 0, 0, "╭");
  for (int x = 1; x < w - 1; x++)
    mvwaddstr(win, 0, x, "─");
  mvwaddstr(win, 0, w - 1, "╮");

  for (int y = 1; y < h - 1; y++) {
    mvwaddstr(win, y, 0, "│");
    mvwaddstr(win, y, w - 1, "│");
  }

  mvwaddstr(win, h - 1, 0, "╰");
  for (int x = 1; x < w - 1; x++)
    mvwaddstr(win, h - 1, x, "─");
  mvwaddstr(win, h - 1, w - 1, "╯");

  if (g_ui_colors)
    wattroff(win, COLOR_PAIR(actual_color));
}

static void draw_title(WINDOW *win, const char *title, int color_pair) {
  int w = getmaxx(win);
  int title_len = (int)strlen(title);
  int pos = (w - title_len - 2) / 2;
  if (pos < 2)
    pos = 2;

  if (g_ui_colors)
    wattron(win, COLOR_PAIR(color_pair) | A_BOLD);
  mvwprintw(win, 0, pos, " %s ", title);
  if (g_ui_colors)
    wattroff(win, COLOR_PAIR(color_pair) | A_BOLD);
}

int ui_calc_input_height_ex(const char *buffer, int win_width,
                            const AttachmentList *attachments) {
  int attach_height = ui_attachment_bar_height(attachments);

  if (!buffer || buffer[0] == '\0')
    return 3 + attach_height;

  int text_width = win_width - 6;
  if (text_width < 10)
    text_width = 10;

  int lines = ui_get_input_line_count(buffer, text_width);
  int height = lines + 2 + attach_height;

  if (height < 3 + attach_height)
    height = 3 + attach_height;
  if (height > INPUT_MAX_LINES + 2 + attach_height)
    height = INPUT_MAX_LINES + 2 + attach_height;

  return height;
}

void ui_layout_windows_with_input(WINDOW **chat_win, WINDOW **input_win,
                                  int input_height) {
  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  if (input_height < 3)
    input_height = 3;
  if (input_height > rows - 3)
    input_height = rows - 3;

  int chat_height = rows - input_height;
  if (chat_height < 3)
    chat_height = 3;

  if (*chat_win)
    delwin(*chat_win);
  if (*input_win)
    delwin(*input_win);

  *chat_win = newwin(chat_height, cols, 0, 0);
  *input_win = newwin(input_height, cols, chat_height, 0);
  keypad(*input_win, TRUE);
}

static bool starts_with(const char *str, const char *prefix) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

typedef struct {
  const char *start;
  int len;
} WrappedLine;

static int wrap_text(const char *text, int width, WrappedLine *out,
                     int max_lines) {
  if (!text || width <= 0 || !out || max_lines <= 0)
    return 0;

  int line_count = 0;
  const char *p = text;

  while (*p && line_count < max_lines) {
    while (*p == ' ')
      p++;
    if (!*p)
      break;

    const char *line_start = p;
    const char *last_break = NULL;
    int col = 0;

    while (*p && *p != '\n' && col < width) {
      if (*p == ' ')
        last_break = p;
      p++;
      col++;
    }

    int line_len;
    if (!*p || *p == '\n') {
      line_len = (int)(p - line_start);
      if (*p == '\n')
        p++;
    } else if (last_break && last_break > line_start) {
      line_len = (int)(last_break - line_start);
      p = last_break + 1;
    } else {
      line_len = col;
    }

    out[line_count].start = line_start;
    out[line_count].len = line_len;
    line_count++;
  }

  if (line_count == 0 && text[0] != '\0') {
    out[0].start = text;
    out[0].len = 0;
    line_count = 1;
  }

  return line_count;
}

static int count_wrapped_lines(const char *text, int width) {
  if (!text || width <= 0)
    return 1;
  if (text[0] == '\0')
    return 1;

  int line_count = 0;
  const char *p = text;

  while (*p) {
    while (*p == ' ')
      p++;
    if (!*p)
      break;

    const char *line_start = p;
    const char *last_break = NULL;
    int col = 0;

    while (*p && *p != '\n' && col < width) {
      if (*p == ' ')
        last_break = p;
      p++;
      col++;
    }

    if (!*p || *p == '\n') {
      if (*p == '\n')
        p++;
    } else if (last_break && last_break > line_start) {
      p = last_break + 1;
    }

    line_count++;
  }

  return line_count > 0 ? line_count : 1;
}

typedef struct {
  int msg_index;
  int line_in_msg;
  const char *line_start;
  int line_len;
  bool is_user;
  bool is_bot;
  bool is_system;
  bool is_first_line;
  bool is_spacer;
  bool is_name_line;
  bool is_reasoning_indicator;
  double reasoning_time_ms;
  unsigned initial_style;
} DisplayLine;

static int build_display_lines(const ChatHistory *history, int content_width,
                               DisplayLine *lines, int max_lines,
                               const InPlaceEdit *edit, const char *char_name,
                               const char *user_name, char **subst_strs,
                               int *subst_count) {
  int total = 0;
  int gutter_width = 3;
  int text_width = content_width - gutter_width;
  if (text_width < 10)
    text_width = 10;

  if (subst_count)
    *subst_count = 0;

  WrappedLine *wrapped = malloc(sizeof(WrappedLine) * 200);
  if (!wrapped)
    return 0;

  for (size_t i = 0; i < history->count && total < max_lines; i++) {
    const char *msg = history_get(history, i);
    if (!msg)
      continue;

    bool use_edit_buffer =
        edit && edit->active && edit->msg_index == (int)i && edit->buffer;

    MessageRole role = history_get_role(history, i);
    bool is_user = (role == ROLE_USER);
    bool is_bot = (role == ROLE_ASSISTANT);
    bool is_system = (role == ROLE_SYSTEM);

    const char *raw_content;
    if (use_edit_buffer) {
      raw_content = edit->buffer;
    } else if (is_user && starts_with(msg, "You: ")) {
      raw_content = msg + 5;
    } else if (is_bot && starts_with(msg, "Bot:")) {
      raw_content = msg + 4;
      while (*raw_content == ' ')
        raw_content++;
    } else {
      raw_content = msg;
    }

    char *substituted = NULL;
    if (char_name || user_name) {
      substituted = macro_substitute(raw_content, char_name, user_name);
    }
    const char *content = substituted ? substituted : raw_content;
    if (substituted && subst_strs && subst_count && *subst_count < 500) {
      subst_strs[(*subst_count)++] = substituted;
    }

    if ((is_user || is_bot || is_system) && total < max_lines) {
      lines[total].msg_index = (int)i;
      lines[total].line_in_msg = -1;
      lines[total].line_start = NULL;
      lines[total].line_len = 0;
      lines[total].is_user = is_user;
      lines[total].is_bot = is_bot;
      lines[total].is_system = is_system;
      lines[total].is_first_line = true;
      lines[total].is_spacer = false;
      lines[total].is_name_line = true;
      lines[total].is_reasoning_indicator = false;
      lines[total].reasoning_time_ms = 0;
      lines[total].initial_style = 0;
      total++;
    }

    size_t active_swipe = history_get_active_swipe(history, i);
    const char *reasoning = history_get_reasoning(history, i, active_swipe);
    if (reasoning && reasoning[0] && total < max_lines) {
      double reasoning_time =
          history_get_reasoning_time(history, i, active_swipe);
      bool expanded = ui_is_reasoning_expanded(i);

      lines[total].msg_index = (int)i;
      lines[total].line_in_msg = -2;
      lines[total].line_start = reasoning;
      lines[total].line_len = (int)strlen(reasoning);
      lines[total].is_user = false;
      lines[total].is_bot = is_bot;
      lines[total].is_system = false;
      lines[total].is_first_line = false;
      lines[total].is_spacer = false;
      lines[total].is_name_line = false;
      lines[total].is_reasoning_indicator = true;
      lines[total].reasoning_time_ms = reasoning_time;
      lines[total].initial_style = 0;
      total++;

      if (expanded) {
        int num_reasoning_wrapped =
            wrap_text(reasoning, text_width - 2, wrapped, 200);
        for (int rline = 0; rline < num_reasoning_wrapped && total < max_lines;
             rline++) {
          lines[total].msg_index = (int)i;
          lines[total].line_in_msg = -3 - rline;
          lines[total].line_start = wrapped[rline].start;
          lines[total].line_len = wrapped[rline].len;
          lines[total].is_user = false;
          lines[total].is_bot = is_bot;
          lines[total].is_system = false;
          lines[total].is_first_line = false;
          lines[total].is_spacer = false;
          lines[total].is_name_line = false;
          lines[total].is_reasoning_indicator = false;
          lines[total].reasoning_time_ms = -1;
          lines[total].initial_style = 0;
          total++;
        }
      }
    }

    int num_wrapped = wrap_text(content, text_width, wrapped, 200);

    unsigned running_style = 0;
    for (int line = 0; line < num_wrapped && total < max_lines; line++) {
      lines[total].msg_index = (int)i;
      lines[total].line_in_msg = line;
      lines[total].line_start = wrapped[line].start;
      lines[total].line_len = wrapped[line].len;
      lines[total].is_user = is_user;
      lines[total].is_bot = is_bot;
      lines[total].is_system = is_system;
      lines[total].is_first_line = false;
      lines[total].is_spacer = false;
      lines[total].is_name_line = false;
      lines[total].is_reasoning_indicator = false;
      lines[total].reasoning_time_ms = 0;
      lines[total].initial_style = running_style;

      running_style = markdown_compute_style_after(
          wrapped[line].start, wrapped[line].len, running_style);

      total++;
    }

    if (i < history->count - 1 && total < max_lines) {
      lines[total].msg_index = (int)i;
      lines[total].line_in_msg = -1;
      lines[total].line_start = NULL;
      lines[total].line_len = 0;
      lines[total].is_user = false;
      lines[total].is_bot = false;
      lines[total].is_system = false;
      lines[total].is_first_line = false;
      lines[total].is_spacer = true;
      lines[total].is_name_line = false;
      lines[total].is_reasoning_indicator = false;
      lines[total].reasoning_time_ms = 0;
      lines[total].initial_style = 0;
      total++;
    }
  }

  free(wrapped);
  return total;
}

int ui_get_total_lines(WINDOW *chat_win, const ChatHistory *history) {
  int height, width;
  getmaxyx(chat_win, height, width);
  (void)height;

  int content_width = width - 4;
  if (content_width <= 0)
    return 0;

  int gutter_width = 3;
  int text_width = content_width - gutter_width;
  if (text_width < 10)
    text_width = 10;

  int total = 0;
  for (size_t i = 0; i < history->count; i++) {
    const char *msg = history_get(history, i);
    if (!msg)
      continue;
    const char *content = msg;
    bool is_user = starts_with(msg, "You: ");
    bool is_bot = starts_with(msg, "Bot:");

    if (is_user) {
      content = msg + 5;
    } else if (is_bot) {
      content = msg + 4;
      while (*content == ' ')
        content++;
    }

    if (is_user || is_bot)
      total++;
    total += count_wrapped_lines(content, text_width);
    if (i < history->count - 1)
      total++;
  }
  return total;
}

int ui_get_msg_scroll_offset(WINDOW *chat_win, const ChatHistory *history,
                             int selected_msg, const InPlaceEdit *edit) {
  if (selected_msg < 0 || history->count == 0)
    return -1;

  int height, width;
  getmaxyx(chat_win, height, width);
  int usable_lines = height - 2;
  int content_width = width - 4;
  if (usable_lines <= 0 || content_width <= 0)
    return -1;

  int max_display = (int)history->count * 100 + 100;
  DisplayLine *all_lines = malloc(sizeof(DisplayLine) * max_display);
  if (!all_lines)
    return -1;

  int subst_count = 0;
  int total_display_lines =
      build_display_lines(history, content_width, all_lines, max_display, edit,
                          NULL, NULL, NULL, &subst_count);

  int msg_start_line = -1;
  int msg_end_line = -1;
  for (int i = 0; i < total_display_lines; i++) {
    if (all_lines[i].msg_index == selected_msg) {
      if (msg_start_line < 0)
        msg_start_line = i;
      msg_end_line = i;
    }
  }

  free(all_lines);

  if (msg_start_line < 0)
    return -1;

  int max_scroll = total_display_lines - usable_lines;
  if (max_scroll < 0)
    max_scroll = 0;

  int msg_height = msg_end_line - msg_start_line + 1;
  int target_scroll;

  if (msg_height >= usable_lines) {
    target_scroll = msg_start_line;
  } else {
    target_scroll = msg_start_line - (usable_lines - msg_height) / 2;
  }

  if (target_scroll < 0)
    target_scroll = 0;
  if (target_scroll > max_scroll)
    target_scroll = max_scroll;

  return target_scroll;
}

void ui_draw_chat_ex(WINDOW *chat_win, const ChatHistory *history,
                     int selected_msg, const char *model_name,
                     const char *user_name, const char *bot_name,
                     bool show_edit_hints, bool move_mode, InPlaceEdit *edit) {
  if (!user_name || !user_name[0])
    user_name = "You";
  if (!bot_name || !bot_name[0])
    bot_name = "Bot";

  werase(chat_win);
  draw_rounded_box(chat_win, COLOR_PAIR_BORDER, true);

  int height, width;
  getmaxyx(chat_win, height, width);

  char title[128];
  if (model_name && model_name[0]) {
    snprintf(title, sizeof(title), "✦ %s", model_name);
  } else {
    snprintf(title, sizeof(title), "✦ sillytui");
  }
  draw_title(chat_win, title, COLOR_PAIR_TITLE);

  if (edit && edit->active) {
    if (g_ui_colors)
      wattron(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    mvwaddstr(chat_win, height - 1, 3, " Enter:save  Esc:cancel ");
    if (g_ui_colors)
      wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  } else if (move_mode && selected_msg >= 0) {
    if (g_ui_colors)
      wattron(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    mvwaddstr(chat_win, height - 1, 3, " ↑/↓:move  Enter/Esc:done ");
    if (g_ui_colors)
      wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  } else if (show_edit_hints && selected_msg >= 0) {
    if (g_ui_colors)
      wattron(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    mvwaddstr(chat_win, height - 1, 3,
              " e:edit  d:delete  m:move  Enter:deselect ");
    if (g_ui_colors)
      wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  }

  int usable_lines = height - 2;
  int content_width = width - 4;
  if (usable_lines <= 0 || content_width <= 0) {
    wrefresh(chat_win);
    return;
  }

  int max_display = (int)history->count * 100 + 100;
  DisplayLine *all_lines = malloc(sizeof(DisplayLine) * max_display);
  if (!all_lines) {
    wrefresh(chat_win);
    return;
  }

  char *subst_strs[500];
  int subst_count = 0;
  int total_display_lines =
      build_display_lines(history, content_width, all_lines, max_display, edit,
                          bot_name, user_name, subst_strs, &subst_count);

  int max_scroll = total_display_lines - usable_lines;
  if (max_scroll < 0)
    max_scroll = 0;

  int scroll_offset;
  if (selected_msg >= 0 && selected_msg < (int)history->count) {
    scroll_offset =
        ui_get_msg_scroll_offset(chat_win, history, selected_msg, edit);
    if (scroll_offset < 0)
      scroll_offset = max_scroll;
  } else {
    scroll_offset = max_scroll;
  }

  int start_line = scroll_offset;
  if (start_line > max_scroll)
    start_line = max_scroll;
  if (start_line < 0)
    start_line = 0;

  int gutter_width = 3;
  int text_width = content_width - gutter_width;
  if (text_width < 10)
    text_width = 10;

  for (int row = 0;
       row < usable_lines && (start_line + row) < total_display_lines; row++) {
    DisplayLine *dl = &all_lines[start_line + row];
    int y = row + 1;
    int x = 2;

    if (dl->is_spacer)
      continue;

    bool is_selected = (dl->msg_index == selected_msg);
    int bg_color = is_selected ? 236 : -1;

    if (is_selected && g_ui_colors) {
      wattron(chat_win, COLOR_PAIR(COLOR_PAIR_MSG_SELECTED));
      mvwhline(chat_win, y, 1, ' ', width - 2);
      wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_MSG_SELECTED));
    }

    if (dl->is_name_line) {
      if (dl->is_user) {
        int pair = is_selected ? COLOR_PAIR_USER_SEL : COLOR_PAIR_USER;
        if (g_ui_colors)
          wattron(chat_win, COLOR_PAIR(pair) | A_BOLD);
        mvwaddstr(chat_win, y, x, "▸ ");
        mvwaddstr(chat_win, y, x + 2, user_name);
        if (g_ui_colors)
          wattroff(chat_win, COLOR_PAIR(pair) | A_BOLD);

        int user_tokens = history_get_token_count(history, dl->msg_index, 0);
        if (user_tokens > 0) {
          char tok_str[32];
          snprintf(tok_str, sizeof(tok_str), " %d tok", user_tokens);
          int hint_pair = is_selected ? COLOR_PAIR_HINT_SEL : COLOR_PAIR_HINT;
          if (g_ui_colors)
            wattron(chat_win, COLOR_PAIR(hint_pair) | A_DIM);
          mvwaddstr(chat_win, y, x + 2 + (int)strlen(user_name), tok_str);
          if (g_ui_colors)
            wattroff(chat_win, COLOR_PAIR(hint_pair) | A_DIM);
        }
      } else if (dl->is_system) {
        int pair = is_selected ? COLOR_PAIR_SYSTEM_SEL : COLOR_PAIR_SYSTEM;
        if (g_ui_colors)
          wattron(chat_win, COLOR_PAIR(pair) | A_BOLD);
        mvwaddstr(chat_win, y, x, "⚙ ");
        mvwaddstr(chat_win, y, x + 2, "System");
        if (g_ui_colors)
          wattroff(chat_win, COLOR_PAIR(pair) | A_BOLD);
      } else if (dl->is_bot) {
        int pair = is_selected ? COLOR_PAIR_BOT_SEL : COLOR_PAIR_BOT;
        if (g_ui_colors)
          wattron(chat_win, COLOR_PAIR(pair) | A_BOLD);
        mvwaddstr(chat_win, y, x, "◆ ");
        mvwaddstr(chat_win, y, x + 2, bot_name);
        if (g_ui_colors)
          wattroff(chat_win, COLOR_PAIR(pair) | A_BOLD);

        size_t swipe_count = history_get_swipe_count(history, dl->msg_index);
        size_t active = history_get_active_swipe(history, dl->msg_index);
        int name_len = (int)strlen(bot_name);
        int cursor_x = x + 2 + name_len;

        if (swipe_count > 1) {
          char swipe_str[32];
          snprintf(swipe_str, sizeof(swipe_str), " ◀ %zu/%zu ▶", active + 1,
                   swipe_count);
          int swipe_pair =
              is_selected ? COLOR_PAIR_SWIPE_SEL : COLOR_PAIR_SWIPE;
          if (g_ui_colors)
            wattron(chat_win, COLOR_PAIR(swipe_pair));
          mvwaddstr(chat_win, y, cursor_x, swipe_str);
          if (g_ui_colors)
            wattroff(chat_win, COLOR_PAIR(swipe_pair));
          cursor_x += (int)strlen(swipe_str);
        }

        int tokens = history_get_token_count(history, dl->msg_index, active);
        double gen_time = history_get_gen_time(history, dl->msg_index, active);
        double output_tps =
            history_get_output_tps(history, dl->msg_index, active);
        if (tokens > 0 || gen_time > 0 || output_tps > 0) {
          char stats_str[96];
          int pos = 0;

          if (tokens > 0) {
            pos += snprintf(stats_str + pos, sizeof(stats_str) - pos, " %d tok",
                            tokens);
          }

          if (output_tps > 0) {
            pos += snprintf(stats_str + pos, sizeof(stats_str) - pos,
                            " (%.1f t/s)", output_tps);
          }

          if (gen_time > 0) {
            if (pos > 0)
              pos += snprintf(stats_str + pos, sizeof(stats_str) - pos, " │");
            if (gen_time >= 1000) {
              pos += snprintf(stats_str + pos, sizeof(stats_str) - pos,
                              " %.1fs", gen_time / 1000.0);
            } else {
              pos += snprintf(stats_str + pos, sizeof(stats_str) - pos,
                              " %.0fms", gen_time);
            }
          }

          int hint_pair = is_selected ? COLOR_PAIR_HINT_SEL : COLOR_PAIR_HINT;
          if (g_ui_colors)
            wattron(chat_win, COLOR_PAIR(hint_pair) | A_DIM);
          mvwaddstr(chat_win, y, cursor_x, stats_str);
          if (g_ui_colors)
            wattroff(chat_win, COLOR_PAIR(hint_pair) | A_DIM);
        }
      }
      continue;
    }

    if (dl->is_reasoning_indicator) {
      bool expanded = ui_is_reasoning_expanded(dl->msg_index);
      int pair = is_selected ? COLOR_PAIR_BOT_SEL : COLOR_PAIR_BOT;
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(pair) | A_DIM);
      mvwaddstr(chat_win, y, x, "│");
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(pair) | A_DIM);

      char reasoning_str[128];
      double secs = dl->reasoning_time_ms / 1000.0;
      const char *arrow = expanded ? "▼" : "▶";
      snprintf(reasoning_str, sizeof(reasoning_str), " %s 💭 Thought for %.1fs",
               arrow, secs);

      int hint_pair = is_selected ? COLOR_PAIR_HINT_SEL : COLOR_PAIR_HINT;
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(hint_pair) | A_DIM);
      mvwaddstr(chat_win, y, x + gutter_width, reasoning_str);
      if (is_selected && g_ui_colors) {
        wattron(chat_win, COLOR_PAIR(hint_pair));
        mvwaddstr(chat_win, y, x + gutter_width + (int)strlen(reasoning_str),
                  " [t]");
        wattroff(chat_win, COLOR_PAIR(hint_pair));
      }
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(hint_pair) | A_DIM);
      continue;
    }

    if (dl->reasoning_time_ms < 0) {
      int pair = is_selected ? COLOR_PAIR_HINT_SEL : COLOR_PAIR_HINT;
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(pair) | A_DIM);
      mvwaddstr(chat_win, y, x, "┊");
      mvwaddstr(chat_win, y, x + 1, "  ");
      mvwaddnstr(chat_win, y, x + gutter_width, dl->line_start, dl->line_len);
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(pair) | A_DIM);
      continue;
    }

    if (dl->is_user) {
      int pair = is_selected ? COLOR_PAIR_USER_SEL : COLOR_PAIR_USER;
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(pair) | A_DIM);
      mvwaddstr(chat_win, y, x, "│");
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(pair) | A_DIM);
    } else if (dl->is_system) {
      int pair = is_selected ? COLOR_PAIR_SYSTEM_SEL : COLOR_PAIR_SYSTEM;
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(pair) | A_DIM);
      mvwaddstr(chat_win, y, x, "┊");
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(pair) | A_DIM);
    } else if (dl->is_bot) {
      int pair = is_selected ? COLOR_PAIR_BOT_SEL : COLOR_PAIR_BOT;
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(pair) | A_DIM);
      mvwaddstr(chat_win, y, x, "│");
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(pair) | A_DIM);
    }

    bool is_editing_this_msg = edit && edit->active &&
                               dl->msg_index == edit->msg_index &&
                               !dl->is_name_line;

    if (is_editing_this_msg) {
      int edit_line_in_msg = 0;
      int cursor_line = 0, cursor_col = 0;
      int col = 0;
      for (int i = 0; i < edit->cursor_pos && i < edit->buf_len; i++) {
        if (edit->buffer[i] == '\n') {
          cursor_line++;
          col = 0;
        } else {
          col++;
          if (col >= text_width) {
            cursor_line++;
            col = 0;
          }
        }
      }
      cursor_col = col;

      col = 0;
      for (int i = 0; i < start_line + row; i++) {
        if (i < total_display_lines &&
            all_lines[i].msg_index == edit->msg_index &&
            !all_lines[i].is_name_line && !all_lines[i].is_spacer) {
          edit_line_in_msg++;
        }
      }

      int line_start_pos = 0;
      int cur_line = 0;
      col = 0;
      for (int i = 0; i <= edit->buf_len; i++) {
        if (cur_line == edit_line_in_msg) {
          line_start_pos = i;
          break;
        }
        if (i < edit->buf_len) {
          if (edit->buffer[i] == '\n') {
            cur_line++;
            col = 0;
          } else {
            col++;
            if (col >= text_width) {
              cur_line++;
              col = 0;
            }
          }
        }
      }

      char line_buf[512];
      int line_len = 0;
      col = 0;
      for (int i = line_start_pos; i < edit->buf_len && line_len < text_width;
           i++) {
        if (edit->buffer[i] == '\n')
          break;
        line_buf[line_len++] = edit->buffer[i];
        col++;
        if (col >= text_width)
          break;
      }
      line_buf[line_len] = '\0';

      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(COLOR_PAIR_MSG_SELECTED));
      mvwaddstr(chat_win, y, x + gutter_width, line_buf);
      for (int pad = line_len; pad < text_width; pad++)
        waddch(chat_win, ' ');
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_MSG_SELECTED));

      if (cursor_line == edit_line_in_msg) {
        int cursor_x = x + gutter_width + cursor_col;
        char ch = (cursor_col < line_len) ? line_buf[cursor_col] : ' ';
        wattron(chat_win, A_REVERSE);
        mvwaddch(chat_win, y, cursor_x, ch);
        wattroff(chat_win, A_REVERSE);
      }
    } else {
      char line_buf[512];
      int to_copy = dl->line_len;
      if (to_copy > (int)sizeof(line_buf) - 1)
        to_copy = (int)sizeof(line_buf) - 1;
      strncpy(line_buf, dl->line_start, to_copy);
      line_buf[to_copy] = '\0';

      markdown_render_line_bg(chat_win, y, x + gutter_width, text_width,
                              line_buf, dl->initial_style, bg_color);
    }
  }

  if (total_display_lines > usable_lines) {
    int scrollbar_height = usable_lines;
    if (scrollbar_height > 2) {
      int thumb_size = (usable_lines * scrollbar_height) / total_display_lines;
      if (thumb_size < 1)
        thumb_size = 1;
      if (thumb_size > scrollbar_height - 2)
        thumb_size = scrollbar_height - 2;
      int thumb_pos =
          (max_scroll > 0)
              ? 1 + (start_line * (scrollbar_height - thumb_size - 2)) /
                        max_scroll
              : 1;

      for (int i = 1; i < scrollbar_height - 1; i++) {
        if (i >= thumb_pos && i < thumb_pos + thumb_size) {
          if (g_ui_colors)
            wattron(chat_win, COLOR_PAIR(COLOR_PAIR_BORDER));
          mvwaddstr(chat_win, i, width - 2, "█");
          if (g_ui_colors)
            wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_BORDER));
        } else {
          if (g_ui_colors)
            wattron(chat_win, COLOR_PAIR(COLOR_PAIR_BORDER_DIM));
          mvwaddstr(chat_win, i, width - 2, "░");
          if (g_ui_colors)
            wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_BORDER_DIM));
        }
      }
    }
  }

  for (int i = 0; i < subst_count; i++) {
    free(subst_strs[i]);
  }
  free(all_lines);
  wrefresh(chat_win);
}

int ui_get_input_line_count(const char *buffer, int text_width) {
  if (!buffer || buffer[0] == '\0')
    return 1;

  int lines = 1;
  int col = 0;
  for (const char *p = buffer; *p; p++) {
    if (*p == '\n') {
      lines++;
      col = 0;
    } else {
      col++;
      if (col >= text_width) {
        lines++;
        col = 0;
      }
    }
  }
  return lines;
}

InputCursorPos ui_cursor_to_line_col(const char *buffer, int cursor_pos,
                                     int text_width) {
  InputCursorPos pos = {0, 0};
  if (!buffer)
    return pos;

  int line = 0;
  int col = 0;
  for (int i = 0; i < cursor_pos && buffer[i]; i++) {
    if (buffer[i] == '\n') {
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
  pos.line = line;
  pos.col = col;
  return pos;
}

int ui_line_col_to_cursor(const char *buffer, int target_line, int target_col,
                          int text_width) {
  if (!buffer)
    return 0;

  int line = 0;
  int col = 0;
  int cursor = 0;

  for (int i = 0; buffer[i]; i++) {
    if (line == target_line && col == target_col)
      return i;

    if (line > target_line)
      return cursor;

    if (buffer[i] == '\n') {
      if (line == target_line)
        return i;
      line++;
      col = 0;
      cursor = i + 1;
    } else {
      col++;
      if (col >= text_width) {
        if (line == target_line && col > target_col)
          return i;
        line++;
        col = 0;
        cursor = i + 1;
      }
    }
  }

  return (int)strlen(buffer);
}

void ui_draw_input_multiline_ex(WINDOW *input_win, const char *buffer,
                                int cursor_pos, bool focused, int scroll_line,
                                bool editing_mode,
                                const AttachmentList *attachments) {
  werase(input_win);
  draw_rounded_box(input_win, COLOR_PAIR_BORDER, focused);

  int h, w;
  getmaxyx(input_win, h, w);
  int buf_len = (int)strlen(buffer);
  int text_width = w - 6;
  if (text_width < 10)
    text_width = 10;

  int attach_height = ui_attachment_bar_height(attachments);
  int visible_lines = h - 2 - attach_height;
  if (visible_lines < 1)
    visible_lines = 1;

  if (attach_height > 0) {
    ui_draw_attachments(input_win, attachments, 1);
  }

  if (editing_mode) {
    if (g_ui_colors)
      wattron(input_win, COLOR_PAIR(COLOR_PAIR_LOADING) | A_BOLD);
    mvwaddstr(input_win, 0, 2, " Editing ");
    if (g_ui_colors)
      wattroff(input_win, COLOR_PAIR(COLOR_PAIR_LOADING) | A_BOLD);
    if (g_ui_colors)
      wattron(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    int hint_x = w - 22;
    if (hint_x > 10)
      mvwaddstr(input_win, 0, hint_x, " Enter:save  Esc:cancel ");
    if (g_ui_colors)
      wattroff(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  }

  int input_start_y = 1 + attach_height;

  if (focused) {
    if (g_ui_colors)
      wattron(input_win, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
    mvwaddstr(input_win, input_start_y, 2, "›");
    if (g_ui_colors)
      wattroff(input_win, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
  } else {
    if (g_ui_colors)
      wattron(input_win, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwaddstr(input_win, input_start_y, 2, "›");
    if (g_ui_colors)
      wattroff(input_win, COLOR_PAIR(COLOR_PAIR_HINT));
  }

  if (buffer[0] == '\0') {
    if (g_ui_colors) {
      wattron(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwaddstr(input_win, input_start_y, 4,
                "Type a message or / for commands...");
      wattroff(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    }
    if (focused) {
      wattron(input_win, A_REVERSE);
      mvwaddch(input_win, input_start_y, 4, ' ');
      wattroff(input_win, A_REVERSE);
    }
    wrefresh(input_win);
    return;
  }

  int total_lines = ui_get_input_line_count(buffer, text_width);
  InputCursorPos cursor_lc =
      ui_cursor_to_line_col(buffer, cursor_pos, text_width);

  if (scroll_line > cursor_lc.line)
    scroll_line = cursor_lc.line;
  if (scroll_line < cursor_lc.line - visible_lines + 1)
    scroll_line = cursor_lc.line - visible_lines + 1;
  if (scroll_line < 0)
    scroll_line = 0;

  int line = 0;
  int col = 0;
  int screen_line = 0;
  const char *p = buffer;
  int char_idx = 0;

  while (line < scroll_line && *p) {
    if (*p == '\n') {
      line++;
      col = 0;
    } else {
      col++;
      if (col >= text_width) {
        line++;
        col = 0;
      }
    }
    p++;
    char_idx++;
  }

  col = 0;
  while (*p && screen_line < visible_lines) {
    int y = input_start_y + screen_line;
    int x = 4 + col;

    if (focused && char_idx == cursor_pos) {
      wattron(input_win, A_REVERSE);
      mvwaddch(input_win, y, x, *p == '\n' ? ' ' : *p);
      wattroff(input_win, A_REVERSE);
    } else if (*p != '\n') {
      mvwaddch(input_win, y, x, *p);
    }

    if (*p == '\n') {
      screen_line++;
      col = 0;
    } else {
      col++;
      if (col >= text_width) {
        screen_line++;
        col = 0;
      }
    }
    p++;
    char_idx++;
  }

  if (focused && cursor_pos >= buf_len) {
    int cursor_screen_line = cursor_lc.line - scroll_line;
    if (cursor_screen_line >= 0 && cursor_screen_line < visible_lines) {
      wattron(input_win, A_REVERSE);
      mvwaddch(input_win, input_start_y + cursor_screen_line, 4 + cursor_lc.col,
               ' ');
      wattroff(input_win, A_REVERSE);
    }
  }

  if (total_lines > visible_lines) {
    if (g_ui_colors)
      wattron(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    if (scroll_line > 0)
      mvwaddstr(input_win, 0, w / 2, "▲");
    if (scroll_line + visible_lines < total_lines)
      mvwaddstr(input_win, h - 1, w / 2, "▼");
    if (g_ui_colors)
      wattroff(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  }

  if (focused && !editing_mode) {
    if (g_ui_colors)
      wattron(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
#ifdef __APPLE__
    const char *newline_hint = " ⌥+Enter:newline ";
#else
    const char *newline_hint = " Alt+Enter:newline ";
#endif
    int hint_len = (int)strlen(newline_hint);
    mvwaddstr(input_win, h - 1, w - hint_len - 2, newline_hint);
    if (g_ui_colors)
      wattroff(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  }

  wrefresh(input_win);
}

#define MAX_SUGGESTIONS 10
#define SUGGESTION_BOX_WIDTH 60

static bool fuzzy_match(const char *pattern, const char *str) {
  if (!pattern || !*pattern)
    return true;
  const char *p = pattern;
  const char *s = str;
  while (*p && *s) {
    if (tolower((unsigned char)*p) == tolower((unsigned char)*s))
      p++;
    s++;
  }
  return *p == '\0';
}

void suggestion_box_init(SuggestionBox *sb, const SlashCommand *commands,
                         int count) {
  memset(sb, 0, sizeof(*sb));
  sb->commands = commands;
  sb->command_count = count;
  sb->matched_indices = malloc(sizeof(int) * count);
  sb->matched_count = 0;
  sb->active_index = 0;
  sb->dynamic_items = malloc(sizeof(DynamicItem) * MAX_DYNAMIC_ITEMS);
  sb->dynamic_count = 0;
  sb->showing_dynamic = false;
  sb->showing_characters = false;
  sb->current_character[0] = '\0';
}

void suggestion_box_free(SuggestionBox *sb) {
  if (sb->matched_indices) {
    free(sb->matched_indices);
    sb->matched_indices = NULL;
  }
  if (sb->dynamic_items) {
    free(sb->dynamic_items);
    sb->dynamic_items = NULL;
  }
  suggestion_box_close(sb);
}

static bool command_matches_filter(const char *cmd_name, const char *query) {
  size_t query_len = strlen(query);
  size_t cmd_len = strlen(cmd_name);

  if (query_len == 0)
    return true;

  if (strncmp(cmd_name, query, query_len) == 0)
    return true;

  if (query_len <= cmd_len && fuzzy_match(query, cmd_name))
    return true;

  return false;
}

static void load_character_suggestions(SuggestionBox *sb,
                                       const char *filter_text) {
  sb->dynamic_count = 0;

  ChatCharacterList list;
  chat_character_list_init(&list);
  if (!chat_character_list_load(&list)) {
    chat_character_list_free(&list);
    return;
  }

  for (size_t i = 0; i < list.count && sb->dynamic_count < MAX_DYNAMIC_ITEMS;
       i++) {
    bool matches = true;
    if (filter_text && filter_text[0]) {
      // Case-insensitive match on character name or dirname
      char lower_filter[128], lower_name[128], lower_dir[128];
      size_t j;
      for (j = 0; filter_text[j] && j < 127; j++)
        lower_filter[j] = tolower((unsigned char)filter_text[j]);
      lower_filter[j] = '\0';
      for (j = 0; list.characters[i].name[j] && j < 127; j++)
        lower_name[j] = tolower((unsigned char)list.characters[i].name[j]);
      lower_name[j] = '\0';
      for (j = 0; list.characters[i].dirname[j] && j < 127; j++)
        lower_dir[j] = tolower((unsigned char)list.characters[i].dirname[j]);
      lower_dir[j] = '\0';

      matches = (strstr(lower_name, lower_filter) != NULL ||
                 strstr(lower_dir, lower_filter) != NULL);
    }
    if (matches) {
      snprintf(sb->dynamic_items[sb->dynamic_count].name, 128, "%s",
               list.characters[i].name);
      snprintf(sb->dynamic_items[sb->dynamic_count].description, 128,
               "%zu chat%s", list.characters[i].chat_count,
               list.characters[i].chat_count == 1 ? "" : "s");
      snprintf(sb->dynamic_items[sb->dynamic_count].id, 128, "%s",
               list.characters[i].dirname);
      sb->dynamic_count++;
    }
  }

  chat_character_list_free(&list);
}

static void load_chat_suggestions_for_character(SuggestionBox *sb,
                                                const char *character_name,
                                                const char *filter_text) {
  sb->dynamic_count = 0;

  ChatList list;
  chat_list_init(&list);
  if (!chat_list_load_for_character(&list, character_name)) {
    chat_list_free(&list);
    return;
  }

  for (size_t i = 0; i < list.count && sb->dynamic_count < MAX_DYNAMIC_ITEMS;
       i++) {
    bool matches = true;
    if (filter_text && filter_text[0]) {
      matches = (strstr(list.chats[i].title, filter_text) != NULL ||
                 strstr(list.chats[i].id, filter_text) != NULL);
    }
    if (matches) {
      snprintf(sb->dynamic_items[sb->dynamic_count].name, 128, "%s",
               list.chats[i].title);
      snprintf(sb->dynamic_items[sb->dynamic_count].description, 128,
               "%zu msgs", list.chats[i].message_count);
      snprintf(sb->dynamic_items[sb->dynamic_count].id, 128, "%s",
               list.chats[i].id);
      sb->dynamic_count++;
    }
  }

  chat_list_free(&list);
}

bool suggestion_box_update(SuggestionBox *sb, const char *filter,
                           WINDOW *parent, int parent_y, int parent_x) {
  (void)parent; // Reserved for future use
  if (!filter || filter[0] != '/') {
    bool was_open = sb->win != NULL;
    suggestion_box_close(sb);
    return was_open;
  }

  const char *query = filter + 1;
  size_t query_len = strlen(query);

  const char *chat_load_prefix = "chat load ";
  size_t prefix_len = strlen(chat_load_prefix);

  if (strncmp(query, chat_load_prefix, prefix_len) == 0) {
    sb->showing_dynamic = true;
    const char *after_prefix = query + prefix_len;

    const char *space_pos = strchr(after_prefix, ' ');
    if (space_pos && space_pos > after_prefix) {
      size_t char_name_len = space_pos - after_prefix;
      char character_name[128];
      if (char_name_len < sizeof(character_name)) {
        strncpy(character_name, after_prefix, char_name_len);
        character_name[char_name_len] = '\0';

        sb->showing_characters = false;
        strncpy(sb->current_character, character_name,
                sizeof(sb->current_character) - 1);
        sb->current_character[sizeof(sb->current_character) - 1] = '\0';

        const char *chat_filter = space_pos + 1;
        load_chat_suggestions_for_character(sb, character_name, chat_filter);
      }
    } else {
      sb->showing_characters = true;
      sb->current_character[0] = '\0';
      load_character_suggestions(sb, after_prefix);
    }

    if (sb->dynamic_count == 0) {
      bool was_open = sb->win != NULL;
      suggestion_box_close(sb);
      return was_open;
    }

    sb->matched_count = sb->dynamic_count;
    if (sb->active_index >= sb->matched_count)
      sb->active_index = sb->matched_count - 1;
  } else {
    sb->showing_dynamic = false;

    bool has_trailing_space = query_len > 0 && query[query_len - 1] == ' ';
    bool exact_match_exists = false;

    for (int i = 0; i < sb->command_count; i++) {
      if (strcmp(sb->commands[i].name, query) == 0) {
        exact_match_exists = true;
        break;
      }
    }

    if (exact_match_exists && has_trailing_space &&
        strcmp(query, "chat load ") != 0) {
      bool was_open = sb->win != NULL;
      suggestion_box_close(sb);
      return was_open;
    }

    sb->matched_count = 0;
    for (int i = 0; i < sb->command_count; i++) {
      if (command_matches_filter(sb->commands[i].name, query))
        sb->matched_indices[sb->matched_count++] = i;
    }

    if (sb->matched_count == 0) {
      bool was_open = sb->win != NULL;
      suggestion_box_close(sb);
      return was_open;
    }

    if (sb->active_index >= sb->matched_count)
      sb->active_index = sb->matched_count - 1;
  }

  int box_height = sb->matched_count + 2;
  if (box_height > MAX_SUGGESTIONS + 2)
    box_height = MAX_SUGGESTIONS + 2;
  sb->visible_count = box_height - 2;

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  (void)max_y;

  int box_width = SUGGESTION_BOX_WIDTH;
  if (box_width > max_x - 4)
    box_width = max_x - 4;

  int box_y = parent_y - box_height;
  int box_x = parent_x + 2;
  if (box_y < 0)
    box_y = 0;
  if (box_x + box_width > max_x)
    box_x = max_x - box_width;

  bool needs_redraw = false;
  if (sb->win) {
    int old_h, old_w, old_y, old_x;
    getmaxyx(sb->win, old_h, old_w);
    getbegyx(sb->win, old_y, old_x);
    if (old_h != box_height || old_w != box_width || old_y != box_y ||
        old_x != box_x) {
      if (old_h > box_height || old_w > box_width || old_y < box_y) {
        needs_redraw = true;
      }
      werase(sb->win);
      wrefresh(sb->win);
      delwin(sb->win);
      sb->win = NULL;
    }
  }

  if (!sb->win) {
    sb->win = newwin(box_height, box_width, box_y, box_x);
  }

  sb->filter = filter;
  return needs_redraw;
}

void suggestion_box_draw(SuggestionBox *sb) {
  if (!sb->win || sb->matched_count == 0)
    return;

  werase(sb->win);

  int h, w;
  getmaxyx(sb->win, h, w);

  if (g_ui_colors)
    wattron(sb->win, COLOR_PAIR(COLOR_PAIR_BORDER));
  mvwaddstr(sb->win, 0, 0, "╭");
  for (int x = 1; x < w - 1; x++)
    mvwaddstr(sb->win, 0, x, "─");
  mvwaddstr(sb->win, 0, w - 1, "╮");
  for (int y = 1; y < h - 1; y++) {
    mvwaddstr(sb->win, y, 0, "│");
    mvwaddstr(sb->win, y, w - 1, "│");
  }
  mvwaddstr(sb->win, h - 1, 0, "╰");
  for (int x = 1; x < w - 1; x++)
    mvwaddstr(sb->win, h - 1, x, "─");
  mvwaddstr(sb->win, h - 1, w - 1, "╯");
  if (g_ui_colors)
    wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_BORDER));

  if (sb->scroll_offset > 0) {
    if (g_ui_colors)
      wattron(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwaddstr(sb->win, 0, w / 2, "▲");
    if (g_ui_colors)
      wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
  }
  if (sb->scroll_offset + sb->visible_count < sb->matched_count) {
    if (g_ui_colors)
      wattron(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwaddstr(sb->win, h - 1, w / 2, "▼");
    if (g_ui_colors)
      wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
  }

  int name_col_width = 18;

  for (int i = 0;
       i < sb->visible_count && (sb->scroll_offset + i) < sb->matched_count;
       i++) {
    int y = i + 1;
    bool is_active = (sb->scroll_offset + i) == sb->active_index;
    const char *name = NULL;
    const char *desc = NULL;

    if (sb->showing_dynamic) {
      int idx = sb->scroll_offset + i;
      if (idx < sb->dynamic_count) {
        name = sb->dynamic_items[idx].name;
        desc = sb->dynamic_items[idx].description;
      }
    } else {
      int cmd_idx = sb->matched_indices[sb->scroll_offset + i];
      name = sb->commands[cmd_idx].name;
      desc = sb->commands[cmd_idx].description;
    }

    if (!name)
      continue;

    if (is_active) {
      wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
      mvwhline(sb->win, y, 1, ' ', w - 2);
    }

    if (sb->showing_dynamic) {
      int name_max = w - 4;
      if (name_max > 40)
        name_max = 40;
      mvwprintw(sb->win, y, 2, "%-*.*s", name_max, name_max, name);

      if (is_active)
        wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));

      if (desc) {
        int desc_x = 2 + name_max + 2;
        int desc_max = w - desc_x - 2;
        if (desc_max > 0) {
          if (is_active) {
            wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
          } else if (g_ui_colors) {
            wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_DESC));
          }
          mvwprintw(sb->win, y, desc_x, "%.*s", desc_max, desc);
          if (is_active) {
            wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
          } else if (g_ui_colors) {
            wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_DESC));
          }
        }
      }
    } else {
      mvwprintw(sb->win, y, 2, "/%-*s", name_col_width - 1, name);

      if (is_active)
        wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));

      if (desc) {
        int desc_x = 2 + name_col_width + 1;
        int desc_max = w - desc_x - 2;
        if (desc_max > 0) {
          if (is_active) {
            wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
          } else if (g_ui_colors) {
            wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_DESC));
          }
          mvwprintw(sb->win, y, desc_x, "%.*s", desc_max, desc);
          if (is_active) {
            wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
          } else if (g_ui_colors) {
            wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_DESC));
          }
        }
      }
    }
  }

  if (sb->showing_dynamic && sb->showing_characters) {
    const char *hint = " Tab:chats  Enter:latest ";
    int hint_len = (int)strlen(hint);
    int hint_x = (w - hint_len) / 2;
    if (hint_x > 1 && hint_x + hint_len < w - 1) {
      if (g_ui_colors)
        wattron(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
      mvwprintw(sb->win, h - 1, hint_x, "%s", hint);
      if (g_ui_colors)
        wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
    }
  }

  wrefresh(sb->win);
}

void suggestion_box_navigate(SuggestionBox *sb, int direction) {
  if (sb->matched_count == 0)
    return;

  sb->active_index += direction;
  if (sb->active_index < 0) {
    sb->active_index = sb->matched_count - 1;
  } else if (sb->active_index >= sb->matched_count) {
    sb->active_index = 0;
  }

  if (sb->active_index < sb->scroll_offset) {
    sb->scroll_offset = sb->active_index;
  } else if (sb->active_index >= sb->scroll_offset + sb->visible_count) {
    sb->scroll_offset = sb->active_index - sb->visible_count + 1;
  }
}

const char *suggestion_box_get_selected(SuggestionBox *sb) {
  if (sb->matched_count == 0 || sb->active_index < 0)
    return NULL;
  if (sb->showing_dynamic) {
    if (sb->active_index < sb->dynamic_count)
      return sb->dynamic_items[sb->active_index].name;
    return NULL;
  }
  int cmd_idx = sb->matched_indices[sb->active_index];
  return sb->commands[cmd_idx].name;
}

const char *suggestion_box_get_selected_id(SuggestionBox *sb) {
  if (sb->matched_count == 0 || sb->active_index < 0)
    return NULL;
  if (sb->showing_dynamic && sb->active_index < sb->dynamic_count)
    return sb->dynamic_items[sb->active_index].id;
  return NULL;
}

void suggestion_box_close(SuggestionBox *sb) {
  if (sb->win) {
    werase(sb->win);
    wrefresh(sb->win);
    delwin(sb->win);
    sb->win = NULL;
  }
  sb->matched_count = 0;
  sb->active_index = 0;
  sb->scroll_offset = 0;
  sb->showing_characters = false;
  sb->current_character[0] = '\0';
}

bool suggestion_box_is_open(const SuggestionBox *sb) {
  return sb->win != NULL && sb->matched_count > 0;
}

void attachment_list_init(AttachmentList *list) {
  if (!list)
    return;
  list->count = 0;
  list->selected = -1;
}

bool attachment_list_add(AttachmentList *list, const char *filename,
                         size_t file_size) {
  if (!list || !filename || list->count >= MAX_ATTACHMENTS)
    return false;
  strncpy(list->items[list->count].filename, filename,
          sizeof(list->items[0].filename) - 1);
  list->items[list->count].filename[sizeof(list->items[0].filename) - 1] = '\0';
  list->items[list->count].file_size = file_size;
  list->count++;
  return true;
}

void attachment_list_remove(AttachmentList *list, int index) {
  if (!list || index < 0 || index >= list->count)
    return;
  for (int i = index; i < list->count - 1; i++) {
    list->items[i] = list->items[i + 1];
  }
  list->count--;
  if (list->selected >= list->count)
    list->selected = list->count - 1;
}

void attachment_list_clear(AttachmentList *list) {
  if (!list)
    return;
  list->count = 0;
  list->selected = -1;
}

static void format_file_size(size_t bytes, char *buf, size_t buf_size) {
  if (bytes < 1024) {
    snprintf(buf, buf_size, "%zuB", bytes);
  } else if (bytes < 1024 * 1024) {
    snprintf(buf, buf_size, "%.1fKB", bytes / 1024.0);
  } else {
    snprintf(buf, buf_size, "%.1fMB", bytes / (1024.0 * 1024.0));
  }
}

void ui_draw_attachments(WINDOW *win, const AttachmentList *list, int y) {
  if (!win || !list || list->count == 0)
    return;

  int max_x = getmaxx(win);

  for (int i = 0; i < list->count; i++) {
    int draw_y = y + i;
    bool is_selected = (i == list->selected);

    wmove(win, draw_y, 0);
    wclrtoeol(win);

    if (is_selected)
      wattron(win, A_REVERSE);

    wattron(win, COLOR_PAIR(COLOR_PAIR_BORDER));
    mvwaddstr(win, draw_y, 1, "📎 ");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_BORDER));

    char size_str[16];
    format_file_size(list->items[i].file_size, size_str, sizeof(size_str));

    int name_max = max_x - 20;
    if (name_max < 10)
      name_max = 10;

    char display_name[128];
    const char *fname = list->items[i].filename;
    if ((int)strlen(fname) > name_max) {
      snprintf(display_name, sizeof(display_name), "%.*s...", name_max - 3,
               fname);
    } else {
      strncpy(display_name, fname, sizeof(display_name) - 1);
      display_name[sizeof(display_name) - 1] = '\0';
    }

    wattron(win, COLOR_PAIR(COLOR_PAIR_USER));
    wprintw(win, "%s", display_name);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_USER));

    wattron(win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    wprintw(win, " (%s)", size_str);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

    int cur_x = getcurx(win);
    if (cur_x < max_x - 4) {
      wmove(win, draw_y, max_x - 3);
      if (is_selected) {
        wattron(win, COLOR_PAIR(COLOR_PAIR_BOT) | A_BOLD);
        waddstr(win, "×");
        wattroff(win, COLOR_PAIR(COLOR_PAIR_BOT) | A_BOLD);
      } else {
        wattron(win, A_DIM);
        waddstr(win, "×");
        wattroff(win, A_DIM);
      }
    }

    if (is_selected)
      wattroff(win, A_REVERSE);
  }
}

int ui_attachment_bar_height(const AttachmentList *list) {
  if (!list || list->count == 0)
    return 0;
  return list->count;
}

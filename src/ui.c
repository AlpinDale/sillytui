#include "ui.h"
#include "markdown.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool g_ui_colors = false;

void ui_init_colors(void) {
  if (!has_colors()) {
    g_ui_colors = false;
    return;
  }
  init_pair(COLOR_PAIR_BORDER, COLOR_MAGENTA, -1);
  init_pair(COLOR_PAIR_TITLE, COLOR_MAGENTA, -1);
  init_pair(COLOR_PAIR_PROMPT, COLOR_GREEN, -1);
  init_pair(COLOR_PAIR_USER, COLOR_GREEN, -1);
  init_pair(COLOR_PAIR_BOT, COLOR_MAGENTA, -1);
  init_pair(COLOR_PAIR_HINT, 8, -1);
  init_pair(COLOR_PAIR_LOADING, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_SUGGEST_ACTIVE, COLOR_BLACK, COLOR_CYAN);
  init_pair(COLOR_PAIR_SUGGEST_DESC, COLOR_CYAN, -1);
  g_ui_colors = true;
}

static void draw_box(WINDOW *win, int color_pair) {
  int h, w;
  getmaxyx(win, h, w);
  if (h < 2 || w < 2)
    return;

  if (g_ui_colors)
    wattron(win, COLOR_PAIR(color_pair));

  mvwaddch(win, 0, 0, ACS_ULCORNER);
  mvwhline(win, 0, 1, ACS_HLINE, w - 2);
  mvwaddch(win, 0, w - 1, ACS_URCORNER);

  for (int y = 1; y < h - 1; y++) {
    mvwaddch(win, y, 0, ACS_VLINE);
    mvwaddch(win, y, w - 1, ACS_VLINE);
  }

  mvwaddch(win, h - 1, 0, ACS_LLCORNER);
  mvwhline(win, h - 1, 1, ACS_HLINE, w - 2);
  mvwaddch(win, h - 1, w - 1, ACS_LRCORNER);

  if (g_ui_colors)
    wattroff(win, COLOR_PAIR(color_pair));
}

static void draw_title(WINDOW *win, const char *title, int color_pair) {
  int w;
  w = getmaxx(win);
  int title_len = (int)strlen(title);
  int pos = (w - title_len - 4) / 2;
  if (pos < 1)
    pos = 1;

  if (g_ui_colors)
    wattron(win, COLOR_PAIR(color_pair) | A_BOLD);
  mvwprintw(win, 0, pos, " %s ", title);
  if (g_ui_colors)
    wattroff(win, COLOR_PAIR(color_pair) | A_BOLD);
}

void ui_layout_windows(WINDOW **chat_win, WINDOW **input_win) {
  int rows, cols;
  getmaxyx(stdscr, rows, cols);
  int input_height = 3;
  if (rows < 6) {
    input_height = 1;
  }
  int chat_height = rows - input_height - 1;
  if (chat_height < 3) {
    chat_height = 3;
  }

  if (*chat_win) {
    delwin(*chat_win);
  }
  if (*input_win) {
    delwin(*input_win);
  }

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
      if (*p == ' ') {
        last_break = p;
      }
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
      if (*p == ' ') {
        last_break = p;
      }
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
  bool is_first_line;
  bool is_spacer;
  bool is_name_line;
  unsigned initial_style;
} DisplayLine;

static int build_display_lines(const ChatHistory *history, int content_width,
                               DisplayLine *lines, int max_lines) {
  int total = 0;
  int gutter_width = 2;
  int text_width = content_width - gutter_width;
  if (text_width < 10)
    text_width = 10;

  WrappedLine *wrapped = malloc(sizeof(WrappedLine) * 200);
  if (!wrapped)
    return 0;

  for (size_t i = 0; i < history->count && total < max_lines; i++) {
    const char *msg = history->items[i];
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

    if ((is_user || is_bot) && total < max_lines) {
      lines[total].msg_index = (int)i;
      lines[total].line_in_msg = -1;
      lines[total].line_start = NULL;
      lines[total].line_len = 0;
      lines[total].is_user = is_user;
      lines[total].is_bot = is_bot;
      lines[total].is_first_line = true;
      lines[total].is_spacer = false;
      lines[total].is_name_line = true;
      lines[total].initial_style = 0;
      total++;
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
      lines[total].is_first_line = false;
      lines[total].is_spacer = false;
      lines[total].is_name_line = false;
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
      lines[total].is_first_line = false;
      lines[total].is_spacer = true;
      lines[total].is_name_line = false;
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

  int gutter_width = 2;
  int text_width = content_width - gutter_width;
  if (text_width < 10)
    text_width = 10;

  int total = 0;
  for (size_t i = 0; i < history->count; i++) {
    const char *msg = history->items[i];
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

    if (is_user || is_bot) {
      total++;
    }
    total += count_wrapped_lines(content, text_width);
    if (i < history->count - 1) {
      total++;
    }
  }
  return total;
}

void ui_draw_chat(WINDOW *chat_win, const ChatHistory *history,
                  int scroll_offset, const char *model_name,
                  const char *user_name, const char *bot_name) {
  if (!user_name || !user_name[0])
    user_name = "You";
  if (!bot_name || !bot_name[0])
    bot_name = "Bot";
  werase(chat_win);
  draw_box(chat_win, COLOR_PAIR_BORDER);

  if (model_name && model_name[0]) {
    char title[64];
    snprintf(title, sizeof(title), "~ %s ~", model_name);
    draw_title(chat_win, title, COLOR_PAIR_TITLE);
  } else {
    draw_title(chat_win, "~ sillytui ~", COLOR_PAIR_TITLE);
  }

  int height, width;
  getmaxyx(chat_win, height, width);

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

  int total_display_lines =
      build_display_lines(history, content_width, all_lines, max_display);

  int max_scroll = total_display_lines - usable_lines;
  if (max_scroll < 0)
    max_scroll = 0;

  int start_line;
  if (scroll_offset < 0) {
    start_line = max_scroll;
  } else {
    start_line = scroll_offset;
    if (start_line > max_scroll)
      start_line = max_scroll;
  }

  int gutter_width = 2;
  int text_width = content_width - gutter_width;
  if (text_width < 10)
    text_width = 10;

  for (int row = 0;
       row < usable_lines && (start_line + row) < total_display_lines; row++) {
    DisplayLine *dl = &all_lines[start_line + row];
    int y = row + 1;
    int x = 2;

    if (dl->is_spacer) {
      continue;
    }

    if (dl->is_name_line) {
      if (dl->is_user) {
        if (g_ui_colors)
          wattron(chat_win, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
        mvwaddstr(chat_win, y, x, user_name);
        if (g_ui_colors)
          wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
      } else if (dl->is_bot) {
        if (g_ui_colors)
          wattron(chat_win, COLOR_PAIR(COLOR_PAIR_BOT) | A_BOLD);
        mvwaddstr(chat_win, y, x, bot_name);
        if (g_ui_colors)
          wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_BOT) | A_BOLD);
      }
      continue;
    }

    if (dl->is_user) {
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(COLOR_PAIR_USER));
      mvwaddch(chat_win, y, x, ACS_VLINE);
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_USER));
    } else if (dl->is_bot) {
      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(COLOR_PAIR_BOT));
      mvwaddch(chat_win, y, x, ACS_VLINE);
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_BOT));
    }

    char line_buf[512];
    int to_copy = dl->line_len;
    if (to_copy > (int)sizeof(line_buf) - 1)
      to_copy = (int)sizeof(line_buf) - 1;
    strncpy(line_buf, dl->line_start, to_copy);
    line_buf[to_copy] = '\0';

    markdown_render_line_styled(chat_win, y, x + gutter_width, text_width,
                                line_buf, dl->initial_style);
  }

  if (total_display_lines > usable_lines) {
    int scrollbar_height = usable_lines - 2;
    if (scrollbar_height > 0) {
      int thumb_size = (usable_lines * scrollbar_height) / total_display_lines;
      if (thumb_size < 1)
        thumb_size = 1;
      int thumb_pos =
          (max_scroll > 0)
              ? (start_line * (scrollbar_height - thumb_size)) / max_scroll
              : 0;

      if (g_ui_colors)
        wattron(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      for (int i = 0; i < scrollbar_height; i++) {
        if (i >= thumb_pos && i < thumb_pos + thumb_size) {
          mvwaddch(chat_win, 2 + i, width - 2, ACS_BLOCK);
        } else {
          mvwaddch(chat_win, 2 + i, width - 2, ACS_VLINE);
        }
      }
      if (g_ui_colors)
        wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    }
  }

  free(all_lines);
  wrefresh(chat_win);
}

void ui_draw_input(WINDOW *input_win, const char *buffer, int cursor_pos) {
  werase(input_win);
  draw_box(input_win, COLOR_PAIR_BORDER);

  int w = getmaxx(input_win);
  int buf_len = (int)strlen(buffer);

  if (g_ui_colors)
    wattron(input_win, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
  mvwaddch(input_win, 1, 2, '>');
  if (g_ui_colors)
    wattroff(input_win, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);

  mvwprintw(input_win, 1, 4, "%s", buffer);

  int screen_cursor = 4 + cursor_pos;
  wattron(input_win, A_REVERSE);
  char under_cursor = (cursor_pos < buf_len) ? buffer[cursor_pos] : ' ';
  mvwaddch(input_win, 1, screen_cursor, under_cursor);
  wattroff(input_win, A_REVERSE);

  if (buffer[0] == '\0' && g_ui_colors) {
    wattron(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    mvwaddstr(input_win, 1, 5, "type something cute...");
    wattroff(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  }

  const char *hint = "arrows: scroll";
  int hint_len = (int)strlen(hint);
  if (g_ui_colors)
    wattron(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwaddstr(input_win, 1, w - hint_len - 2, hint);
  if (g_ui_colors)
    wattroff(input_win, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

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
    if (tolower((unsigned char)*p) == tolower((unsigned char)*s)) {
      p++;
    }
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
}

void suggestion_box_update(SuggestionBox *sb, const char *filter,
                           WINDOW *parent, int parent_y, int parent_x) {
  if (!filter || filter[0] != '/') {
    suggestion_box_close(sb);
    return;
  }

  const char *query = filter + 1;

  int space_pos = -1;
  for (int i = 0; query[i]; i++) {
    if (query[i] == ' ') {
      space_pos = i;
      break;
    }
  }

  if (space_pos >= 0) {
    suggestion_box_close(sb);
    return;
  }

  sb->matched_count = 0;
  for (int i = 0; i < sb->command_count; i++) {
    if (fuzzy_match(query, sb->commands[i].name)) {
      sb->matched_indices[sb->matched_count++] = i;
    }
  }

  if (sb->matched_count == 0) {
    suggestion_box_close(sb);
    return;
  }

  if (sb->active_index >= sb->matched_count) {
    sb->active_index = sb->matched_count - 1;
  }

  int box_height = sb->matched_count + 2;
  if (box_height > MAX_SUGGESTIONS + 2) {
    box_height = MAX_SUGGESTIONS + 2;
  }
  sb->visible_count = box_height - 2;

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  (void)max_y;

  int box_width = SUGGESTION_BOX_WIDTH;
  if (box_width > max_x - 4)
    box_width = max_x - 4;

  int box_y = parent_y - box_height;
  int box_x = parent_x + 4;
  if (box_y < 0)
    box_y = 0;
  if (box_x + box_width > max_x)
    box_x = max_x - box_width;

  if (!sb->win) {
    sb->win = newwin(box_height, box_width, box_y, box_x);
  } else {
    wresize(sb->win, box_height, box_width);
    mvwin(sb->win, box_y, box_x);
  }

  sb->filter = filter;
}

void suggestion_box_draw(SuggestionBox *sb) {
  if (!sb->win || sb->matched_count == 0)
    return;

  werase(sb->win);

  int h, w;
  getmaxyx(sb->win, h, w);

  if (g_ui_colors)
    wattron(sb->win, COLOR_PAIR(COLOR_PAIR_BORDER));
  box(sb->win, 0, 0);
  if (g_ui_colors)
    wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_BORDER));

  if (sb->scroll_offset > 0) {
    if (g_ui_colors)
      wattron(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwaddch(sb->win, 0, w / 2, ACS_UARROW);
    if (g_ui_colors)
      wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
  }
  if (sb->scroll_offset + sb->visible_count < sb->matched_count) {
    if (g_ui_colors)
      wattron(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwaddch(sb->win, h - 1, w / 2, ACS_DARROW);
    if (g_ui_colors)
      wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_HINT));
  }

  int name_col_width = 15;

  for (int i = 0;
       i < sb->visible_count && (sb->scroll_offset + i) < sb->matched_count;
       i++) {
    int cmd_idx = sb->matched_indices[sb->scroll_offset + i];
    const SlashCommand *cmd = &sb->commands[cmd_idx];
    int y = i + 1;
    bool is_active = (sb->scroll_offset + i) == sb->active_index;

    if (is_active) {
      wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
      mvwhline(sb->win, y, 1, ' ', w - 2);
    }

    mvwprintw(sb->win, y, 2, "/%-*s", name_col_width - 1, cmd->name);

    if (is_active) {
      wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
    }

    if (cmd->description) {
      int desc_x = 2 + name_col_width + 1;
      int desc_max = w - desc_x - 2;
      if (desc_max > 0) {
        if (is_active) {
          wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
        } else if (g_ui_colors) {
          wattron(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_DESC));
        }
        mvwprintw(sb->win, y, desc_x, "%.*s", desc_max, cmd->description);
        if (is_active) {
          wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_ACTIVE));
        } else if (g_ui_colors) {
          wattroff(sb->win, COLOR_PAIR(COLOR_PAIR_SUGGEST_DESC));
        }
      }
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
  int cmd_idx = sb->matched_indices[sb->active_index];
  return sb->commands[cmd_idx].name;
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
}

bool suggestion_box_is_open(const SuggestionBox *sb) {
  return sb->win != NULL && sb->matched_count > 0;
}

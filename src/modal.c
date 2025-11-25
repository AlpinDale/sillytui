#include "modal.h"
#include "ui.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void modal_init(Modal *m) {
  memset(m, 0, sizeof(*m));
  m->type = MODAL_NONE;
}

static void create_window(Modal *m, int height, int width) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  m->height = height;
  m->width = width;
  if (m->width > max_x - 4)
    m->width = max_x - 4;
  if (m->height > max_y - 4)
    m->height = max_y - 4;

  m->start_y = (max_y - m->height) / 2;
  m->start_x = (max_x - m->width) / 2;

  m->win = newwin(m->height, m->width, m->start_y, m->start_x);
  keypad(m->win, TRUE);
}

void modal_open_model_set(Modal *m) {
  modal_close(m);
  m->type = MODAL_MODEL_SET;
  m->field_index = 0;
  for (int i = 0; i < 4; i++) {
    m->fields[i][0] = '\0';
    m->field_cursor[i] = 0;
    m->field_len[i] = 0;
  }
  create_window(m, 16, 60);
}

void modal_open_model_list(Modal *m, const ModelsFile *mf) {
  modal_close(m);
  m->type = MODAL_MODEL_LIST;
  m->list_selection = (mf->active_index >= 0) ? mf->active_index : 0;
  m->list_scroll = 0;
  int height = (int)mf->count + 8;
  if (height < 10)
    height = 10;
  if (height > 20)
    height = 20;
  create_window(m, height, 50);
}

void modal_open_message(Modal *m, const char *msg, bool is_error) {
  modal_close(m);
  m->type = MODAL_MESSAGE;
  strncpy(m->message, msg, sizeof(m->message) - 1);
  m->message[sizeof(m->message) - 1] = '\0';
  m->message_is_error = is_error;

  int line_count = 1;
  int max_line_len = 0;
  int cur_line_len = 0;
  for (const char *p = msg; *p; p++) {
    if (*p == '\n') {
      line_count++;
      if (cur_line_len > max_line_len)
        max_line_len = cur_line_len;
      cur_line_len = 0;
    } else {
      cur_line_len++;
    }
  }
  if (cur_line_len > max_line_len)
    max_line_len = cur_line_len;

  int width = max_line_len + 22;
  if (width < 45)
    width = 45;
  if (width > 80)
    width = 80;
  int height = line_count + 5;
  if (height < 7)
    height = 7;

  create_window(m, height, width);
}

void modal_open_chat_list(Modal *m) {
  modal_close(m);
  m->type = MODAL_CHAT_LIST;
  m->list_selection = 0;
  m->list_scroll = 0;
  chat_list_init(&m->chat_list);
  chat_list_load(&m->chat_list);

  int height = (int)m->chat_list.count + 8;
  if (height < 12)
    height = 12;
  if (height > 20)
    height = 20;
  create_window(m, height, 60);
}

void modal_open_chat_save(Modal *m, const char *current_id,
                          const char *character_path) {
  modal_close(m);
  m->type = MODAL_CHAT_SAVE;
  m->field_index = 0;
  for (int i = 0; i < 4; i++) {
    m->fields[i][0] = '\0';
    m->field_cursor[i] = 0;
    m->field_len[i] = 0;
  }
  if (current_id && current_id[0]) {
    strncpy(m->current_chat_id, current_id, CHAT_ID_MAX - 1);
  } else {
    m->current_chat_id[0] = '\0';
  }
  if (character_path && character_path[0]) {
    strncpy(m->character_path, character_path, CHAT_CHAR_PATH_MAX - 1);
  } else {
    m->character_path[0] = '\0';
  }
  create_window(m, 10, 50);
}

void modal_open_exit_confirm(Modal *m) {
  modal_close(m);
  m->type = MODAL_EXIT_CONFIRM;
  m->field_index = 1;
  m->exit_dont_ask = false;
  create_window(m, 9, 45);
}

void modal_open_persona_edit(Modal *m, const Persona *persona) {
  modal_close(m);
  m->type = MODAL_PERSONA_EDIT;
  m->field_index = 0;
  for (int i = 0; i < 4; i++) {
    m->fields[i][0] = '\0';
    m->field_cursor[i] = 0;
    m->field_len[i] = 0;
  }
  if (persona) {
    strncpy(m->fields[0], persona->name, sizeof(m->fields[0]) - 1);
    m->field_len[0] = (int)strlen(m->fields[0]);
    m->field_cursor[0] = m->field_len[0];
    strncpy(m->fields[1], persona->description, sizeof(m->fields[1]) - 1);
    m->field_len[1] = (int)strlen(m->fields[1]);
    m->field_cursor[1] = m->field_len[1];
  }
  create_window(m, 12, 60);
}

void modal_open_character_info(Modal *m, const CharacterCard *card) {
  modal_close(m);
  m->type = MODAL_CHARACTER_INFO;
  m->character = card;
  m->list_scroll = 0;
  create_window(m, 18, 70);
}

void modal_open_greeting_select(Modal *m, const CharacterCard *card) {
  modal_close(m);
  m->type = MODAL_GREETING_SELECT;
  m->character = card;
  m->greeting_selection = 0;
  m->list_scroll = 0;
  int total_greetings = 1 + (card ? (int)card->alternate_greetings_count : 0);
  int height = total_greetings + 6;
  if (height < 10)
    height = 10;
  if (height > 18)
    height = 18;
  create_window(m, height, 60);
}

void modal_close(Modal *m) {
  if (m->win) {
    delwin(m->win);
    m->win = NULL;
  }
  if (m->type == MODAL_CHAT_LIST) {
    chat_list_free(&m->chat_list);
  }
  m->type = MODAL_NONE;
}

bool modal_is_open(const Modal *m) { return m->type != MODAL_NONE; }

bool modal_get_exit_dont_ask(const Modal *m) { return m->exit_dont_ask; }

static void draw_box_fancy(WINDOW *win, int h, int w) {
  wattron(win, COLOR_PAIR(COLOR_PAIR_BORDER));
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
  wattroff(win, COLOR_PAIR(COLOR_PAIR_BORDER));
}

static void draw_title(WINDOW *win, int w, const char *title) {
  int len = (int)strlen(title);
  int pos = (w - len - 2) / 2;
  wattron(win, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
  mvwprintw(win, 0, pos, " %s ", title);
  wattroff(win, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
}

static void draw_field(WINDOW *win, int y, int x, int width, const char *label,
                       const char *value, int cursor, bool active,
                       bool is_password) {
  wattron(win, COLOR_PAIR(COLOR_PAIR_HINT));
  mvwprintw(win, y, x, "%s", label);
  wattroff(win, COLOR_PAIR(COLOR_PAIR_HINT));

  int field_x = x;
  int field_y = y + 1;
  int field_w = width;

  if (active) {
    wattron(win, COLOR_PAIR(COLOR_PAIR_PROMPT));
  }
  mvwaddch(win, field_y, field_x, '[');
  mvwaddch(win, field_y, field_x + field_w - 1, ']');
  if (active) {
    wattroff(win, COLOR_PAIR(COLOR_PAIR_PROMPT));
  }

  int inner_w = field_w - 2;
  int val_len = (int)strlen(value);

  for (int i = 0; i < inner_w; i++) {
    int pos = field_x + 1 + i;
    char ch = ' ';
    if (i < val_len) {
      ch = is_password ? '*' : value[i];
    }

    if (active && i == cursor) {
      wattron(win, A_REVERSE);
      mvwaddch(win, field_y, pos, ch);
      wattroff(win, A_REVERSE);
    } else {
      mvwaddch(win, field_y, pos, ch);
    }
  }
}

static void draw_button(WINDOW *win, int y, int x, const char *label,
                        bool selected) {
  if (selected) {
    wattron(win, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD | A_REVERSE);
  } else {
    wattron(win, COLOR_PAIR(COLOR_PAIR_HINT));
  }
  mvwprintw(win, y, x, " %s ", label);
  if (selected) {
    wattroff(win, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD | A_REVERSE);
  } else {
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HINT));
  }
}

static void draw_model_set(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Add Model");

  const char *labels[] = {"Name", "Base URL", "API Key", "Model ID"};
  bool is_pw[] = {false, false, true, false};
  int field_w = m->width - 6;

  for (int i = 0; i < 4; i++) {
    draw_field(w, 2 + i * 3, 3, field_w, labels[i], m->fields[i],
               m->field_cursor[i], m->field_index == i, is_pw[i]);
  }

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 12, "Save", m->field_index == 4);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 5);

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, btn_y, 3, "Tab: next");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wrefresh(w);
}

static void draw_model_list(Modal *m, const ModelsFile *mf) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Select Model");

  if (mf->count == 0) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwprintw(w, m->height / 2, (m->width - 18) / 2, "No models saved.");
    mvwprintw(w, m->height / 2 + 1, (m->width - 24) / 2,
              "Use /model set to add one.");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
  } else {
    int visible = m->height - 6;
    int start = m->list_scroll;

    for (int i = 0; i < visible && (start + i) < (int)mf->count; i++) {
      int idx = start + i;
      int y = 2 + i;

      bool is_active = (idx == mf->active_index);
      bool is_selected = (idx == m->list_selection);

      if (is_selected) {
        wattron(w, A_REVERSE);
      }

      mvwhline(w, y, 2, ' ', m->width - 4);

      if (is_active) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
        mvwaddch(w, y, 3, '>');
        wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
      }

      mvwprintw(w, y, 5, "%-20s", mf->models[idx].name);

      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwprintw(w, y, 26, "%.18s", mf->models[idx].model_id);
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

      if (is_selected) {
        wattroff(w, A_REVERSE);
      }
    }
  }

  int btn_y = m->height - 2;
  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, btn_y, 3, "Enter: select  d: delete  Esc: close");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wrefresh(w);
}

static void draw_message(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);

  if (m->message_is_error) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_BOT) | A_BOLD);
    draw_title(w, m->width, "Error");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_BOT) | A_BOLD);
  } else {
    draw_title(w, m->width, "Help");
  }

  int max_w = m->width - 6;
  int y = 2;
  const char *p = m->message;

  while (*p && y < m->height - 2) {
    const char *line_end = strchr(p, '\n');
    int line_len = line_end ? (int)(line_end - p) : (int)strlen(p);
    if (line_len > max_w)
      line_len = max_w;

    if (line_len == 0) {
      y++;
      p = line_end ? line_end + 1 : p + strlen(p);
      continue;
    }

    char line_buf[256];
    int to_copy = line_len < 255 ? line_len : 255;
    strncpy(line_buf, p, to_copy);
    line_buf[to_copy] = '\0';

    char *cmd_start = strchr(line_buf, '/');
    char *dash = strstr(line_buf, " - ");
    bool is_section_header =
        (line_buf[line_len - 1] == ':' && !dash && !cmd_start);

    if (is_section_header) {
      wattron(w, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
      mvwprintw(w, y, 3, "%s", line_buf);
      wattroff(w, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
    } else if (cmd_start) {
      if (dash) {
        *dash = '\0';
        wattron(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
        mvwprintw(w, y, 3, "%-14s", line_buf);
        wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwprintw(w, y, 18, "%s", dash + 3);
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
      } else {
        wattron(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
        mvwprintw(w, y, 3, "%s", line_buf);
        wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
      }
    } else if (dash) {
      *dash = '\0';
      wattron(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
      mvwprintw(w, y, 3, "%-14s", line_buf);
      wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
      mvwprintw(w, y, 18, "%s", dash + 3);
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
    } else {
      mvwprintw(w, y, 3, "%s", line_buf);
    }

    y++;
    p = line_end ? line_end + 1 : p + strlen(p);
  }

  draw_button(w, m->height - 2, (m->width - 6) / 2, "OK", true);

  wrefresh(w);
}

static void draw_chat_list(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Load Chat");

  int list_height = m->height - 6;
  int visible_count = list_height;

  if (m->chat_list.count == 0) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    mvwprintw(w, 3, 3, "No saved chats");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  } else {
    for (int i = 0;
         i < visible_count && (m->list_scroll + i) < (int)m->chat_list.count;
         i++) {
      int idx = m->list_scroll + i;
      ChatMeta *chat = &m->chat_list.chats[idx];
      bool is_selected = (idx == m->list_selection);

      int y = 2 + i;

      if (is_selected) {
        wattron(w, A_REVERSE);
      }

      mvwhline(w, y, 2, ' ', m->width - 4);

      char time_str[32];
      struct tm *t = localtime(&chat->updated_at);
      strftime(time_str, sizeof(time_str), "%m/%d %H:%M", t);

      char display[128];
      int title_max = m->width - 20;
      if ((int)strlen(chat->title) > title_max) {
        snprintf(display, sizeof(display), "%.*s...", title_max - 3,
                 chat->title);
      } else {
        snprintf(display, sizeof(display), "%s", chat->title);
      }

      mvwprintw(w, y, 3, "%-*s", title_max, display);
      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwprintw(w, y, m->width - 14, "%s", time_str);
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

      if (is_selected) {
        wattroff(w, A_REVERSE);
      }
    }
  }

  int btn_y = m->height - 2;
  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, btn_y, 3, "Enter: load  d: delete  Esc: close");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wrefresh(w);
}

static void draw_chat_save(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Save Chat");

  int field_width = m->width - 10;

  draw_field(w, 2, 3, field_width, "Title", m->fields[0], m->field_cursor[0],
             m->field_index == 0, false);

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  if (m->current_chat_id[0]) {
    mvwprintw(w, 5, 3, "Updating existing chat");
  } else {
    mvwprintw(w, 5, 3, "Creating new chat");
  }
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 8, "Save", m->field_index == 1);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 2);

  wrefresh(w);
}

static void draw_chat_overwrite_confirm(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Overwrite Chat?");

  mvwprintw(w, 2, 3, "A chat with this name already exists.");
  mvwprintw(w, 3, 3, "Overwrite \"%.*s\"?", m->width - 16,
            m->pending_save_title);

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 12, "Overwrite", m->field_index == 0);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 1);

  wrefresh(w);
}

static void draw_exit_confirm(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Exit");

  mvwprintw(w, 2, 3, "Are you sure you want to exit?");

  int checkbox_y = 4;
  if (m->field_index == 0) {
    wattron(w, A_REVERSE);
  }
  mvwprintw(w, checkbox_y, 3, "[%c] Don't ask again",
            m->exit_dont_ask ? 'x' : ' ');
  if (m->field_index == 0) {
    wattroff(w, A_REVERSE);
  }

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 10, "Exit", m->field_index == 1);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 2);

  wrefresh(w);
}

static void draw_persona_edit(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Edit Persona");

  int field_w = m->width - 6;
  draw_field(w, 2, 3, field_w, "Name", m->fields[0], m->field_cursor[0],
             m->field_index == 0, false);
  draw_field(w, 5, 3, field_w, "Description", m->fields[1], m->field_cursor[1],
             m->field_index == 1, false);

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 10, "Save", m->field_index == 2);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 3);

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, btn_y, 3, "Tab: next");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wrefresh(w);
}

static void draw_character_info(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Character Info");

  const CharacterCard *card = m->character;
  if (!card) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwprintw(w, 3, 3, "No character loaded");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
    draw_button(w, m->height - 2, (m->width - 6) / 2, "OK", true);
    wrefresh(w);
    return;
  }

  int y = 2;
  int max_w = m->width - 6;

  wattron(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
  mvwprintw(w, y++, 3, "Name: ");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
  mvwprintw(w, y - 1, 9, "%.*s", max_w - 6, card->name);

  if (card->creator && card->creator[0]) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwprintw(w, y++, 3, "Creator: %.*s", max_w - 12, card->creator);
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
  }

  if (card->character_version && card->character_version[0]) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwprintw(w, y++, 3, "Version: %.*s", max_w - 12, card->character_version);
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
  }

  y++;

  if (card->description && card->description[0]) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
    mvwprintw(w, y++, 3, "Description:");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
    int desc_lines = 0;
    const char *p = card->description;
    while (*p && y < m->height - 4 && desc_lines < 4) {
      int line_len = 0;
      while (p[line_len] && p[line_len] != '\n' && line_len < max_w)
        line_len++;
      mvwaddnstr(w, y++, 3, p, line_len);
      p += line_len;
      if (*p == '\n')
        p++;
      desc_lines++;
    }
    if (*p) {
      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwprintw(w, y++, 3, "...(truncated)");
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    }
  }

  y++;
  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
  mvwprintw(w, y, 3, "Greetings: %zu",
            1 + (card->alternate_greetings_count
                     ? card->alternate_greetings_count
                     : 0));
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));

  draw_button(w, m->height - 2, (m->width - 6) / 2, "OK", true);

  wrefresh(w);
}

static void draw_greeting_select(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Select Greeting");

  const CharacterCard *card = m->character;
  if (!card) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwprintw(w, 3, 3, "No character loaded");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
    wrefresh(w);
    return;
  }

  int total = 1 + (int)card->alternate_greetings_count;
  int visible = m->height - 6;
  int start = m->list_scroll;

  for (int i = 0; i < visible && (start + i) < total; i++) {
    int idx = start + i;
    int y = 2 + i;
    bool is_selected = (idx == (int)m->greeting_selection);

    if (is_selected) {
      wattron(w, A_REVERSE);
    }

    mvwhline(w, y, 2, ' ', m->width - 4);

    const char *greeting = character_get_greeting(card, idx);
    if (greeting) {
      char preview[64];
      int preview_len = 0;
      for (int j = 0; greeting[j] && preview_len < 50; j++) {
        if (greeting[j] == '\n') {
          preview[preview_len++] = ' ';
        } else {
          preview[preview_len++] = greeting[j];
        }
      }
      preview[preview_len] = '\0';

      if (idx == 0) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
        mvwprintw(w, y, 3, "Default: ");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
        mvwprintw(w, y, 12, "%.*s", m->width - 16, preview);
      } else {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwprintw(w, y, 3, "Alt %d: ", idx);
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwprintw(w, y, 11, "%.*s", m->width - 15, preview);
      }
    }

    if (is_selected) {
      wattroff(w, A_REVERSE);
    }
  }

  int btn_y = m->height - 2;
  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, btn_y, 3, "Enter: select  Esc: cancel");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wrefresh(w);
}

void modal_draw(Modal *m, const ModelsFile *mf) {
  if (!m->win)
    return;

  switch (m->type) {
  case MODAL_MODEL_SET:
    draw_model_set(m);
    break;
  case MODAL_MODEL_LIST:
    draw_model_list(m, mf);
    break;
  case MODAL_MESSAGE:
    draw_message(m);
    break;
  case MODAL_CHAT_LIST:
    draw_chat_list(m);
    break;
  case MODAL_CHAT_SAVE:
    draw_chat_save(m);
    break;
  case MODAL_CHAT_OVERWRITE_CONFIRM:
    draw_chat_overwrite_confirm(m);
    break;
  case MODAL_EXIT_CONFIRM:
    draw_exit_confirm(m);
    break;
  case MODAL_PERSONA_EDIT:
    draw_persona_edit(m);
    break;
  case MODAL_CHARACTER_INFO:
    draw_character_info(m);
    break;
  case MODAL_GREETING_SELECT:
    draw_greeting_select(m);
    break;
  default:
    break;
  }
}

static bool handle_field_key(Modal *m, int ch) {
  int fi = m->field_index;
  if (fi >= 4)
    return false;

  char *field = m->fields[fi];
  int *cursor = &m->field_cursor[fi];
  int *len = &m->field_len[fi];
  int max_len = (int)sizeof(m->fields[0]) - 1;

  if (ch == KEY_LEFT) {
    if (*cursor > 0)
      (*cursor)--;
    return true;
  }
  if (ch == KEY_RIGHT) {
    if (*cursor < *len)
      (*cursor)++;
    return true;
  }
  if (ch == KEY_HOME || ch == 1) {
    *cursor = 0;
    return true;
  }
  if (ch == KEY_END || ch == 5) {
    *cursor = *len;
    return true;
  }
  if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
    if (*cursor > 0) {
      memmove(&field[*cursor - 1], &field[*cursor], *len - *cursor + 1);
      (*len)--;
      (*cursor)--;
    }
    return true;
  }
  if (ch == KEY_DC) {
    if (*cursor < *len) {
      memmove(&field[*cursor], &field[*cursor + 1], *len - *cursor);
      (*len)--;
    }
    return true;
  }
  if (isprint(ch) && *len < max_len) {
    memmove(&field[*cursor + 1], &field[*cursor], *len - *cursor + 1);
    field[*cursor] = (char)ch;
    (*len)++;
    (*cursor)++;
    return true;
  }

  return false;
}

ModalResult modal_handle_key(Modal *m, int ch, ModelsFile *mf,
                             ChatHistory *history, char *loaded_chat_id,
                             char *loaded_char_path, size_t char_path_size,
                             Persona *persona, size_t *selected_greeting) {
  if (m->type == MODAL_MESSAGE) {
    if (ch == '\n' || ch == '\r' || ch == 27) {
      modal_close(m);
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_MODEL_SET) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    if (ch == '\t' || ch == KEY_DOWN) {
      m->field_index = (m->field_index + 1) % 6;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_BTAB || ch == KEY_UP) {
      m->field_index = (m->field_index + 5) % 6;
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 5) {
        modal_close(m);
        return MODAL_RESULT_NONE;
      }
      if (m->field_index == 4) {
        if (m->fields[0][0] == '\0') {
          modal_open_message(m, "Name is required", true);
          return MODAL_RESULT_NONE;
        }
        if (m->fields[1][0] == '\0') {
          modal_open_message(m, "Base URL is required", true);
          return MODAL_RESULT_NONE;
        }
        if (m->fields[3][0] == '\0') {
          modal_open_message(m, "Model ID is required", true);
          return MODAL_RESULT_NONE;
        }

        ModelConfig mc = {0};
        strncpy(mc.name, m->fields[0], sizeof(mc.name) - 1);
        strncpy(mc.base_url, m->fields[1], sizeof(mc.base_url) - 1);
        strncpy(mc.api_key, m->fields[2], sizeof(mc.api_key) - 1);
        strncpy(mc.model_id, m->fields[3], sizeof(mc.model_id) - 1);

        if (config_add_model(mf, &mc)) {
          if (mf->active_index < 0) {
            mf->active_index = 0;
          }
          config_save_models(mf);
          modal_open_message(m, "Model saved!", false);
        } else {
          modal_open_message(m, "Failed to save model", true);
        }
        return MODAL_RESULT_NONE;
      }
      m->field_index = (m->field_index + 1) % 6;
      return MODAL_RESULT_NONE;
    }

    if (m->field_index < 4) {
      handle_field_key(m, ch);
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_MODEL_LIST) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_UP || ch == 'k') {
      if (m->list_selection > 0) {
        m->list_selection--;
        if (m->list_selection < m->list_scroll) {
          m->list_scroll = m->list_selection;
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      if (m->list_selection < (int)mf->count - 1) {
        m->list_selection++;
        int visible = m->height - 6;
        if (m->list_selection >= m->list_scroll + visible) {
          m->list_scroll = m->list_selection - visible + 1;
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (mf->count > 0) {
        config_set_active(mf, m->list_selection);
        config_save_models(mf);
        modal_close(m);
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == 'd' || ch == KEY_DC) {
      if (mf->count > 0) {
        config_remove_model(mf, m->list_selection);
        config_save_models(mf);
        if (m->list_selection >= (int)mf->count && m->list_selection > 0) {
          m->list_selection--;
        }
        if (mf->count == 0) {
          modal_close(m);
        }
      }
      return MODAL_RESULT_NONE;
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_CHAT_LIST) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_UP || ch == 'k') {
      if (m->list_selection > 0) {
        m->list_selection--;
        if (m->list_selection < m->list_scroll) {
          m->list_scroll = m->list_selection;
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      if (m->list_selection < (int)m->chat_list.count - 1) {
        m->list_selection++;
        int visible = m->height - 6;
        if (m->list_selection >= m->list_scroll + visible) {
          m->list_scroll = m->list_selection - visible + 1;
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (m->chat_list.count > 0 && history) {
        const char *id = m->chat_list.chats[m->list_selection].id;
        if (chat_load(history, id, loaded_char_path, char_path_size)) {
          if (loaded_chat_id) {
            strncpy(loaded_chat_id, id, CHAT_ID_MAX - 1);
            loaded_chat_id[CHAT_ID_MAX - 1] = '\0';
          }
          modal_close(m);
          return MODAL_RESULT_CHAT_LOADED;
        } else {
          modal_open_message(m, "Failed to load chat", true);
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == 'd' || ch == KEY_DC) {
      if (m->chat_list.count > 0) {
        const char *id = m->chat_list.chats[m->list_selection].id;
        chat_delete(id);
        chat_list_load(&m->chat_list);
        if (m->list_selection >= (int)m->chat_list.count &&
            m->list_selection > 0) {
          m->list_selection--;
        }
        if (m->chat_list.count == 0) {
          modal_close(m);
        }
        return MODAL_RESULT_CHAT_DELETED;
      }
      return MODAL_RESULT_NONE;
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_CHAT_SAVE) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    if (ch == '\t' || ch == KEY_DOWN) {
      m->field_index = (m->field_index + 1) % 3;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_BTAB || ch == KEY_UP) {
      m->field_index = (m->field_index + 2) % 3;
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 2) {
        modal_close(m);
        return MODAL_RESULT_NONE;
      }
      if (m->field_index == 1 || m->field_index == 0) {
        const char *title = NULL;
        static char existing_title[CHAT_TITLE_MAX];

        if (m->fields[0][0]) {
          title = m->fields[0];
        } else if (m->current_chat_id[0] &&
                   chat_get_title_by_id(m->current_chat_id, existing_title,
                                        sizeof(existing_title))) {
          title = existing_title;
        } else {
          title = chat_auto_title(history);
        }

        char existing_id[CHAT_ID_MAX] = {0};
        bool title_exists =
            chat_find_by_title(title, existing_id, sizeof(existing_id));

        bool is_same_chat = m->current_chat_id[0] &&
                            strcmp(m->current_chat_id, existing_id) == 0;

        if (title_exists && !is_same_chat) {
          strncpy(m->pending_save_title, title, CHAT_TITLE_MAX - 1);
          m->pending_save_title[CHAT_TITLE_MAX - 1] = '\0';
          strncpy(m->existing_chat_id, existing_id, CHAT_ID_MAX - 1);
          m->existing_chat_id[CHAT_ID_MAX - 1] = '\0';

          modal_close(m);
          m->type = MODAL_CHAT_OVERWRITE_CONFIRM;
          m->field_index = 0;
          create_window(m, 8, 50);
          return MODAL_RESULT_NONE;
        }

        const char *id =
            m->current_chat_id[0] ? m->current_chat_id : chat_generate_id();

        if (chat_save(history, id, title, m->character_path)) {
          if (loaded_chat_id) {
            strncpy(loaded_chat_id, id, CHAT_ID_MAX - 1);
            loaded_chat_id[CHAT_ID_MAX - 1] = '\0';
          }
          modal_close(m);
          return MODAL_RESULT_CHAT_SAVED;
        } else {
          modal_open_message(m, "Failed to save chat", true);
        }
        return MODAL_RESULT_NONE;
      }
      m->field_index = (m->field_index + 1) % 3;
      return MODAL_RESULT_NONE;
    }

    if (m->field_index == 0) {
      handle_field_key(m, ch);
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_CHAT_OVERWRITE_CONFIRM) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    if (ch == '\t' || ch == KEY_LEFT || ch == KEY_RIGHT) {
      m->field_index = 1 - m->field_index;
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 0) {
        chat_delete(m->existing_chat_id);

        const char *id = m->existing_chat_id;
        if (chat_save(history, id, m->pending_save_title, m->character_path)) {
          if (loaded_chat_id) {
            strncpy(loaded_chat_id, id, CHAT_ID_MAX - 1);
            loaded_chat_id[CHAT_ID_MAX - 1] = '\0';
          }
          modal_close(m);
          return MODAL_RESULT_CHAT_SAVED;
        } else {
          modal_open_message(m, "Failed to save chat", true);
        }
      } else {
        modal_close(m);
      }
      return MODAL_RESULT_NONE;
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_EXIT_CONFIRM) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_EXIT_CANCELLED;
    }
    if (ch == '\t' || ch == KEY_DOWN) {
      m->field_index = (m->field_index + 1) % 3;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_BTAB || ch == KEY_UP) {
      m->field_index = (m->field_index + 2) % 3;
      return MODAL_RESULT_NONE;
    }
    if (ch == ' ' && m->field_index == 0) {
      m->exit_dont_ask = !m->exit_dont_ask;
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 0) {
        m->exit_dont_ask = !m->exit_dont_ask;
        return MODAL_RESULT_NONE;
      }
      if (m->field_index == 1) {
        modal_close(m);
        return MODAL_RESULT_EXIT_CONFIRMED;
      }
      if (m->field_index == 2) {
        modal_close(m);
        return MODAL_RESULT_EXIT_CANCELLED;
      }
    }
    if (ch == 'y' || ch == 'Y') {
      modal_close(m);
      return MODAL_RESULT_EXIT_CONFIRMED;
    }
    if (ch == 'n' || ch == 'N') {
      modal_close(m);
      return MODAL_RESULT_EXIT_CANCELLED;
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_PERSONA_EDIT) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    if (ch == '\t' || ch == KEY_DOWN) {
      m->field_index = (m->field_index + 1) % 4;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_BTAB || ch == KEY_UP) {
      m->field_index = (m->field_index + 3) % 4;
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 3) {
        modal_close(m);
        return MODAL_RESULT_NONE;
      }
      if (m->field_index == 2) {
        if (persona) {
          persona_set_name(persona, m->fields[0]);
          persona_set_description(persona, m->fields[1]);
          persona_save(persona);
        }
        modal_close(m);
        return MODAL_RESULT_PERSONA_SAVED;
      }
      m->field_index = (m->field_index + 1) % 4;
      return MODAL_RESULT_NONE;
    }
    if (m->field_index < 2) {
      handle_field_key(m, ch);
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_CHARACTER_INFO) {
    if (ch == 27 || ch == '\n' || ch == '\r') {
      modal_close(m);
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_GREETING_SELECT) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    int total =
        1 + (m->character ? (int)m->character->alternate_greetings_count : 0);
    if (ch == KEY_UP || ch == 'k') {
      if (m->greeting_selection > 0) {
        m->greeting_selection--;
        if ((int)m->greeting_selection < m->list_scroll) {
          m->list_scroll = (int)m->greeting_selection;
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      if ((int)m->greeting_selection < total - 1) {
        m->greeting_selection++;
        int visible = m->height - 6;
        if ((int)m->greeting_selection >= m->list_scroll + visible) {
          m->list_scroll = (int)m->greeting_selection - visible + 1;
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (selected_greeting) {
        *selected_greeting = m->greeting_selection;
      }
      modal_close(m);
      return MODAL_RESULT_GREETING_SELECTED;
    }
    return MODAL_RESULT_NONE;
  }

  return MODAL_RESULT_NONE;
}

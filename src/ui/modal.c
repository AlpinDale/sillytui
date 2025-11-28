#include "ui/modal.h"
#include "llm/backends/backend.h"
#include "ui/ui.h"
#include <ctype.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

static const char *get_default_base_url(ApiType type) {
  switch (type) {
  case API_TYPE_OPENAI:
    return "https://api.openai.com/v1";
  case API_TYPE_ANTHROPIC:
    return "https://api.anthropic.com";
  case API_TYPE_APHRODITE:
    return "http://localhost:2242/v1";
  case API_TYPE_KOBOLDCPP:
    return "http://localhost:5000";
  case API_TYPE_VLLM:
    return "http://localhost:8000/v1";
  case API_TYPE_LLAMACPP:
    return "http://localhost:8080";
  case API_TYPE_TABBY:
    return "http://localhost:5000/v1";
  default:
    return "";
  }
}

static bool is_default_url(const char *url) {
  if (!url || !url[0])
    return true;
  for (int i = 0; i < API_TYPE_COUNT; i++) {
    const char *def = get_default_base_url((ApiType)i);
    if (def[0] && strcmp(url, def) == 0)
      return true;
  }
  return false;
}

static void set_default_url_for_api(Modal *m) {
  if (is_default_url(m->fields[1])) {
    const char *url = get_default_base_url(m->api_type_selection);
    strncpy(m->fields[1], url, sizeof(m->fields[1]) - 1);
    m->field_len[1] = (int)strlen(m->fields[1]);
    m->field_cursor[1] = m->field_len[1];
  }
}

static void free_fetched_models(Modal *m) {
  if (m->fetched_models) {
    for (size_t i = 0; i < m->fetched_models_count; i++) {
      free(m->fetched_models[i]);
    }
    free(m->fetched_models);
    m->fetched_models = NULL;
  }
  m->fetched_models_count = 0;
  m->fetched_model_index = 0;
}

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} FetchBuffer;

static size_t fetch_write_callback(char *ptr, size_t size, size_t nmemb,
                                   void *userdata) {
  FetchBuffer *buf = userdata;
  size_t total = size * nmemb;

  if (buf->len + total + 1 > buf->cap) {
    size_t newcap = buf->cap == 0 ? 4096 : buf->cap * 2;
    while (newcap < buf->len + total + 1)
      newcap *= 2;
    char *tmp = realloc(buf->data, newcap);
    if (!tmp)
      return 0;
    buf->data = tmp;
    buf->cap = newcap;
  }

  memcpy(buf->data + buf->len, ptr, total);
  buf->len += total;
  buf->data[buf->len] = '\0';
  return total;
}

static bool fetch_models_from_api(Modal *m) {
  free_fetched_models(m);

  if (!m->fields[1][0])
    return false;

  char url[512];
  const char *base = m->fields[1];
  size_t base_len = strlen(base);
  while (base_len > 0 && base[base_len - 1] == '/')
    base_len--;

  char base_trimmed[256];
  snprintf(base_trimmed, sizeof(base_trimmed), "%.*s", (int)base_len, base);

  if (strstr(base_trimmed, "/v1"))
    snprintf(url, sizeof(url), "%s/models", base_trimmed);
  else
    snprintf(url, sizeof(url), "%s/v1/models", base_trimmed);

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  if (m->fields[2][0]) {
    char auth[320];
    snprintf(auth, sizeof(auth), "Authorization: Bearer %s", m->fields[2]);
    headers = curl_slist_append(headers, auth);
  }

  FetchBuffer buf = {0};

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fetch_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

  if (strstr(url, "localhost") || strstr(url, "127.0.0.1")) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  }

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || !buf.data) {
    free(buf.data);
    return false;
  }

  size_t cap = 16;
  m->fetched_models = malloc(cap * sizeof(char *));
  if (!m->fetched_models) {
    free(buf.data);
    return false;
  }

  const char *p = buf.data;
  int max_model_len = 0;

  while ((p = strstr(p, "\"id\"")) != NULL) {
    p += 4;
    while (*p && (*p == ' ' || *p == ':' || *p == '"'))
      p++;
    if (!*p)
      break;

    const char *end = p;
    while (*end && *end != '"')
      end++;

    size_t len = end - p;
    if (len > 0 && len < 256) {
      bool skip = (len > 10 && strncmp(p, "modelperm-", 10) == 0) ||
                  (len > 6 && strncmp(p, "chatcmpl-", 9) == 0);

      if (!skip) {
        if (m->fetched_models_count >= cap) {
          cap *= 2;
          char **tmp = realloc(m->fetched_models, cap * sizeof(char *));
          if (!tmp)
            break;
          m->fetched_models = tmp;
        }

        char *model = malloc(len + 1);
        if (model) {
          memcpy(model, p, len);
          model[len] = '\0';
          m->fetched_models[m->fetched_models_count++] = model;
        }

        if (max_model_len == 0) {
          const char *ctx = strstr(end, "\"max_model_len\"");
          if (ctx) {
            ctx += 15;
            while (*ctx && (*ctx == ' ' || *ctx == ':'))
              ctx++;
            if (*ctx >= '0' && *ctx <= '9') {
              max_model_len = atoi(ctx);
            }
          }
        }
      }
    }
    p = end;
  }

  free(buf.data);
  m->fetched_model_index = 0;

  if (m->fetched_models_count > 0) {
    strncpy(m->fields[3], m->fetched_models[0], sizeof(m->fields[3]) - 1);
    m->field_len[3] = (int)strlen(m->fields[3]);
    m->field_cursor[3] = m->field_len[3];

    if (max_model_len > 0) {
      snprintf(m->fields[4], sizeof(m->fields[4]), "%d", max_model_len);
      m->field_len[4] = (int)strlen(m->fields[4]);
      m->field_cursor[4] = m->field_len[4];
    }
  }

  return m->fetched_models_count > 0;
}

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

  if (m->height < 1)
    m->height = 1;
  if (m->width < 1)
    m->width = 1;

  m->start_y = (max_y - m->height) / 2;
  m->start_x = (max_x - m->width) / 2;

  m->win = newwin(m->height, m->width, m->start_y, m->start_x);
  if (m->win)
    keypad(m->win, TRUE);
}

void modal_open_model_set(Modal *m) {
  modal_close(m);
  m->type = MODAL_MODEL_SET;
  m->field_index = 0;
  for (int i = 0; i < 6; i++) {
    m->fields[i][0] = '\0';
    m->field_cursor[i] = 0;
    m->field_len[i] = 0;
  }
  snprintf(m->fields[4], sizeof(m->fields[4]), "%d", DEFAULT_CONTEXT_LENGTH);
  m->field_len[4] = (int)strlen(m->fields[4]);
  m->field_cursor[4] = m->field_len[4];
  m->api_type_selection = API_TYPE_APHRODITE;

  const char *url = get_default_base_url(m->api_type_selection);
  strncpy(m->fields[1], url, sizeof(m->fields[1]) - 1);
  m->field_len[1] = (int)strlen(m->fields[1]);
  m->field_cursor[1] = m->field_len[1];

  create_window(m, 21, 60);
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

void modal_open_model_edit(Modal *m, const ModelsFile *mf, int model_index) {
  if (model_index < 0 || model_index >= (int)mf->count)
    return;

  modal_close(m);
  m->type = MODAL_MODEL_EDIT;
  m->field_index = 0;
  m->edit_model_index = model_index;

  const ModelConfig *mc = &mf->models[model_index];

  strncpy(m->fields[0], mc->name, sizeof(m->fields[0]) - 1);
  m->field_len[0] = (int)strlen(m->fields[0]);
  m->field_cursor[0] = m->field_len[0];

  strncpy(m->fields[1], mc->base_url, sizeof(m->fields[1]) - 1);
  m->field_len[1] = (int)strlen(m->fields[1]);
  m->field_cursor[1] = m->field_len[1];

  strncpy(m->fields[2], mc->api_key, sizeof(m->fields[2]) - 1);
  m->field_len[2] = (int)strlen(m->fields[2]);
  m->field_cursor[2] = m->field_len[2];

  strncpy(m->fields[3], mc->model_id, sizeof(m->fields[3]) - 1);
  m->field_len[3] = (int)strlen(m->fields[3]);
  m->field_cursor[3] = m->field_len[3];

  snprintf(m->fields[4], sizeof(m->fields[4]), "%d",
           mc->context_length > 0 ? mc->context_length
                                  : DEFAULT_CONTEXT_LENGTH);
  m->field_len[4] = (int)strlen(m->fields[4]);
  m->field_cursor[4] = m->field_len[4];

  m->api_type_selection = mc->api_type;

  create_window(m, 21, 60);
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
                          const char *character_path,
                          const char *character_name) {
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
  if (character_name && character_name[0]) {
    strncpy(m->character_name, character_name, CHAT_CHAR_NAME_MAX - 1);
  } else {
    m->character_name[0] = '\0';
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
    snprintf(m->fields[0], sizeof(m->fields[0]), "%.254s", persona->name);
    m->field_len[0] = (int)strlen(m->fields[0]);
    m->field_cursor[0] = m->field_len[0];
    snprintf(m->fields[1], sizeof(m->fields[1]), "%.254s",
             persona->description);
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

void modal_open_message_edit(Modal *m, int msg_index, const char *content) {
  modal_close(m);
  m->type = MODAL_MESSAGE_EDIT;
  m->edit_msg_index = msg_index;
  m->edit_buffer[0] = '\0';
  m->edit_cursor = 0;
  m->edit_len = 0;
  m->edit_scroll = 0;

  if (content) {
    const char *text = content;
    if (strncmp(content, "You: ", 5) == 0) {
      text = content + 5;
    } else if (strncmp(content, "Bot: ", 5) == 0) {
      text = content + 5;
    } else if (strncmp(content, "Bot:", 4) == 0) {
      text = content + 4;
      while (*text == ' ')
        text++;
    }
    strncpy(m->edit_buffer, text, sizeof(m->edit_buffer) - 1);
    m->edit_buffer[sizeof(m->edit_buffer) - 1] = '\0';
    m->edit_len = (int)strlen(m->edit_buffer);
    m->edit_cursor = m->edit_len;
  }

  create_window(m, 14, 70);
}

void modal_open_message_delete(Modal *m, int msg_index) {
  modal_close(m);
  m->type = MODAL_MESSAGE_DELETE_CONFIRM;
  m->edit_msg_index = msg_index;
  m->field_index = 0;
  create_window(m, 7, 45);
}

void modal_open_sampler_settings(Modal *m, ApiType api_type) {
  modal_close(m);
  m->type = MODAL_SAMPLER_SETTINGS;
  m->sampler_api_type = api_type;
  m->sampler_field_index = 0;
  m->sampler_scroll = 0;
  m->sampler_input[0] = '\0';
  m->sampler_input_cursor = 0;
  m->sampler_adding_custom = false;
  m->sampler_custom_field = 0;
  m->sampler_custom_name[0] = '\0';
  m->sampler_custom_value[0] = '\0';
  m->sampler_custom_min[0] = '\0';
  m->sampler_custom_max[0] = '\0';
  m->sampler_custom_step[0] = '\0';
  m->sampler_custom_type = SAMPLER_TYPE_FLOAT;
  m->sampler_custom_cursor = 0;
  sampler_load(&m->sampler, api_type);
  create_window(m, 24, 58);
}

void modal_open_sampler_yaml(Modal *m, SamplerSettings *sampler,
                             ApiType api_type) {
  modal_close(m);
  m->type = MODAL_SAMPLER_YAML;
  m->sampler = *sampler;
  m->sampler_api_type = api_type;
  m->sampler_yaml_buffer[0] = '\0';
  m->sampler_yaml_cursor = 0;
  m->sampler_yaml_scroll = 0;
  create_window(m, 20, 60);
}

int modal_get_edit_msg_index(const Modal *m) { return m->edit_msg_index; }

const char *modal_get_edit_content(const Modal *m) { return m->edit_buffer; }

void modal_close(Modal *m) {
  if (m->win) {
    delwin(m->win);
    m->win = NULL;
  }
  if (m->type == MODAL_CHAT_LIST) {
    chat_list_free(&m->chat_list);
  }
  free_fetched_models(m);
  m->type = MODAL_NONE;
}

bool modal_is_open(const Modal *m) {
  return m->type != MODAL_NONE && m->win != NULL;
}

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

static void draw_model_fields(Modal *m, WINDOW *w, int *y, int field_w) {
  const char *labels[] = {"Name", "Base URL", "API Key", "Model", "Context"};
  bool is_pw[] = {false, false, true, false, false};

  bool is_anthropic = (m->api_type_selection == API_TYPE_ANTHROPIC);
  bool is_openai_compat = (m->api_type_selection == API_TYPE_APHRODITE ||
                           m->api_type_selection == API_TYPE_VLLM ||
                           m->api_type_selection == API_TYPE_OPENAI ||
                           m->api_type_selection == API_TYPE_TABBY);
  bool has_fetched = (m->fetched_models_count > 0);

  for (int i = 0; i < 5; i++) {
    int fi = i + 1;
    bool model_field = (i == 3);

    if (model_field && is_anthropic) {
      bool selected = (m->field_index == fi);
      if (selected)
        wattron(w, A_BOLD);
      mvwprintw(w, *y, 3, "%s:", labels[i]);
      if (selected)
        wattroff(w, A_BOLD);

      size_t count;
      const char **models = anthropic_get_models(&count);
      const char *display = m->fields[i][0] ? m->fields[i] : models[0];

      if (selected)
        wattron(w, A_REVERSE);
      mvwprintw(w, *y, 12, " < %.30s > ", display);
      if (selected)
        wattroff(w, A_REVERSE);

      if (selected) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
        mvwprintw(w, *y + 1, 3, "←/→: cycle models, or type custom");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      }
      *y += 3;
    } else if (model_field && is_openai_compat && has_fetched) {
      bool selected = (m->field_index == fi);
      if (selected)
        wattron(w, A_BOLD);
      mvwprintw(w, *y, 3, "%s:", labels[i]);
      if (selected)
        wattroff(w, A_BOLD);

      const char *display = m->fields[i][0] ? m->fields[i] : "";

      if (selected)
        wattron(w, A_REVERSE);
      mvwprintw(w, *y, 12, " < %.30s > ", display);
      if (selected)
        wattroff(w, A_REVERSE);

      if (selected) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
        mvwprintw(w, *y + 1, 3, "←/→: cycle, f: refresh, or type");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      }
      *y += 3;
    } else if (model_field && is_openai_compat && !has_fetched) {
      draw_field(w, *y, 3, field_w, labels[i], m->fields[i], m->field_cursor[i],
                 m->field_index == fi, is_pw[i]);
      if (m->field_index == fi) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
        mvwprintw(w, *y + 2, 3, "f: fetch models from API");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      }
      *y += 3;
    } else {
      draw_field(w, *y, 3, field_w, labels[i], m->fields[i], m->field_cursor[i],
                 m->field_index == fi, is_pw[i]);
      *y += 3;
    }
  }
}

static void draw_model_set(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Add Model");

  int field_w = m->width - 6;
  int y = 2;

  bool api_selected = (m->field_index == 0);
  if (api_selected)
    wattron(w, A_BOLD);
  mvwprintw(w, y, 3, "API Type:");
  if (api_selected)
    wattroff(w, A_BOLD);

  if (api_selected)
    wattron(w, A_REVERSE);
  mvwprintw(w, y, 14, " < %s > ", api_type_name(m->api_type_selection));
  if (api_selected)
    wattroff(w, A_REVERSE);
  y += 2;

  draw_model_fields(m, w, &y, field_w);

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 12, "Save", m->field_index == 6);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 7);

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, btn_y, 3, "Tab: next");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wrefresh(w);
}

static void draw_model_edit(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Edit Model");

  int field_w = m->width - 6;
  int y = 2;

  bool api_selected = (m->field_index == 0);
  if (api_selected)
    wattron(w, A_BOLD);
  mvwprintw(w, y, 3, "API Type:");
  if (api_selected)
    wattroff(w, A_BOLD);

  if (api_selected)
    wattron(w, A_REVERSE);
  mvwprintw(w, y, 14, " < %s > ", api_type_name(m->api_type_selection));
  if (api_selected)
    wattroff(w, A_REVERSE);
  y += 2;

  draw_model_fields(m, w, &y, field_w);

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 12, "Save", m->field_index == 6);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 7);

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
  mvwprintw(w, btn_y, 3, "Enter:select e:edit d:del Esc:close");
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
        mvwprintw(w, y, 3, "%s", line_buf);
        wattroff(w, COLOR_PAIR(COLOR_PAIR_PROMPT) | A_BOLD);
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwprintw(w, y, 22, "%s", dash + 3);
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

static void draw_message_edit(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Edit Message");

  int text_w = m->width - 6;
  int text_h = m->height - 6;

  wattron(w, COLOR_PAIR(COLOR_PAIR_BORDER));
  mvwaddstr(w, 2, 2, "╭");
  for (int x = 3; x < m->width - 3; x++)
    mvwaddstr(w, 2, x, "─");
  mvwaddstr(w, 2, m->width - 3, "╮");
  for (int y = 3; y < 3 + text_h; y++) {
    mvwaddstr(w, y, 2, "│");
    mvwaddstr(w, y, m->width - 3, "│");
  }
  mvwaddstr(w, 3 + text_h, 2, "╰");
  for (int x = 3; x < m->width - 3; x++)
    mvwaddstr(w, 3 + text_h, x, "─");
  mvwaddstr(w, 3 + text_h, m->width - 3, "╯");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_BORDER));

  int line = 0;
  int col = 0;
  int cursor_line = 0;
  int cursor_col = 0;

  for (int i = 0; i < m->edit_len; i++) {
    if (i == m->edit_cursor) {
      cursor_line = line;
      cursor_col = col;
    }
    if (m->edit_buffer[i] == '\n') {
      line++;
      col = 0;
    } else {
      col++;
      if (col >= text_w) {
        line++;
        col = 0;
      }
    }
  }
  if (m->edit_cursor == m->edit_len) {
    cursor_line = line;
    cursor_col = col;
  }

  if (cursor_line < m->edit_scroll)
    m->edit_scroll = cursor_line;
  if (cursor_line >= m->edit_scroll + text_h)
    m->edit_scroll = cursor_line - text_h + 1;

  line = 0;
  col = 0;
  int display_line = 0;
  for (int i = 0; i < m->edit_len && display_line < text_h; i++) {
    if (line >= m->edit_scroll) {
      display_line = line - m->edit_scroll;
      if (display_line < text_h) {
        int y = 3 + display_line;
        int x = 3 + col;

        if (i == m->edit_cursor) {
          wattron(w, A_REVERSE);
          mvwaddch(w, y, x,
                   m->edit_buffer[i] == '\n' ? ' ' : m->edit_buffer[i]);
          wattroff(w, A_REVERSE);
        } else if (m->edit_buffer[i] != '\n') {
          mvwaddch(w, y, x, m->edit_buffer[i]);
        }
      }
    }

    if (m->edit_buffer[i] == '\n') {
      line++;
      col = 0;
    } else {
      col++;
      if (col >= text_w) {
        line++;
        col = 0;
      }
    }
  }

  if (m->edit_cursor == m->edit_len) {
    int y = 3 + cursor_line - m->edit_scroll;
    int x = 3 + cursor_col;
    if (y >= 3 && y < 3 + text_h) {
      wattron(w, A_REVERSE);
      mvwaddch(w, y, x, ' ');
      wattroff(w, A_REVERSE);
    }
  }

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 10, "Save", m->field_index == 0);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 1);

  wrefresh(w);
}

static void draw_message_delete_confirm(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "Delete Message?");

  mvwprintw(w, 2, 3, "Delete this message permanently?");

  int btn_y = m->height - 2;
  draw_button(w, btn_y, m->width / 2 - 10, "Delete", m->field_index == 0);
  draw_button(w, btn_y, m->width / 2 + 2, "Cancel", m->field_index == 1);

  wrefresh(w);
}

typedef struct {
  const char *name;
  double *dval;
  int *ival;
  bool is_int;
  double min_val;
  double max_val;
  int category;
} SamplerField;

static const char *SAMPLER_CATEGORIES[] = {
    "─── Core ───",   "─── Penalties ───",    "─── Truncation ───",
    "─── Tokens ───", "─── Dynamic Temp ───", "─── Mirostat ───",
    "─── DRY ───",    "─── XTC ───",          "─── Other ───",
};

static void draw_sampler_settings(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);

  char title[64];
  snprintf(title, sizeof(title), "⚙ Samplers · %s",
           api_type_name(m->sampler_api_type));
  draw_title(w, m->width, title);

  SamplerSettings *s = &m->sampler;
  SamplerField fields[] = {
      {"Temperature", &s->temperature, NULL, false, 0.0, 5.0, 0},
      {"Top P", &s->top_p, NULL, false, 0.0, 1.0, 0},
      {"Top K", NULL, &s->top_k, true, -1, 1000, 0},
      {"Min P", &s->min_p, NULL, false, 0.0, 1.0, 0},
      {"Freq Penalty", &s->frequency_penalty, NULL, false, -2.0, 2.0, 1},
      {"Pres Penalty", &s->presence_penalty, NULL, false, -2.0, 2.0, 1},
      {"Rep Penalty", &s->repetition_penalty, NULL, false, 0.0, 3.0, 1},
      {"Typical P", &s->typical_p, NULL, false, 0.0, 1.0, 2},
      {"TFS", &s->tfs, NULL, false, 0.0, 1.0, 2},
      {"Top A", &s->top_a, NULL, false, 0.0, 1.0, 2},
      {"Smoothing", &s->smoothing_factor, NULL, false, 0.0, 10.0, 2},
      {"Min Tokens", NULL, &s->min_tokens, true, 0, 4096, 3},
      {"Max Tokens", NULL, &s->max_tokens, true, 1, 16384, 3},
      {"DynaTemp Min", &s->dynatemp_min, NULL, false, 0.0, 5.0, 4},
      {"DynaTemp Max", &s->dynatemp_max, NULL, false, 0.0, 5.0, 4},
      {"DynaTemp Exp", &s->dynatemp_exponent, NULL, false, 0.0, 10.0, 4},
      {"Mirostat Mode", NULL, &s->mirostat_mode, true, 0, 2, 5},
      {"Mirostat Tau", &s->mirostat_tau, NULL, false, 0.0, 20.0, 5},
      {"Mirostat Eta", &s->mirostat_eta, NULL, false, 0.0, 1.0, 5},
      {"DRY Mult", &s->dry_multiplier, NULL, false, 0.0, 2.0, 6},
      {"DRY Base", &s->dry_base, NULL, false, 1.0, 3.0, 6},
      {"DRY Len", NULL, &s->dry_allowed_length, true, 0, 20, 6},
      {"DRY Range", NULL, &s->dry_range, true, 0, 4096, 6},
      {"XTC Thresh", &s->xtc_threshold, NULL, false, 0.0, 1.0, 7},
      {"XTC Prob", &s->xtc_probability, NULL, false, 0.0, 1.0, 7},
      {"NSigma", &s->nsigma, NULL, false, 0.0, 5.0, 8},
      {"Skew", &s->skew, NULL, false, -10.0, 10.0, 8},
  };
  int base_field_count = sizeof(fields) / sizeof(fields[0]);
  int total_fields = base_field_count + s->custom_count;

  int visible = m->height - 6;
  int start = m->sampler_scroll;

  if (start > total_fields - visible)
    start = total_fields - visible;
  if (start < 0)
    start = 0;

  const char *current_cat_str;
  if (m->sampler_field_index < base_field_count) {
    current_cat_str =
        SAMPLER_CATEGORIES[fields[m->sampler_field_index].category];
  } else {
    current_cat_str = "─── Custom ───";
  }
  wattron(w, COLOR_PAIR(m->sampler_field_index < base_field_count
                            ? COLOR_PAIR_BOT
                            : COLOR_PAIR_SWIPE) |
                 A_DIM);
  int cat_display_len = 0;
  for (const char *p = current_cat_str; *p; p++) {
    if ((*p & 0xC0) != 0x80)
      cat_display_len++;
  }
  int cat_x = (m->width - cat_display_len) / 2;
  mvwprintw(w, 2, cat_x, "%s", current_cat_str);
  wattroff(w, COLOR_PAIR(m->sampler_field_index < base_field_count
                             ? COLOR_PAIR_BOT
                             : COLOR_PAIR_SWIPE) |
                  A_DIM);

  for (int i = 0; i < visible; i++) {
    int y = 3 + i;
    int idx = start + i;

    if (idx >= total_fields)
      break;

    bool is_selected = (idx == m->sampler_field_index);

    if (idx < base_field_count) {
      SamplerField *f = &fields[idx];

      char val_str[32];
      double val_normalized = 0.0;
      if (f->is_int) {
        snprintf(val_str, sizeof(val_str), "%d", *f->ival);
        if (f->max_val > f->min_val)
          val_normalized = (*f->ival - f->min_val) / (f->max_val - f->min_val);
      } else {
        snprintf(val_str, sizeof(val_str), "%.3g", *f->dval);
        if (f->max_val > f->min_val)
          val_normalized = (*f->dval - f->min_val) / (f->max_val - f->min_val);
      }
      if (val_normalized < 0.0)
        val_normalized = 0.0;
      if (val_normalized > 1.0)
        val_normalized = 1.0;

      int left_pad = 6;
      if (is_selected) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
        mvwaddstr(w, y, left_pad, "▸");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
      } else {
        mvwaddstr(w, y, left_pad, " ");
      }

      if (is_selected)
        wattron(w, A_BOLD);
      mvwprintw(w, y, left_pad + 2, "%-13s", f->name);
      if (is_selected)
        wattroff(w, A_BOLD);

      int bar_width = 14;
      int bar_x = left_pad + 16;
      int filled = (int)(val_normalized * bar_width);

      mvwaddstr(w, y, bar_x, "│");
      for (int b = 0; b < bar_width; b++) {
        if (b < filled) {
          wattron(w, COLOR_PAIR(COLOR_PAIR_USER));
          waddstr(w, "━");
          wattroff(w, COLOR_PAIR(COLOR_PAIR_USER));
        } else {
          wattron(w, A_DIM);
          waddstr(w, "─");
          wattroff(w, A_DIM);
        }
      }
      waddstr(w, "│");

      int val_x = bar_x + bar_width + 3;
      if (is_selected && m->sampler_input[0]) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
        mvwprintw(w, y, val_x, "%-8s", m->sampler_input);
        wattroff(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
        int cx = val_x + m->sampler_input_cursor;
        if (cx < m->width - 2)
          mvwchgat(w, y, cx, 1, A_UNDERLINE, 0, NULL);
      } else {
        if (is_selected)
          wattron(w, COLOR_PAIR(COLOR_PAIR_USER));
        mvwprintw(w, y, val_x, "%-8s", val_str);
        if (is_selected)
          wattroff(w, COLOR_PAIR(COLOR_PAIR_USER));
      }
    } else {
      int custom_idx = idx - base_field_count;
      CustomSampler *cs = &s->custom[custom_idx];
      int left_pad = 6;

      if (is_selected) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
        mvwaddstr(w, y, left_pad, "▸");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
      } else {
        mvwaddstr(w, y, left_pad, " ");
      }

      if (is_selected)
        wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
      else
        wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
      mvwprintw(w, y, left_pad + 2, "%-13.13s", cs->name);
      if (is_selected)
        wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
      else
        wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));

      char val_str[64];
      if (cs->type == SAMPLER_TYPE_STRING) {
        snprintf(val_str, sizeof(val_str), "%.20s", cs->str_value);
      } else if (cs->type == SAMPLER_TYPE_INT) {
        snprintf(val_str, sizeof(val_str), "%d", (int)cs->value);
      } else if (cs->type == SAMPLER_TYPE_BOOL) {
        snprintf(val_str, sizeof(val_str), "%s",
                 cs->value != 0 ? "true" : "false");
      } else if (cs->type == SAMPLER_TYPE_LIST_FLOAT ||
                 cs->type == SAMPLER_TYPE_LIST_INT ||
                 cs->type == SAMPLER_TYPE_LIST_STRING) {
        if (cs->list_count == 0) {
          snprintf(val_str, sizeof(val_str), "(empty)");
        } else if (cs->list_count == 1) {
          if (cs->type == SAMPLER_TYPE_LIST_STRING)
            snprintf(val_str, sizeof(val_str), "[%.12s]", cs->list_strings[0]);
          else if (cs->type == SAMPLER_TYPE_LIST_INT)
            snprintf(val_str, sizeof(val_str), "[%d]", (int)cs->list_values[0]);
          else
            snprintf(val_str, sizeof(val_str), "[%.4g]", cs->list_values[0]);
        } else {
          snprintf(val_str, sizeof(val_str), "[%d items]", cs->list_count);
        }
      } else if (cs->type == SAMPLER_TYPE_DICT) {
        if (cs->dict_count == 0) {
          snprintf(val_str, sizeof(val_str), "(empty)");
        } else if (cs->dict_count == 1) {
          snprintf(val_str, sizeof(val_str), "{%s:...}",
                   cs->dict_entries[0].key);
        } else {
          snprintf(val_str, sizeof(val_str), "{%d keys}", cs->dict_count);
        }
      } else {
        snprintf(val_str, sizeof(val_str), "%.3g", cs->value);
      }

      int val_x;
      int tag_x = left_pad + 16;
      if (cs->type == SAMPLER_TYPE_STRING) {
        wattron(w, A_DIM);
        mvwprintw(w, y, tag_x, "[str]");
        wattroff(w, A_DIM);
        val_x = tag_x + 6;
      } else if (cs->type == SAMPLER_TYPE_BOOL) {
        wattron(w, A_DIM);
        mvwprintw(w, y, tag_x, "[?]");
        wattroff(w, A_DIM);
        val_x = tag_x + 4;
      } else if (cs->type == SAMPLER_TYPE_DICT) {
        wattron(w, A_DIM);
        mvwprintw(w, y, tag_x, "{}");
        wattroff(w, A_DIM);
        val_x = tag_x + 3;
      } else if (cs->type >= SAMPLER_TYPE_LIST_FLOAT) {
        const char *tag = cs->type == SAMPLER_TYPE_LIST_STRING ? "[s]"
                          : cs->type == SAMPLER_TYPE_LIST_INT  ? "[i]"
                                                               : "[f]";
        wattron(w, A_DIM);
        mvwprintw(w, y, tag_x, "%s", tag);
        wattroff(w, A_DIM);
        val_x = tag_x + 4;
      } else {
        double val_normalized = 0.0;
        if (cs->max_val > cs->min_val)
          val_normalized =
              (cs->value - cs->min_val) / (cs->max_val - cs->min_val);
        if (val_normalized < 0.0)
          val_normalized = 0.0;
        if (val_normalized > 1.0)
          val_normalized = 1.0;

        int bar_width = 14;
        int bar_x = tag_x;
        int filled = (int)(val_normalized * bar_width);

        mvwaddstr(w, y, bar_x, "│");
        for (int b = 0; b < bar_width; b++) {
          if (b < filled) {
            wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
            waddstr(w, "━");
            wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
          } else {
            wattron(w, A_DIM);
            waddstr(w, "─");
            wattroff(w, A_DIM);
          }
        }
        waddstr(w, "│");
        val_x = bar_x + bar_width + 3;
      }

      if (is_selected && m->sampler_input[0]) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
        mvwprintw(w, y, val_x, "%-12s", m->sampler_input);
        wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
      } else {
        if (is_selected)
          wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
        mvwprintw(w, y, val_x, "%-12s", val_str);
        if (is_selected)
          wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
      }
    }
  }

  if (start > 0) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwaddstr(w, 3, m->width - 3, "▲");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
  }
  if (start + visible < total_fields) {
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwaddstr(w, m->height - 3, m->width - 3, "▼");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
  }

  mvwhline(w, m->height - 3, 1, ACS_HLINE, m->width - 2);
  mvwaddch(w, m->height - 3, 0, ACS_LTEE);
  mvwaddch(w, m->height - 3, m->width - 1, ACS_RTEE);

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, m->height - 2, 2, "↑↓");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
  mvwprintw(w, m->height - 2, 5, "+");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
  mvwprintw(w, m->height - 2, 6, "add");

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, m->height - 2, 11, "d");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, m->height - 2, 12, "el");

  wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
  mvwprintw(w, m->height - 2, 17, "y");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
  mvwprintw(w, m->height - 2, 18, "aml");

  wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
  mvwprintw(w, m->height - 2, m->width - 14, "Tab");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
  mvwprintw(w, m->height - 2, m->width - 10, "save");

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, m->height - 2, m->width - 5, "Esc");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  if (m->sampler_adding_custom) {
    int dw = 56, dh = 16;
    int dx = (m->width - dw) / 2;
    int dy = (m->height - dh) / 2;

    for (int yy = dy; yy < dy + dh; yy++) {
      mvwhline(w, yy, dx, ' ', dw);
    }

    wattron(w, COLOR_PAIR(COLOR_PAIR_BORDER));
    for (int yy = dy; yy < dy + dh; yy++) {
      mvwaddstr(w, yy, dx, "│");
      mvwaddstr(w, yy, dx + dw - 1, "│");
    }
    mvwhline(w, dy, dx + 1, ACS_HLINE, dw - 2);
    mvwhline(w, dy + dh - 1, dx + 1, ACS_HLINE, dw - 2);
    mvwaddch(w, dy, dx, ACS_ULCORNER);
    mvwaddch(w, dy, dx + dw - 1, ACS_URCORNER);
    mvwaddch(w, dy + dh - 1, dx, ACS_LLCORNER);
    mvwaddch(w, dy + dh - 1, dx + dw - 1, ACS_LRCORNER);
    wattroff(w, COLOR_PAIR(COLOR_PAIR_BORDER));

    wattron(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_USER));
    mvwprintw(w, dy, dx + (dw - 16) / 2, " Add Sampler ");
    wattroff(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_USER));

    wattron(w, A_DIM);
    mvwprintw(w, dy + 2, dx + 2, "Name");
    wattroff(w, A_DIM);
    if (m->sampler_custom_field == 0)
      wattron(w, A_REVERSE);
    mvwprintw(w, dy + 3, dx + 2, " %-48.48s ", m->sampler_custom_name);
    if (m->sampler_custom_field == 0)
      wattroff(w, A_REVERSE);

    wattron(w, A_DIM);
    mvwprintw(w, dy + 5, dx + 2, "Type");
    wattroff(w, A_DIM);
    const char *types[] = {"float", "int", "str", "bool",
                           "[f]",   "[i]", "[s]", "{}"};
    int type_idx = (int)m->sampler_custom_type;
    int type_pos[] = {0, 6, 10, 14, 19, 23, 27, 31};
    if (m->sampler_custom_field == 1)
      wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
    mvwprintw(w, dy + 6, dx + 2, "←");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
    for (int t = 0; t < 8; t++) {
      if (t == type_idx) {
        wattron(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_SWIPE));
      }
      mvwprintw(w, dy + 6, dx + 4 + type_pos[t], "%s", types[t]);
      if (t == type_idx) {
        wattroff(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_SWIPE));
      }
    }
    if (m->sampler_custom_field == 1)
      wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
    mvwprintw(w, dy + 6, dx + 38, "→");
    wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));

    if (m->sampler_custom_type >= SAMPLER_TYPE_LIST_FLOAT &&
        m->sampler_custom_type <= SAMPLER_TYPE_LIST_STRING) {
      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwprintw(w, dy + 8, dx + 2, "(List items added after creation)");
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    } else if (m->sampler_custom_type == SAMPLER_TYPE_DICT) {
      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwprintw(w, dy + 8, dx + 2, "(Dict entries added after creation)");
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    } else if (m->sampler_custom_type == SAMPLER_TYPE_BOOL) {
      wattron(w, A_DIM);
      mvwprintw(w, dy + 8, dx + 2, "Value");
      wattroff(w, A_DIM);
      bool bval = (m->sampler_custom_value[0] == 't' ||
                   m->sampler_custom_value[0] == 'T' ||
                   m->sampler_custom_value[0] == '1');
      if (m->sampler_custom_field == 2)
        wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
      mvwprintw(w, dy + 9, dx + 2, "←");
      wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
      if (bval)
        wattron(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_SWIPE));
      mvwprintw(w, dy + 9, dx + 4, "true");
      if (bval)
        wattroff(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_SWIPE));
      mvwaddstr(w, dy + 9, dx + 9, " / ");
      if (!bval)
        wattron(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_SWIPE));
      mvwprintw(w, dy + 9, dx + 12, "false");
      if (!bval)
        wattroff(w, A_BOLD | COLOR_PAIR(COLOR_PAIR_SWIPE));
      if (m->sampler_custom_field == 2)
        wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
      mvwprintw(w, dy + 9, dx + 18, "→");
      wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
    } else {
      wattron(w, A_DIM);
      mvwprintw(w, dy + 8, dx + 2, "Value");
      wattroff(w, A_DIM);
      if (m->sampler_custom_field == 2)
        wattron(w, A_REVERSE);
      if (m->sampler_custom_type == SAMPLER_TYPE_STRING) {
        mvwprintw(w, dy + 9, dx + 2, " %-48.48s ", m->sampler_custom_value);
      } else {
        mvwprintw(w, dy + 9, dx + 2, " %-10.10s ", m->sampler_custom_value);
      }
      if (m->sampler_custom_field == 2)
        wattroff(w, A_REVERSE);
    }

    if (m->sampler_custom_type < SAMPLER_TYPE_STRING) {
      wattron(w, A_DIM);
      mvwprintw(w, dy + 9, dx + 18, "Min");
      mvwprintw(w, dy + 9, dx + 30, "Max");
      mvwprintw(w, dy + 9, dx + 42, "Step");
      wattroff(w, A_DIM);
      if (m->sampler_custom_field == 3)
        wattron(w, A_REVERSE);
      mvwprintw(w, dy + 10, dx + 18, "%-8.8s",
                m->sampler_custom_min[0] ? m->sampler_custom_min : "0");
      if (m->sampler_custom_field == 3)
        wattroff(w, A_REVERSE);

      if (m->sampler_custom_field == 4)
        wattron(w, A_REVERSE);
      mvwprintw(w, dy + 10, dx + 30, "%-8.8s",
                m->sampler_custom_max[0] ? m->sampler_custom_max : "100");
      if (m->sampler_custom_field == 4)
        wattroff(w, A_REVERSE);

      if (m->sampler_custom_field == 5)
        wattron(w, A_REVERSE);
      mvwprintw(
          w, dy + 10, dx + 42, "%-8.8s",
          m->sampler_custom_step[0]
              ? m->sampler_custom_step
              : (m->sampler_custom_type == SAMPLER_TYPE_INT ? "1" : "0.1"));
      if (m->sampler_custom_field == 5)
        wattroff(w, A_REVERSE);
    }

    mvwhline(w, dy + dh - 4, dx + 1, ACS_HLINE, dw - 2);
    mvwaddch(w, dy + dh - 4, dx, ACS_LTEE);
    mvwaddch(w, dy + dh - 4, dx + dw - 1, ACS_RTEE);

    const char *type_desc[] = {"Decimal number (e.g. 0.95, 1.5)",
                               "Whole number (e.g. 40, 100)",
                               "Text string (e.g. \"hello\")",
                               "True or false toggle",
                               "List of decimal numbers",
                               "List of whole numbers",
                               "List of text strings",
                               "Key-value dictionary"};
    wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
    mvwprintw(w, dy + dh - 3, dx + 2, "%s", type_desc[type_idx]);
    wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));

    wattron(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 2, "Tab");
    wattroff(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 6, "next");
    wattron(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 13, "←→");
    wattroff(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 16, "type");
    wattron(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 23, "Enter");
    wattroff(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 29, "add");
    wattron(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 35, "Esc");
    wattroff(w, A_DIM);
    mvwprintw(w, dy + dh - 2, dx + 39, "cancel");
  }

  if (m->sampler_editing_list) {
    int base_count = 27;
    int idx = m->sampler_field_index;
    if (idx >= base_count) {
      CustomSampler *cs = &s->custom[idx - base_count];

      int dw = 50, dh = 16;
      int dx = (m->width - dw) / 2;
      int dy = (m->height - dh) / 2;

      for (int yy = dy; yy < dy + dh; yy++) {
        mvwhline(w, yy, dx, ' ', dw);
      }

      wattron(w, COLOR_PAIR(COLOR_PAIR_BORDER));
      for (int yy = dy; yy < dy + dh; yy++) {
        mvwaddstr(w, yy, dx, "│");
        mvwaddstr(w, yy, dx + dw - 1, "│");
      }
      mvwhline(w, dy, dx + 1, ACS_HLINE, dw - 2);
      mvwhline(w, dy + dh - 1, dx + 1, ACS_HLINE, dw - 2);
      mvwaddch(w, dy, dx, ACS_ULCORNER);
      mvwaddch(w, dy, dx + dw - 1, ACS_URCORNER);
      mvwaddch(w, dy + dh - 1, dx, ACS_LLCORNER);
      mvwaddch(w, dy + dh - 1, dx + dw - 1, ACS_LRCORNER);
      wattroff(w, COLOR_PAIR(COLOR_PAIR_BORDER));

      char title[128];
      snprintf(title, sizeof(title), " %s (%d items) ", cs->name,
               cs->list_count);
      wattron(w, A_BOLD);
      mvwprintw(w, dy, dx + (dw - (int)strlen(title)) / 2, "%s", title);
      wattroff(w, A_BOLD);

      int visible = dh - 5;
      int start = m->sampler_list_scroll;
      if (start > cs->list_count - visible)
        start = cs->list_count - visible;
      if (start < 0)
        start = 0;

      for (int i = 0; i < visible; i++) {
        int li = start + i;
        int row = dy + 2 + i;

        if (li >= cs->list_count) {
          if (li == cs->list_count && m->sampler_list_index == li) {
            wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
            mvwprintw(w, row, dx + 2, "▸ + Add new item");
            wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
          }
          continue;
        }

        bool is_sel = (li == m->sampler_list_index);

        if (is_sel) {
          wattron(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
          mvwaddstr(w, row, dx + 2, "▸");
          wattroff(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
        } else {
          mvwaddstr(w, row, dx + 2, " ");
        }

        wattron(w, A_DIM);
        mvwprintw(w, row, dx + 4, "%2d.", li + 1);
        wattroff(w, A_DIM);

        char item_str[40];
        if (cs->type == SAMPLER_TYPE_LIST_STRING)
          snprintf(item_str, sizeof(item_str), "%.36s", cs->list_strings[li]);
        else if (cs->type == SAMPLER_TYPE_LIST_INT)
          snprintf(item_str, sizeof(item_str), "%d", (int)cs->list_values[li]);
        else
          snprintf(item_str, sizeof(item_str), "%.6g", cs->list_values[li]);

        if (is_sel && m->sampler_list_input[0]) {
          wattron(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
          mvwprintw(w, row, dx + 8, "%-38.38s", m->sampler_list_input);
          wattroff(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
        } else {
          if (is_sel)
            wattron(w, COLOR_PAIR(COLOR_PAIR_USER));
          mvwprintw(w, row, dx + 8, "%-38.38s", item_str);
          if (is_sel)
            wattroff(w, COLOR_PAIR(COLOR_PAIR_USER));
        }
      }

      if (start > 0) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwaddstr(w, dy + 2, dx + dw - 2, "▲");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
      }
      if (start + visible < cs->list_count + 1) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwaddstr(w, dy + dh - 4, dx + dw - 2, "▼");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
      }

      mvwhline(w, dy + dh - 3, dx + 1, ACS_HLINE, dw - 2);
      mvwaddch(w, dy + dh - 3, dx, ACS_LTEE);
      mvwaddch(w, dy + dh - 3, dx + dw - 1, ACS_RTEE);

      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwprintw(w, dy + dh - 2, dx + 2, "↑↓:nav Enter:edit/add d:del Esc:done");
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    }
  }

  if (m->sampler_editing_dict) {
    int base_count = 27;
    int idx = m->sampler_field_index;
    if (idx >= base_count) {
      CustomSampler *cs = &s->custom[idx - base_count];

      int dw = 54, dh = 14;
      int dx = (m->width - dw) / 2;
      int dy = (m->height - dh) / 2;

      for (int yy = dy; yy < dy + dh; yy++) {
        mvwhline(w, yy, dx, ' ', dw);
      }

      wattron(w, COLOR_PAIR(COLOR_PAIR_BORDER));
      for (int yy = dy; yy < dy + dh; yy++) {
        mvwaddstr(w, yy, dx, "│");
        mvwaddstr(w, yy, dx + dw - 1, "│");
      }
      mvwhline(w, dy, dx + 1, ACS_HLINE, dw - 2);
      mvwhline(w, dy + dh - 1, dx + 1, ACS_HLINE, dw - 2);
      mvwaddch(w, dy, dx, ACS_ULCORNER);
      mvwaddch(w, dy, dx + dw - 1, ACS_URCORNER);
      mvwaddch(w, dy + dh - 1, dx, ACS_LLCORNER);
      mvwaddch(w, dy + dh - 1, dx + dw - 1, ACS_LRCORNER);
      wattroff(w, COLOR_PAIR(COLOR_PAIR_BORDER));

      char title[128];
      snprintf(title, sizeof(title), " %s (%d keys) ", cs->name,
               cs->dict_count);
      wattron(w, A_BOLD);
      mvwprintw(w, dy, dx + (dw - (int)strlen(title)) / 2, "%s", title);
      wattroff(w, A_BOLD);

      int visible = dh - 5;
      int start = m->sampler_dict_scroll;
      if (start > cs->dict_count - visible)
        start = cs->dict_count - visible;
      if (start < 0)
        start = 0;

      for (int i = 0; i < visible; i++) {
        int di = start + i;
        int row = dy + 2 + i;

        if (di > cs->dict_count)
          continue;

        if (di == cs->dict_count) {
          if (m->sampler_dict_index == di) {
            wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
            mvwprintw(w, row, dx + 2, "▸ + Add new entry");
            wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE) | A_BOLD);
          }
          continue;
        }

        bool is_sel = (di == m->sampler_dict_index);
        DictEntry *de = &cs->dict_entries[di];

        if (is_sel) {
          wattron(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
          mvwaddstr(w, row, dx + 2, "▸");
          wattroff(w, COLOR_PAIR(COLOR_PAIR_USER) | A_BOLD);
        } else {
          mvwaddstr(w, row, dx + 2, " ");
        }

        if (is_sel)
          wattron(w, COLOR_PAIR(COLOR_PAIR_USER));
        mvwprintw(w, row, dx + 4, "%-14.14s", de->key);
        if (is_sel)
          wattroff(w, COLOR_PAIR(COLOR_PAIR_USER));

        wattron(w, A_DIM);
        mvwaddstr(w, row, dx + 19, ":");
        wattroff(w, A_DIM);

        char val_disp[128];
        if (de->is_string)
          snprintf(val_disp, sizeof(val_disp), "\"%s\"", de->str_val);
        else
          snprintf(val_disp, sizeof(val_disp), "%.6g", de->num_val);

        if (is_sel)
          wattron(w, COLOR_PAIR(COLOR_PAIR_USER));
        mvwprintw(w, row, dx + 21, "%-28.28s", val_disp);
        if (is_sel)
          wattroff(w, COLOR_PAIR(COLOR_PAIR_USER));
      }

      if (start > 0) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwaddstr(w, dy + 2, dx + dw - 2, "▲");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
      }
      if (start + visible < cs->dict_count + 1) {
        wattron(w, COLOR_PAIR(COLOR_PAIR_HINT));
        mvwaddstr(w, dy + dh - 4, dx + dw - 2, "▼");
        wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT));
      }

      mvwhline(w, dy + dh - 3, dx + 1, ACS_HLINE, dw - 2);
      mvwaddch(w, dy + dh - 3, dx, ACS_LTEE);
      mvwaddch(w, dy + dh - 3, dx + dw - 1, ACS_RTEE);

      wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
      mvwprintw(w, dy + dh - 2, dx + 2, "↑↓:nav Enter:add d:del Esc:done");
      wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
    }
  }

  wrefresh(w);
}

static void draw_sampler_yaml(Modal *m) {
  WINDOW *w = m->win;
  werase(w);
  draw_box_fancy(w, m->height, m->width);
  draw_title(w, m->width, "YAML Bulk Import");

  int text_area_h = m->height - 6;
  int text_area_w = m->width - 4;

  int line_count = 1;
  for (int i = 0; m->sampler_yaml_buffer[i]; i++) {
    if (m->sampler_yaml_buffer[i] == '\n')
      line_count++;
  }

  int visible_lines = text_area_h;
  if (m->sampler_yaml_scroll > line_count - visible_lines)
    m->sampler_yaml_scroll = line_count - visible_lines;
  if (m->sampler_yaml_scroll < 0)
    m->sampler_yaml_scroll = 0;

  int cur_line = 0;
  for (int i = 0; i < m->sampler_yaml_cursor && m->sampler_yaml_buffer[i];
       i++) {
    if (m->sampler_yaml_buffer[i] == '\n') {
      cur_line++;
    }
  }

  if (cur_line < m->sampler_yaml_scroll)
    m->sampler_yaml_scroll = cur_line;
  if (cur_line >= m->sampler_yaml_scroll + visible_lines)
    m->sampler_yaml_scroll = cur_line - visible_lines + 1;

  int line = 0;
  int col = 0;
  int buf_pos = 0;
  while (m->sampler_yaml_buffer[buf_pos] && line < m->sampler_yaml_scroll) {
    if (m->sampler_yaml_buffer[buf_pos] == '\n')
      line++;
    buf_pos++;
  }

  for (int y = 0; y < visible_lines; y++) {
    int draw_y = 2 + y;
    col = 0;

    int line_num = m->sampler_yaml_scroll + y + 1;
    if (line_num <= line_count) {
      wattron(w, A_DIM);
      mvwprintw(w, draw_y, 1, "%2d", line_num);
      wattroff(w, A_DIM);
    } else {
      mvwprintw(w, draw_y, 1, "  ");
    }
    mvwaddstr(w, draw_y, 3, " ");

    if (m->sampler_yaml_buffer[buf_pos]) {
      while (m->sampler_yaml_buffer[buf_pos] &&
             m->sampler_yaml_buffer[buf_pos] != '\n' && col < text_area_w - 4) {
        char c = m->sampler_yaml_buffer[buf_pos];
        if (buf_pos == m->sampler_yaml_cursor) {
          wattron(w, A_REVERSE);
          mvwaddch(w, draw_y, 4 + col, c);
          wattroff(w, A_REVERSE);
        } else {
          if (c == ':') {
            wattron(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
            mvwaddch(w, draw_y, 4 + col, c);
            wattroff(w, COLOR_PAIR(COLOR_PAIR_SWIPE));
          } else {
            mvwaddch(w, draw_y, 4 + col, c);
          }
        }
        col++;
        buf_pos++;
      }

      if (m->sampler_yaml_buffer[buf_pos] == '\n') {
        if (buf_pos == m->sampler_yaml_cursor) {
          wattron(w, A_REVERSE);
          mvwaddch(w, draw_y, 4 + col, ' ');
          wattroff(w, A_REVERSE);
        }
        buf_pos++;
        line++;
      } else if (!m->sampler_yaml_buffer[buf_pos] &&
                 buf_pos == m->sampler_yaml_cursor) {
        wattron(w, A_REVERSE);
        mvwaddch(w, draw_y, 4 + col, ' ');
        wattroff(w, A_REVERSE);
      }
    } else if (y == 0 && !m->sampler_yaml_buffer[0]) {
      wattron(w, A_REVERSE);
      mvwaddch(w, draw_y, 4, ' ');
      wattroff(w, A_REVERSE);
    }
  }

  mvwhline(w, m->height - 4, 1, ACS_HLINE, m->width - 2);
  mvwaddch(w, m->height - 4, 0, ACS_LTEE);
  mvwaddch(w, m->height - 4, m->width - 1, ACS_RTEE);

  wattron(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
  mvwprintw(w, m->height - 3, 2,
            "Format: key: value  |  key: [a, b]  |  key: true");
  wattroff(w, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

  wattron(w, A_DIM);
  mvwprintw(w, m->height - 2, 2, "Tab");
  wattroff(w, A_DIM);
  mvwprintw(w, m->height - 2, 6, "import");
  wattron(w, A_DIM);
  mvwprintw(w, m->height - 2, 15, "Esc");
  wattroff(w, A_DIM);
  mvwprintw(w, m->height - 2, 19, "cancel");
  wattron(w, A_DIM);
  mvwprintw(w, m->height - 2, 28, "Enter");
  wattroff(w, A_DIM);
  mvwprintw(w, m->height - 2, 34, "newline");

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
  case MODAL_MODEL_EDIT:
    draw_model_edit(m);
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
  case MODAL_MESSAGE_EDIT:
    draw_message_edit(m);
    break;
  case MODAL_MESSAGE_DELETE_CONFIRM:
    draw_message_delete_confirm(m);
    break;
  case MODAL_SAMPLER_SETTINGS:
    draw_sampler_settings(m);
    break;
  case MODAL_SAMPLER_YAML:
    draw_sampler_yaml(m);
    break;
  default:
    break;
  }
}

static bool handle_field_key(Modal *m, int ch) {
  int fi = m->field_index;
  if (fi >= 5)
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
      m->field_index = (m->field_index + 1) % 8;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_BTAB || ch == KEY_UP) {
      m->field_index = (m->field_index + 7) % 8;
      return MODAL_RESULT_NONE;
    }

    if (m->field_index == 0) {
      if (ch == KEY_LEFT || ch == 'h') {
        m->api_type_selection =
            (m->api_type_selection + API_TYPE_COUNT - 1) % API_TYPE_COUNT;
        set_default_url_for_api(m);
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT || ch == 'l') {
        m->api_type_selection = (m->api_type_selection + 1) % API_TYPE_COUNT;
        set_default_url_for_api(m);
        return MODAL_RESULT_NONE;
      }
    }

    if (m->field_index == 4 && m->api_type_selection == API_TYPE_ANTHROPIC) {
      size_t count;
      const char **models = anthropic_get_models(&count);
      if (ch == KEY_LEFT || ch == 'h') {
        int current = -1;
        for (size_t i = 0; i < count; i++) {
          if (strcmp(m->fields[3], models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next = (current <= 0) ? (int)count - 1 : current - 1;
        strncpy(m->fields[3], models[next], sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT || ch == 'l') {
        int current = -1;
        for (size_t i = 0; i < count; i++) {
          if (strcmp(m->fields[3], models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next = (current < 0 || current >= (int)count - 1) ? 0 : current + 1;
        strncpy(m->fields[3], models[next], sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
    }

    bool is_openai_compat = (m->api_type_selection == API_TYPE_APHRODITE ||
                             m->api_type_selection == API_TYPE_VLLM ||
                             m->api_type_selection == API_TYPE_OPENAI ||
                             m->api_type_selection == API_TYPE_TABBY);

    if (m->field_index == 4 && is_openai_compat && ch == 'f') {
      if (!fetch_models_from_api(m)) {
        modal_open_message(m, "Failed to fetch models from API", true);
      }
      return MODAL_RESULT_NONE;
    }

    if (m->field_index == 4 && is_openai_compat &&
        m->fetched_models_count > 0) {
      if (ch == KEY_LEFT || ch == 'h') {
        int current = -1;
        for (size_t i = 0; i < m->fetched_models_count; i++) {
          if (strcmp(m->fields[3], m->fetched_models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next =
            (current <= 0) ? (int)m->fetched_models_count - 1 : current - 1;
        strncpy(m->fields[3], m->fetched_models[next],
                sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT || ch == 'l') {
        int current = -1;
        for (size_t i = 0; i < m->fetched_models_count; i++) {
          if (strcmp(m->fields[3], m->fetched_models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next = (current < 0 || current >= (int)m->fetched_models_count - 1)
                       ? 0
                       : current + 1;
        strncpy(m->fields[3], m->fetched_models[next],
                sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
    }

    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 7) {
        modal_close(m);
        return MODAL_RESULT_NONE;
      }
      if (m->field_index == 6) {
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
        snprintf(mc.name, sizeof(mc.name), "%.63s", m->fields[0]);
        snprintf(mc.base_url, sizeof(mc.base_url), "%.255s", m->fields[1]);
        snprintf(mc.api_key, sizeof(mc.api_key), "%.255s", m->fields[2]);
        snprintf(mc.model_id, sizeof(mc.model_id), "%.127s", m->fields[3]);
        mc.context_length = atoi(m->fields[4]);
        if (mc.context_length <= 0)
          mc.context_length = DEFAULT_CONTEXT_LENGTH;
        mc.api_type = m->api_type_selection;

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
      m->field_index = (m->field_index + 1) % 8;
      return MODAL_RESULT_NONE;
    }

    if (m->field_index >= 1 && m->field_index <= 5) {
      int fi = m->field_index - 1;
      char *field = m->fields[fi];
      int *cursor = &m->field_cursor[fi];
      int *len = &m->field_len[fi];
      int max_len = (int)sizeof(m->fields[0]) - 1;

      if (ch == KEY_LEFT) {
        if (*cursor > 0)
          (*cursor)--;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT) {
        if (*cursor < *len)
          (*cursor)++;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_HOME || ch == 1) {
        *cursor = 0;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_END || ch == 5) {
        *cursor = *len;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (*cursor > 0) {
          memmove(&field[*cursor - 1], &field[*cursor], *len - *cursor + 1);
          (*len)--;
          (*cursor)--;
        }
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_DC) {
        if (*cursor < *len) {
          memmove(&field[*cursor], &field[*cursor + 1], *len - *cursor);
          (*len)--;
        }
        return MODAL_RESULT_NONE;
      }
      if (isprint(ch) && *len < max_len) {
        memmove(&field[*cursor + 1], &field[*cursor], *len - *cursor + 1);
        field[*cursor] = (char)ch;
        (*len)++;
        (*cursor)++;
        return MODAL_RESULT_NONE;
      }
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
    if (ch == 'e') {
      if (mf->count > 0) {
        modal_open_model_edit(m, mf, m->list_selection);
      }
      return MODAL_RESULT_NONE;
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_MODEL_EDIT) {
    if (ch == 27) {
      modal_open_model_list(m, mf);
      return MODAL_RESULT_NONE;
    }

    if (ch == '\t' || ch == KEY_DOWN) {
      m->field_index = (m->field_index + 1) % 8;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_BTAB || ch == KEY_UP) {
      m->field_index = (m->field_index + 7) % 8;
      return MODAL_RESULT_NONE;
    }

    if (m->field_index == 0) {
      if (ch == KEY_LEFT || ch == 'h') {
        m->api_type_selection =
            (m->api_type_selection + API_TYPE_COUNT - 1) % API_TYPE_COUNT;
        set_default_url_for_api(m);
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT || ch == 'l') {
        m->api_type_selection = (m->api_type_selection + 1) % API_TYPE_COUNT;
        set_default_url_for_api(m);
        return MODAL_RESULT_NONE;
      }
    }

    if (m->field_index == 4 && m->api_type_selection == API_TYPE_ANTHROPIC) {
      size_t count;
      const char **models = anthropic_get_models(&count);
      if (ch == KEY_LEFT || ch == 'h') {
        int current = -1;
        for (size_t i = 0; i < count; i++) {
          if (strcmp(m->fields[3], models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next = (current <= 0) ? (int)count - 1 : current - 1;
        strncpy(m->fields[3], models[next], sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT || ch == 'l') {
        int current = -1;
        for (size_t i = 0; i < count; i++) {
          if (strcmp(m->fields[3], models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next = (current < 0 || current >= (int)count - 1) ? 0 : current + 1;
        strncpy(m->fields[3], models[next], sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
    }

    bool is_openai_compat_edit = (m->api_type_selection == API_TYPE_APHRODITE ||
                                  m->api_type_selection == API_TYPE_VLLM ||
                                  m->api_type_selection == API_TYPE_OPENAI ||
                                  m->api_type_selection == API_TYPE_TABBY);

    if (m->field_index == 4 && is_openai_compat_edit && ch == 'f') {
      if (!fetch_models_from_api(m)) {
        modal_open_message(m, "Failed to fetch models from API", true);
      }
      return MODAL_RESULT_NONE;
    }

    if (m->field_index == 4 && is_openai_compat_edit &&
        m->fetched_models_count > 0) {
      if (ch == KEY_LEFT || ch == 'h') {
        int current = -1;
        for (size_t i = 0; i < m->fetched_models_count; i++) {
          if (strcmp(m->fields[3], m->fetched_models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next =
            (current <= 0) ? (int)m->fetched_models_count - 1 : current - 1;
        strncpy(m->fields[3], m->fetched_models[next],
                sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT || ch == 'l') {
        int current = -1;
        for (size_t i = 0; i < m->fetched_models_count; i++) {
          if (strcmp(m->fields[3], m->fetched_models[i]) == 0) {
            current = (int)i;
            break;
          }
        }
        int next = (current < 0 || current >= (int)m->fetched_models_count - 1)
                       ? 0
                       : current + 1;
        strncpy(m->fields[3], m->fetched_models[next],
                sizeof(m->fields[3]) - 1);
        m->field_len[3] = (int)strlen(m->fields[3]);
        m->field_cursor[3] = m->field_len[3];
        return MODAL_RESULT_NONE;
      }
    }

    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 7) {
        modal_open_model_list(m, mf);
        return MODAL_RESULT_NONE;
      }
      if (m->field_index == 6) {
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

        ModelConfig *mc = &mf->models[m->edit_model_index];
        snprintf(mc->name, sizeof(mc->name), "%.63s", m->fields[0]);
        snprintf(mc->base_url, sizeof(mc->base_url), "%.255s", m->fields[1]);
        snprintf(mc->api_key, sizeof(mc->api_key), "%.255s", m->fields[2]);
        snprintf(mc->model_id, sizeof(mc->model_id), "%.127s", m->fields[3]);
        mc->context_length = atoi(m->fields[4]);
        if (mc->context_length <= 0)
          mc->context_length = DEFAULT_CONTEXT_LENGTH;
        mc->api_type = m->api_type_selection;

        config_save_models(mf);
        modal_open_message(m, "Model updated!", false);
        return MODAL_RESULT_NONE;
      }
      m->field_index = (m->field_index + 1) % 8;
      return MODAL_RESULT_NONE;
    }

    if (m->field_index >= 1 && m->field_index <= 5) {
      int fi = m->field_index - 1;
      char *field = m->fields[fi];
      int *cursor = &m->field_cursor[fi];
      int *len = &m->field_len[fi];
      int max_len = (int)sizeof(m->fields[0]) - 1;

      if (ch == KEY_LEFT) {
        if (*cursor > 0)
          (*cursor)--;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_RIGHT) {
        if (*cursor < *len)
          (*cursor)++;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_HOME || ch == 1) {
        *cursor = 0;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_END || ch == 5) {
        *cursor = *len;
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (*cursor > 0) {
          memmove(&field[*cursor - 1], &field[*cursor], *len - *cursor + 1);
          (*len)--;
          (*cursor)--;
        }
        return MODAL_RESULT_NONE;
      }
      if (ch == KEY_DC) {
        if (*cursor < *len) {
          memmove(&field[*cursor], &field[*cursor + 1], *len - *cursor);
          (*len)--;
        }
        return MODAL_RESULT_NONE;
      }
      if (isprint(ch) && *len < max_len) {
        memmove(&field[*cursor + 1], &field[*cursor], *len - *cursor + 1);
        field[*cursor] = (char)ch;
        (*len)++;
        (*cursor)++;
        return MODAL_RESULT_NONE;
      }
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
        const char *char_name =
            m->chat_list.chats[m->list_selection].character_name;
        if (chat_load(history, id, char_name, loaded_char_path,
                      char_path_size)) {
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
        const char *char_name =
            m->chat_list.chats[m->list_selection].character_name;
        chat_delete(id, char_name);
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
                   chat_get_title_by_id(m->current_chat_id, m->character_name,
                                        existing_title,
                                        sizeof(existing_title))) {
          title = existing_title;
        } else {
          title = chat_auto_title(history);
        }

        char existing_id[CHAT_ID_MAX] = {0};
        bool title_exists = chat_find_by_title(
            title, m->character_name, existing_id, sizeof(existing_id));

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

        if (chat_save(history, id, title, m->character_path,
                      m->character_name)) {
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
        chat_delete(m->existing_chat_id, m->character_name);

        const char *id = m->existing_chat_id;
        if (chat_save(history, id, m->pending_save_title, m->character_path,
                      m->character_name)) {
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

  if (m->type == MODAL_MESSAGE_EDIT) {
    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }
    if (ch == '\t') {
      m->field_index = 1 - m->field_index;
      return MODAL_RESULT_NONE;
    }
    if (ch == '\n' || ch == '\r') {
      if (m->field_index == 1) {
        modal_close(m);
        return MODAL_RESULT_NONE;
      }
      modal_close(m);
      return MODAL_RESULT_MESSAGE_EDITED;
    }

    if (ch == KEY_LEFT) {
      if (m->edit_cursor > 0)
        m->edit_cursor--;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_RIGHT) {
      if (m->edit_cursor < m->edit_len)
        m->edit_cursor++;
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_UP) {
      int text_w = m->width - 6;
      int line = 0, col = 0;
      for (int i = 0; i < m->edit_cursor; i++) {
        if (m->edit_buffer[i] == '\n') {
          line++;
          col = 0;
        } else {
          col++;
          if (col >= text_w) {
            line++;
            col = 0;
          }
        }
      }
      if (line > 0) {
        int target_line = line - 1;
        int cur_line = 0, cur_col = 0;
        for (int i = 0; i <= m->edit_len; i++) {
          if (cur_line == target_line && cur_col == col) {
            m->edit_cursor = i;
            break;
          }
          if (cur_line > target_line) {
            m->edit_cursor = i > 0 ? i - 1 : 0;
            break;
          }
          if (i < m->edit_len) {
            if (m->edit_buffer[i] == '\n') {
              if (cur_line == target_line) {
                m->edit_cursor = i;
                break;
              }
              cur_line++;
              cur_col = 0;
            } else {
              cur_col++;
              if (cur_col >= text_w) {
                if (cur_line == target_line) {
                  m->edit_cursor = i + 1;
                  break;
                }
                cur_line++;
                cur_col = 0;
              }
            }
          }
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_DOWN) {
      int text_w = m->width - 6;
      int line = 0, col = 0;
      for (int i = 0; i < m->edit_cursor; i++) {
        if (m->edit_buffer[i] == '\n') {
          line++;
          col = 0;
        } else {
          col++;
          if (col >= text_w) {
            line++;
            col = 0;
          }
        }
      }
      int target_line = line + 1;
      int cur_line = 0, cur_col = 0;
      for (int i = 0; i <= m->edit_len; i++) {
        if (cur_line == target_line && cur_col == col) {
          m->edit_cursor = i;
          break;
        }
        if (cur_line > target_line) {
          m->edit_cursor = i > 0 ? i - 1 : 0;
          break;
        }
        if (i < m->edit_len) {
          if (m->edit_buffer[i] == '\n') {
            if (cur_line == target_line) {
              m->edit_cursor = i;
              break;
            }
            cur_line++;
            cur_col = 0;
          } else {
            cur_col++;
            if (cur_col >= text_w) {
              if (cur_line == target_line) {
                m->edit_cursor = i + 1;
                break;
              }
              cur_line++;
              cur_col = 0;
            }
          }
        }
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (m->edit_cursor > 0) {
        memmove(&m->edit_buffer[m->edit_cursor - 1],
                &m->edit_buffer[m->edit_cursor],
                m->edit_len - m->edit_cursor + 1);
        m->edit_cursor--;
        m->edit_len--;
      }
      return MODAL_RESULT_NONE;
    }
    if (ch == KEY_DC) {
      if (m->edit_cursor < m->edit_len) {
        memmove(&m->edit_buffer[m->edit_cursor],
                &m->edit_buffer[m->edit_cursor + 1],
                m->edit_len - m->edit_cursor);
        m->edit_len--;
      }
      return MODAL_RESULT_NONE;
    }
    if ((ch >= 32 && ch < 127) || ch == '\n') {
      if (m->edit_len < (int)sizeof(m->edit_buffer) - 1) {
        memmove(&m->edit_buffer[m->edit_cursor + 1],
                &m->edit_buffer[m->edit_cursor],
                m->edit_len - m->edit_cursor + 1);
        m->edit_buffer[m->edit_cursor] = (char)ch;
        m->edit_cursor++;
        m->edit_len++;
      }
      return MODAL_RESULT_NONE;
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_MESSAGE_DELETE_CONFIRM) {
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
        modal_close(m);
        return MODAL_RESULT_MESSAGE_DELETED;
      } else {
        modal_close(m);
        return MODAL_RESULT_NONE;
      }
    }
    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_SAMPLER_YAML) {
    SamplerSettings *s = &m->sampler;

    if (ch == 27) {
      SamplerSettings saved = *s;
      ApiType saved_api = m->sampler_api_type;
      modal_open_sampler_settings(m, saved_api);
      m->sampler = saved;
      return MODAL_RESULT_NONE;
    }

    if (ch == '\t') {
      char *p = m->sampler_yaml_buffer;
      while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n')
          p++;
        if (!*p)
          break;

        char key[64] = {0};
        int ki = 0;
        while (*p && *p != ':' && *p != '\n' && ki < 63) {
          if (*p != ' ' && *p != '\t')
            key[ki++] = *p;
          p++;
        }
        key[ki] = '\0';

        if (*p != ':') {
          while (*p && *p != '\n')
            p++;
          continue;
        }
        p++;

        while (*p == ' ' || *p == '\t')
          p++;

        if (*p == '[') {
          p++;
          char first_item[64] = {0};
          int is_string_list = 0;
          int is_int_list = 1;
          double list_vals[MAX_LIST_ITEMS];
          char list_strs[MAX_LIST_ITEMS][64];
          int list_cnt = 0;

          while (*p && *p != ']' && list_cnt < MAX_LIST_ITEMS) {
            while (*p == ' ' || *p == '\t' || *p == ',')
              p++;
            if (*p == ']')
              break;

            if (*p == '"') {
              is_string_list = 1;
              is_int_list = 0;
              p++;
              int vi = 0;
              while (*p && *p != '"' && vi < 63) {
                list_strs[list_cnt][vi++] = *p++;
              }
              list_strs[list_cnt][vi] = '\0';
              if (*p == '"')
                p++;
              list_cnt++;
            } else if (*p == '-' || (*p >= '0' && *p <= '9')) {
              char num[32] = {0};
              int ni = 0;
              while (*p &&
                     ((*p >= '0' && *p <= '9') || *p == '.' || *p == '-' ||
                      *p == 'e' || *p == 'E') &&
                     ni < 31) {
                if (*p == '.')
                  is_int_list = 0;
                num[ni++] = *p++;
              }
              list_vals[list_cnt] = atof(num);
              if (list_cnt == 0)
                strncpy(first_item, num, 63);
              list_cnt++;
            } else {
              p++;
            }
          }
          if (*p == ']')
            p++;

          if (key[0] && list_cnt > 0) {
            SamplerValueType type = is_string_list ? SAMPLER_TYPE_LIST_STRING
                                    : is_int_list  ? SAMPLER_TYPE_LIST_INT
                                                   : SAMPLER_TYPE_LIST_FLOAT;
            sampler_add_custom(s, key, type, 0, NULL, 0, 100, 1);
            CustomSampler *cs = &s->custom[s->custom_count - 1];
            cs->list_count = list_cnt;
            for (int i = 0; i < list_cnt; i++) {
              cs->list_values[i] = list_vals[i];
              strncpy(cs->list_strings[i], list_strs[i], 63);
            }
          }
        } else if (*p == '{') {
          p++;
          DictEntry entries[MAX_DICT_ITEMS];
          int dict_cnt = 0;

          while (*p && *p != '}' && dict_cnt < MAX_DICT_ITEMS) {
            while (*p == ' ' || *p == '\t' || *p == ',' || *p == '\n')
              p++;
            if (*p == '}')
              break;

            char dkey[32] = {0};
            int dki = 0;
            if (*p == '"') {
              p++;
              while (*p && *p != '"' && dki < 31)
                dkey[dki++] = *p++;
              if (*p == '"')
                p++;
            } else {
              while (*p && *p != ':' && *p != ' ' && dki < 31)
                dkey[dki++] = *p++;
            }
            dkey[dki] = '\0';

            while (*p == ' ' || *p == '\t')
              p++;
            if (*p == ':')
              p++;
            while (*p == ' ' || *p == '\t')
              p++;

            if (*p == '"') {
              p++;
              int vi = 0;
              while (*p && *p != '"' && vi < 63) {
                entries[dict_cnt].str_val[vi++] = *p++;
              }
              entries[dict_cnt].str_val[vi] = '\0';
              if (*p == '"')
                p++;
              entries[dict_cnt].is_string = true;
            } else {
              char num[32] = {0};
              int ni = 0;
              while (*p &&
                     ((*p >= '0' && *p <= '9') || *p == '.' || *p == '-') &&
                     ni < 31)
                num[ni++] = *p++;
              entries[dict_cnt].num_val = atof(num);
              entries[dict_cnt].is_string = false;
            }
            strncpy(entries[dict_cnt].key, dkey, 31);
            dict_cnt++;
          }
          if (*p == '}')
            p++;

          if (key[0] && dict_cnt > 0) {
            sampler_add_custom(s, key, SAMPLER_TYPE_DICT, 0, NULL, 0, 100, 1);
            CustomSampler *cs = &s->custom[s->custom_count - 1];
            cs->dict_count = dict_cnt;
            for (int i = 0; i < dict_cnt; i++) {
              cs->dict_entries[i] = entries[i];
            }
          }
        } else if (strncmp(p, "true", 4) == 0 || strncmp(p, "false", 5) == 0) {
          bool bval = (*p == 't');
          if (key[0]) {
            sampler_add_custom(s, key, SAMPLER_TYPE_BOOL, bval ? 1.0 : 0.0,
                               NULL, 0, 1, 1);
          }
          while (*p && *p != '\n')
            p++;
        } else if (*p == '"') {
          p++;
          char val[256] = {0};
          int vi = 0;
          while (*p && *p != '"' && *p != '\n' && vi < 255)
            val[vi++] = *p++;
          val[vi] = '\0';
          if (*p == '"')
            p++;
          if (key[0]) {
            sampler_add_custom(s, key, SAMPLER_TYPE_STRING, 0, val, 0, 100, 1);
          }
        } else if (*p == '-' || (*p >= '0' && *p <= '9')) {
          char num[32] = {0};
          int ni = 0;
          bool is_float = false;
          while (*p &&
                 ((*p >= '0' && *p <= '9') || *p == '.' || *p == '-' ||
                  *p == 'e' || *p == 'E') &&
                 ni < 31) {
            if (*p == '.' || *p == 'e' || *p == 'E')
              is_float = true;
            num[ni++] = *p++;
          }
          double val = atof(num);
          if (key[0]) {
            bool added = sampler_add_custom(
                s, key, is_float ? SAMPLER_TYPE_FLOAT : SAMPLER_TYPE_INT, val,
                NULL, 0, 100, is_float ? 0.1 : 1);
            (void)added;
          }
        }

        while (*p && *p != '\n')
          p++;
      }

      SamplerSettings saved = *s;
      ApiType saved_api = m->sampler_api_type;
      modal_open_sampler_settings(m, saved_api);
      m->sampler = saved;
      return MODAL_RESULT_NONE;
    }

    if (ch == '\n' || ch == '\r') {
      int len = (int)strlen(m->sampler_yaml_buffer);
      if (len < 4094) {
        memmove(&m->sampler_yaml_buffer[m->sampler_yaml_cursor + 1],
                &m->sampler_yaml_buffer[m->sampler_yaml_cursor],
                len - m->sampler_yaml_cursor + 1);
        m->sampler_yaml_buffer[m->sampler_yaml_cursor] = '\n';
        m->sampler_yaml_cursor++;
      }
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_BACKSPACE || ch == 127) {
      int len = (int)strlen(m->sampler_yaml_buffer);
      if (m->sampler_yaml_cursor > 0 && len > 0) {
        memmove(&m->sampler_yaml_buffer[m->sampler_yaml_cursor - 1],
                &m->sampler_yaml_buffer[m->sampler_yaml_cursor],
                len - m->sampler_yaml_cursor + 1);
        m->sampler_yaml_cursor--;
      }
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_LEFT) {
      if (m->sampler_yaml_cursor > 0)
        m->sampler_yaml_cursor--;
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_RIGHT) {
      if (m->sampler_yaml_cursor < (int)strlen(m->sampler_yaml_buffer))
        m->sampler_yaml_cursor++;
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_UP) {
      int pos = m->sampler_yaml_cursor;
      int col = 0;
      while (pos > 0 && m->sampler_yaml_buffer[pos - 1] != '\n') {
        pos--;
        col++;
      }
      if (pos > 0) {
        pos--;
        int prev_line_start = pos;
        while (prev_line_start > 0 &&
               m->sampler_yaml_buffer[prev_line_start - 1] != '\n')
          prev_line_start--;
        int prev_line_len = pos - prev_line_start;
        if (col > prev_line_len)
          col = prev_line_len;
        m->sampler_yaml_cursor = prev_line_start + col;
      }
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_DOWN) {
      int len = (int)strlen(m->sampler_yaml_buffer);
      int pos = m->sampler_yaml_cursor;
      int col = 0;
      while (pos > 0 && m->sampler_yaml_buffer[pos - 1] != '\n') {
        pos--;
        col++;
      }
      pos = m->sampler_yaml_cursor;
      while (pos < len && m->sampler_yaml_buffer[pos] != '\n')
        pos++;
      if (pos < len) {
        pos++;
        int next_line_start = pos;
        while (pos < len && m->sampler_yaml_buffer[pos] != '\n')
          pos++;
        int next_line_len = pos - next_line_start;
        if (col > next_line_len)
          col = next_line_len;
        m->sampler_yaml_cursor = next_line_start + col;
      }
      return MODAL_RESULT_NONE;
    }

    if (ch >= 32 && ch < 127) {
      int len = (int)strlen(m->sampler_yaml_buffer);
      if (len < 4094) {
        memmove(&m->sampler_yaml_buffer[m->sampler_yaml_cursor + 1],
                &m->sampler_yaml_buffer[m->sampler_yaml_cursor],
                len - m->sampler_yaml_cursor + 1);
        m->sampler_yaml_buffer[m->sampler_yaml_cursor] = (char)ch;
        m->sampler_yaml_cursor++;
      }
      return MODAL_RESULT_NONE;
    }

    return MODAL_RESULT_NONE;
  }

  if (m->type == MODAL_SAMPLER_SETTINGS) {
    SamplerSettings *s = &m->sampler;

    if (m->sampler_adding_custom) {
      if (ch == 27) {
        m->sampler_adding_custom = false;
        return MODAL_RESULT_NONE;
      }
      if (ch == '\t') {
        int max_fields;
        if (m->sampler_custom_type >= SAMPLER_TYPE_LIST_FLOAT ||
            m->sampler_custom_type == SAMPLER_TYPE_DICT)
          max_fields = 2;
        else if (m->sampler_custom_type == SAMPLER_TYPE_STRING ||
                 m->sampler_custom_type == SAMPLER_TYPE_BOOL)
          max_fields = 3;
        else
          max_fields = 6;
        m->sampler_custom_field = (m->sampler_custom_field + 1) % max_fields;
        char *bufs[] = {m->sampler_custom_name,  NULL,
                        m->sampler_custom_value, m->sampler_custom_min,
                        m->sampler_custom_max,   m->sampler_custom_step};
        if (bufs[m->sampler_custom_field])
          m->sampler_custom_cursor = (int)strlen(bufs[m->sampler_custom_field]);
        else
          m->sampler_custom_cursor = 0;
        return MODAL_RESULT_NONE;
      }
      if ((ch == KEY_LEFT) && m->sampler_custom_field == 1) {
        int t = (int)m->sampler_custom_type;
        t = (t - 1 + 8) % 8;
        m->sampler_custom_type = (SamplerValueType)t;
        return MODAL_RESULT_NONE;
      }
      if ((ch == KEY_RIGHT) && m->sampler_custom_field == 1) {
        int t = (int)m->sampler_custom_type;
        t = (t + 1) % 8;
        m->sampler_custom_type = (SamplerValueType)t;
        return MODAL_RESULT_NONE;
      }
      if ((ch == KEY_LEFT || ch == KEY_RIGHT) && m->sampler_custom_field == 2 &&
          m->sampler_custom_type == SAMPLER_TYPE_BOOL) {
        bool bval = (m->sampler_custom_value[0] == 't' ||
                     m->sampler_custom_value[0] == 'T' ||
                     m->sampler_custom_value[0] == '1');
        strcpy(m->sampler_custom_value, bval ? "false" : "true");
        return MODAL_RESULT_NONE;
      }
      if (ch == '\n' || ch == '\r') {
        bool is_list = (m->sampler_custom_type >= SAMPLER_TYPE_LIST_FLOAT &&
                        m->sampler_custom_type <= SAMPLER_TYPE_LIST_STRING);
        bool is_dict = (m->sampler_custom_type == SAMPLER_TYPE_DICT);
        bool is_bool = (m->sampler_custom_type == SAMPLER_TYPE_BOOL);
        bool valid =
            m->sampler_custom_name[0] &&
            (is_list || is_dict || is_bool || m->sampler_custom_value[0]);
        if (valid) {
          double val;
          if (is_bool) {
            val = (m->sampler_custom_value[0] == 't' ||
                   m->sampler_custom_value[0] == 'T' ||
                   m->sampler_custom_value[0] == '1')
                      ? 1.0
                      : 0.0;
          } else {
            val = atof(m->sampler_custom_value);
          }
          double min_v =
              m->sampler_custom_min[0] ? atof(m->sampler_custom_min) : 0;
          double max_v =
              m->sampler_custom_max[0] ? atof(m->sampler_custom_max) : 100;
          double step =
              m->sampler_custom_step[0] ? atof(m->sampler_custom_step) : 0;
          const char *str_val = (m->sampler_custom_type == SAMPLER_TYPE_STRING)
                                    ? m->sampler_custom_value
                                    : NULL;
          sampler_add_custom(s, m->sampler_custom_name, m->sampler_custom_type,
                             val, str_val, min_v, max_v, step);
          int base_count = 27;
          m->sampler_field_index = base_count + s->custom_count - 1;
          int visible = m->height - 6;
          if (m->sampler_field_index >= m->sampler_scroll + visible)
            m->sampler_scroll = m->sampler_field_index - visible + 1;
          m->sampler_adding_custom = false;
          if (is_list) {
            m->sampler_editing_list = true;
            m->sampler_list_index = 0;
            m->sampler_list_scroll = 0;
            m->sampler_list_input[0] = '\0';
            m->sampler_list_input_cursor = 0;
          } else if (is_dict) {
            m->sampler_editing_dict = true;
            m->sampler_dict_index = 0;
            m->sampler_dict_scroll = 0;
            m->sampler_dict_field = 0;
            m->sampler_dict_key[0] = '\0';
            m->sampler_dict_val[0] = '\0';
            m->sampler_dict_cursor = 0;
            m->sampler_dict_val_is_str = true;
          }
          return MODAL_RESULT_NONE;
        }
        m->sampler_adding_custom = false;
        return MODAL_RESULT_NONE;
      }
      if (m->sampler_custom_field == 1) {
        return MODAL_RESULT_NONE;
      }
      char *bufs[] = {m->sampler_custom_name,  NULL,
                      m->sampler_custom_value, m->sampler_custom_min,
                      m->sampler_custom_max,   m->sampler_custom_step};
      int maxlens[] = {63, 0, 255, 31, 31, 31};
      char *buf = bufs[m->sampler_custom_field];
      int maxlen = maxlens[m->sampler_custom_field];
      if (ch == KEY_BACKSPACE || ch == 127) {
        int len = (int)strlen(buf);
        if (m->sampler_custom_cursor > 0 && len > 0) {
          memmove(&buf[m->sampler_custom_cursor - 1],
                  &buf[m->sampler_custom_cursor],
                  len - m->sampler_custom_cursor + 1);
          m->sampler_custom_cursor--;
        }
        return MODAL_RESULT_NONE;
      }
      if (ch >= 32 && ch < 127) {
        int len = (int)strlen(buf);
        if (len < maxlen) {
          memmove(&buf[m->sampler_custom_cursor + 1],
                  &buf[m->sampler_custom_cursor],
                  len - m->sampler_custom_cursor + 1);
          buf[m->sampler_custom_cursor] = (char)ch;
          m->sampler_custom_cursor++;
        }
        return MODAL_RESULT_NONE;
      }
      return MODAL_RESULT_NONE;
    }

    if (m->sampler_editing_list) {
      int base_count = 27;
      int idx = m->sampler_field_index;
      if (idx >= base_count) {
        CustomSampler *cs = &s->custom[idx - base_count];

        if (ch == 27) {
          m->sampler_editing_list = false;
          m->sampler_list_input[0] = '\0';
          return MODAL_RESULT_NONE;
        }

        if (ch == KEY_UP) {
          if (m->sampler_list_input[0]) {
            if (m->sampler_list_index < cs->list_count) {
              if (cs->type == SAMPLER_TYPE_LIST_STRING) {
                strncpy(cs->list_strings[m->sampler_list_index],
                        m->sampler_list_input, 63);
              } else {
                cs->list_values[m->sampler_list_index] =
                    atof(m->sampler_list_input);
              }
            }
            m->sampler_list_input[0] = '\0';
            m->sampler_list_input_cursor = 0;
          }
          if (m->sampler_list_index > 0)
            m->sampler_list_index--;
          if (m->sampler_list_index < m->sampler_list_scroll)
            m->sampler_list_scroll = m->sampler_list_index;
          return MODAL_RESULT_NONE;
        }

        if (ch == KEY_DOWN) {
          if (m->sampler_list_input[0]) {
            if (m->sampler_list_index < cs->list_count) {
              if (cs->type == SAMPLER_TYPE_LIST_STRING) {
                strncpy(cs->list_strings[m->sampler_list_index],
                        m->sampler_list_input, 63);
              } else {
                cs->list_values[m->sampler_list_index] =
                    atof(m->sampler_list_input);
              }
            }
            m->sampler_list_input[0] = '\0';
            m->sampler_list_input_cursor = 0;
          }
          if (m->sampler_list_index < cs->list_count)
            m->sampler_list_index++;
          int visible = 11;
          if (m->sampler_list_index >= m->sampler_list_scroll + visible)
            m->sampler_list_scroll = m->sampler_list_index - visible + 1;
          return MODAL_RESULT_NONE;
        }

        if (ch == '\n' || ch == '\r') {
          if (m->sampler_list_index == cs->list_count) {
            if (cs->list_count < MAX_LIST_ITEMS) {
              if (cs->type == SAMPLER_TYPE_LIST_STRING)
                cs->list_strings[cs->list_count][0] = '\0';
              else
                cs->list_values[cs->list_count] = 0;
              cs->list_count++;
              m->sampler_list_input[0] = '\0';
              m->sampler_list_input_cursor = 0;
            }
          } else if (m->sampler_list_input[0]) {
            if (cs->type == SAMPLER_TYPE_LIST_STRING) {
              strncpy(cs->list_strings[m->sampler_list_index],
                      m->sampler_list_input, 63);
            } else {
              cs->list_values[m->sampler_list_index] =
                  atof(m->sampler_list_input);
            }
            m->sampler_list_input[0] = '\0';
            m->sampler_list_input_cursor = 0;
          }
          return MODAL_RESULT_NONE;
        }

        if ((ch == 'd' || ch == 'D') &&
            m->sampler_list_index < cs->list_count) {
          for (int i = m->sampler_list_index; i < cs->list_count - 1; i++) {
            cs->list_values[i] = cs->list_values[i + 1];
            strncpy(cs->list_strings[i], cs->list_strings[i + 1], 63);
          }
          cs->list_count--;
          if (m->sampler_list_index >= cs->list_count && cs->list_count > 0)
            m->sampler_list_index = cs->list_count - 1;
          if (m->sampler_list_index < 0)
            m->sampler_list_index = 0;
          return MODAL_RESULT_NONE;
        }

        if (ch == KEY_BACKSPACE || ch == 127) {
          if (m->sampler_list_input_cursor > 0) {
            int len = (int)strlen(m->sampler_list_input);
            memmove(&m->sampler_list_input[m->sampler_list_input_cursor - 1],
                    &m->sampler_list_input[m->sampler_list_input_cursor],
                    len - m->sampler_list_input_cursor + 1);
            m->sampler_list_input_cursor--;
          }
          return MODAL_RESULT_NONE;
        }

        if (ch >= 32 && ch < 127 && m->sampler_list_index < cs->list_count) {
          int len = (int)strlen(m->sampler_list_input);
          if (len < 255) {
            memmove(&m->sampler_list_input[m->sampler_list_input_cursor + 1],
                    &m->sampler_list_input[m->sampler_list_input_cursor],
                    len - m->sampler_list_input_cursor + 1);
            m->sampler_list_input[m->sampler_list_input_cursor] = (char)ch;
            m->sampler_list_input_cursor++;
          }
          return MODAL_RESULT_NONE;
        }

        return MODAL_RESULT_NONE;
      }
      m->sampler_editing_list = false;
      return MODAL_RESULT_NONE;
    }

    if (m->sampler_editing_dict) {
      int base_count = 27;
      int idx = m->sampler_field_index;
      if (idx >= base_count) {
        CustomSampler *cs = &s->custom[idx - base_count];

        if (ch == 27) {
          m->sampler_editing_dict = false;
          m->sampler_dict_key[0] = '\0';
          m->sampler_dict_val[0] = '\0';
          return MODAL_RESULT_NONE;
        }

        if (ch == KEY_UP) {
          if (m->sampler_dict_index > 0)
            m->sampler_dict_index--;
          if (m->sampler_dict_index < m->sampler_dict_scroll)
            m->sampler_dict_scroll = m->sampler_dict_index;
          return MODAL_RESULT_NONE;
        }

        if (ch == KEY_DOWN) {
          if (m->sampler_dict_index < cs->dict_count)
            m->sampler_dict_index++;
          int visible = 9;
          if (m->sampler_dict_index >= m->sampler_dict_scroll + visible)
            m->sampler_dict_scroll = m->sampler_dict_index - visible + 1;
          return MODAL_RESULT_NONE;
        }

        if (ch == 'd' && m->sampler_dict_index < cs->dict_count &&
            cs->dict_count > 0) {
          for (int i = m->sampler_dict_index; i < cs->dict_count - 1; i++) {
            cs->dict_entries[i] = cs->dict_entries[i + 1];
          }
          cs->dict_count--;
          if (m->sampler_dict_index >= cs->dict_count && cs->dict_count > 0)
            m->sampler_dict_index = cs->dict_count - 1;
          if (m->sampler_dict_index < 0)
            m->sampler_dict_index = 0;
          return MODAL_RESULT_NONE;
        }

        if ((ch == '\n' || ch == '\r') &&
            m->sampler_dict_index == cs->dict_count) {
          if (cs->dict_count < MAX_DICT_ITEMS) {
            m->sampler_dict_field = 0;
            m->sampler_dict_key[0] = '\0';
            m->sampler_dict_val[0] = '\0';
            m->sampler_dict_cursor = 0;
            m->sampler_dict_val_is_str = true;

            WINDOW *dwin = m->win;
            int dw = 40, ddh = 8;
            int ddx = (m->width - dw) / 2;
            int ddy = (m->height - ddh) / 2;
            bool editing = true;
            while (editing) {
              for (int yy = ddy; yy < ddy + ddh; yy++)
                mvwhline(dwin, yy, ddx, ' ', dw);
              wattron(dwin, COLOR_PAIR(COLOR_PAIR_BORDER));
              for (int yy = ddy; yy < ddy + ddh; yy++) {
                mvwaddstr(dwin, yy, ddx, "│");
                mvwaddstr(dwin, yy, ddx + dw - 1, "│");
              }
              mvwhline(dwin, ddy, ddx + 1, ACS_HLINE, dw - 2);
              mvwhline(dwin, ddy + ddh - 1, ddx + 1, ACS_HLINE, dw - 2);
              mvwaddch(dwin, ddy, ddx, ACS_ULCORNER);
              mvwaddch(dwin, ddy, ddx + dw - 1, ACS_URCORNER);
              mvwaddch(dwin, ddy + ddh - 1, ddx, ACS_LLCORNER);
              mvwaddch(dwin, ddy + ddh - 1, ddx + dw - 1, ACS_LRCORNER);
              wattroff(dwin, COLOR_PAIR(COLOR_PAIR_BORDER));

              wattron(dwin, A_BOLD);
              mvwprintw(dwin, ddy, ddx + (dw - 15) / 2, " Add Dict Entry ");
              wattroff(dwin, A_BOLD);

              mvwprintw(dwin, ddy + 2, ddx + 2, "Key:");
              if (m->sampler_dict_field == 0)
                wattron(dwin, A_REVERSE);
              mvwprintw(dwin, ddy + 2, ddx + 8, "%-28.28s",
                        m->sampler_dict_key);
              if (m->sampler_dict_field == 0)
                wattroff(dwin, A_REVERSE);

              mvwprintw(dwin, ddy + 3, ddx + 2, "Type:");
              if (m->sampler_dict_field == 1)
                wattron(dwin, COLOR_PAIR(COLOR_PAIR_SWIPE));
              mvwprintw(dwin, ddy + 3, ddx + 8, "←");
              wattroff(dwin, COLOR_PAIR(COLOR_PAIR_SWIPE));
              if (m->sampler_dict_val_is_str)
                wattron(dwin, A_REVERSE);
              mvwprintw(dwin, ddy + 3, ddx + 10, "str");
              if (m->sampler_dict_val_is_str)
                wattroff(dwin, A_REVERSE);
              if (!m->sampler_dict_val_is_str)
                wattron(dwin, A_REVERSE);
              mvwprintw(dwin, ddy + 3, ddx + 15, "num");
              if (!m->sampler_dict_val_is_str)
                wattroff(dwin, A_REVERSE);
              if (m->sampler_dict_field == 1)
                wattron(dwin, COLOR_PAIR(COLOR_PAIR_SWIPE));
              mvwprintw(dwin, ddy + 3, ddx + 19, "→");
              wattroff(dwin, COLOR_PAIR(COLOR_PAIR_SWIPE));

              mvwprintw(dwin, ddy + 4, ddx + 2, "Value:");
              if (m->sampler_dict_field == 2)
                wattron(dwin, A_REVERSE);
              mvwprintw(dwin, ddy + 4, ddx + 9, "%-27.27s",
                        m->sampler_dict_val);
              if (m->sampler_dict_field == 2)
                wattroff(dwin, A_REVERSE);

              wattron(dwin, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);
              mvwprintw(dwin, ddy + ddh - 2, ddx + 2,
                        "Tab:next Enter:save Esc:cancel");
              wattroff(dwin, COLOR_PAIR(COLOR_PAIR_HINT) | A_DIM);

              wrefresh(dwin);
              int dch = wgetch(dwin);

              if (dch == 27) {
                editing = false;
              } else if (dch == '\t') {
                m->sampler_dict_field = (m->sampler_dict_field + 1) % 3;
                m->sampler_dict_cursor = 0;
              } else if (dch == KEY_LEFT && m->sampler_dict_field == 1) {
                m->sampler_dict_val_is_str = !m->sampler_dict_val_is_str;
              } else if (dch == KEY_RIGHT && m->sampler_dict_field == 1) {
                m->sampler_dict_val_is_str = !m->sampler_dict_val_is_str;
              } else if (dch == '\n' || dch == '\r') {
                if (m->sampler_dict_key[0] && m->sampler_dict_val[0]) {
                  DictEntry *de = &cs->dict_entries[cs->dict_count];
                  strncpy(de->key, m->sampler_dict_key, DICT_KEY_LEN - 1);
                  de->is_string = m->sampler_dict_val_is_str;
                  if (de->is_string)
                    strncpy(de->str_val, m->sampler_dict_val, DICT_VAL_LEN - 1);
                  else
                    de->num_val = atof(m->sampler_dict_val);
                  cs->dict_count++;
                  m->sampler_dict_index = cs->dict_count;
                }
                editing = false;
              } else if (dch == KEY_BACKSPACE || dch == 127) {
                char *buf = (m->sampler_dict_field == 0) ? m->sampler_dict_key
                                                         : m->sampler_dict_val;
                int len = (int)strlen(buf);
                if (len > 0)
                  buf[len - 1] = '\0';
              } else if (dch >= 32 && dch < 127 && m->sampler_dict_field != 1) {
                char *buf = (m->sampler_dict_field == 0) ? m->sampler_dict_key
                                                         : m->sampler_dict_val;
                int maxlen = (m->sampler_dict_field == 0) ? DICT_KEY_LEN - 1
                                                          : DICT_VAL_LEN - 1;
                int len = (int)strlen(buf);
                if (len < maxlen) {
                  buf[len] = (char)dch;
                  buf[len + 1] = '\0';
                }
              }
            }
          }
          return MODAL_RESULT_NONE;
        }

        return MODAL_RESULT_NONE;
      }
      m->sampler_editing_dict = false;
      return MODAL_RESULT_NONE;
    }

    double *dvals[] = {
        &s->temperature,
        &s->top_p,
        NULL,
        &s->min_p,
        &s->frequency_penalty,
        &s->presence_penalty,
        &s->repetition_penalty,
        &s->typical_p,
        &s->tfs,
        &s->top_a,
        &s->smoothing_factor,
        NULL,
        NULL,
        &s->dynatemp_min,
        &s->dynatemp_max,
        &s->dynatemp_exponent,
        NULL,
        &s->mirostat_tau,
        &s->mirostat_eta,
        &s->dry_multiplier,
        &s->dry_base,
        NULL,
        NULL,
        &s->xtc_threshold,
        &s->xtc_probability,
        &s->nsigma,
        &s->skew,
    };
    int *ivals[] = {
        NULL,
        NULL,
        &s->top_k,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &s->min_tokens,
        &s->max_tokens,
        NULL,
        NULL,
        NULL,
        &s->mirostat_mode,
        NULL,
        NULL,
        NULL,
        NULL,
        &s->dry_allowed_length,
        &s->dry_range,
        NULL,
        NULL,
        NULL,
        NULL,
    };
    double steps[] = {
        0.05, 0.05, 1,    0.01, 0.05, 0.05, 0.05, 0.05, 0.05,
        0.01, 0.1,  1,    64,   0.05, 0.05, 0.1,  1,    0.1,
        0.01, 0.05, 0.05, 1,    64,   0.01, 0.01, 0.1,  0.1,
    };
    int base_field_count = 27;
    int total_fields = base_field_count + s->custom_count;

    if (ch == 27) {
      modal_close(m);
      return MODAL_RESULT_NONE;
    }

    if (ch == '\t') {
      sampler_save(&m->sampler, m->sampler_api_type);
      modal_close(m);
      return MODAL_RESULT_SAMPLER_SAVED;
    }

    if (ch == '+') {
      m->sampler_adding_custom = true;
      m->sampler_custom_field = 0;
      m->sampler_custom_name[0] = '\0';
      m->sampler_custom_value[0] = '\0';
      m->sampler_custom_min[0] = '\0';
      m->sampler_custom_max[0] = '\0';
      m->sampler_custom_step[0] = '\0';
      m->sampler_custom_type = SAMPLER_TYPE_FLOAT;
      m->sampler_custom_cursor = 0;
      return MODAL_RESULT_NONE;
    }

    if (ch == 'y' || ch == 'Y') {
      SamplerSettings saved_sampler = m->sampler;
      ApiType saved_api = m->sampler_api_type;
      modal_open_sampler_yaml(m, &saved_sampler, saved_api);
      return MODAL_RESULT_NONE;
    }

    if (ch == 'd' || ch == 'D') {
      int idx = m->sampler_field_index;
      if (idx >= base_field_count) {
        int custom_idx = idx - base_field_count;
        sampler_remove_custom(s, custom_idx);
        if (m->sampler_field_index >= base_field_count + s->custom_count &&
            m->sampler_field_index > 0) {
          m->sampler_field_index--;
        }
      }
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_UP) {
      if (m->sampler_input[0]) {
        double val = atof(m->sampler_input);
        int idx = m->sampler_field_index;
        if (idx < base_field_count) {
          if (dvals[idx])
            *dvals[idx] = val;
          else if (ivals[idx])
            *ivals[idx] = (int)val;
        } else {
          s->custom[idx - base_field_count].value = val;
        }
        m->sampler_input[0] = '\0';
        m->sampler_input_cursor = 0;
      }
      if (m->sampler_field_index > 0) {
        m->sampler_field_index--;
        if (m->sampler_field_index < m->sampler_scroll)
          m->sampler_scroll = m->sampler_field_index;
      }
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_DOWN) {
      if (m->sampler_input[0]) {
        double val = atof(m->sampler_input);
        int idx = m->sampler_field_index;
        if (idx < base_field_count) {
          if (dvals[idx])
            *dvals[idx] = val;
          else if (ivals[idx])
            *ivals[idx] = (int)val;
        } else {
          s->custom[idx - base_field_count].value = val;
        }
        m->sampler_input[0] = '\0';
        m->sampler_input_cursor = 0;
      }
      if (m->sampler_field_index < total_fields - 1) {
        m->sampler_field_index++;
        int visible = m->height - 6;
        if (m->sampler_field_index >= m->sampler_scroll + visible)
          m->sampler_scroll = m->sampler_field_index - visible + 1;
      }
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_LEFT) {
      int idx = m->sampler_field_index;
      if (idx < base_field_count) {
        double step = steps[idx];
        if (dvals[idx])
          *dvals[idx] -= step;
        else if (ivals[idx])
          *ivals[idx] -= (int)step;
      } else {
        CustomSampler *cs = &s->custom[idx - base_field_count];
        if (cs->type != SAMPLER_TYPE_STRING) {
          cs->value -= cs->step;
          if (cs->value < cs->min_val)
            cs->value = cs->min_val;
        }
      }
      m->sampler_input[0] = '\0';
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_RIGHT) {
      int idx = m->sampler_field_index;
      if (idx < base_field_count) {
        double step = steps[idx];
        if (dvals[idx])
          *dvals[idx] += step;
        else if (ivals[idx])
          *ivals[idx] += (int)step;
      } else {
        CustomSampler *cs = &s->custom[idx - base_field_count];
        if (cs->type != SAMPLER_TYPE_STRING) {
          cs->value += cs->step;
          if (cs->value > cs->max_val)
            cs->value = cs->max_val;
        }
      }
      m->sampler_input[0] = '\0';
      return MODAL_RESULT_NONE;
    }

    if (ch == '\n' || ch == '\r') {
      int idx = m->sampler_field_index;
      if (idx >= base_field_count) {
        CustomSampler *cs = &s->custom[idx - base_field_count];
        if (cs->type >= SAMPLER_TYPE_LIST_FLOAT &&
            cs->type <= SAMPLER_TYPE_LIST_STRING && !m->sampler_input[0]) {
          m->sampler_editing_list = true;
          m->sampler_list_index = 0;
          m->sampler_list_scroll = 0;
          m->sampler_list_input[0] = '\0';
          m->sampler_list_input_cursor = 0;
          return MODAL_RESULT_NONE;
        }
        if (cs->type == SAMPLER_TYPE_DICT && !m->sampler_input[0]) {
          m->sampler_editing_dict = true;
          m->sampler_dict_index = 0;
          m->sampler_dict_scroll = 0;
          m->sampler_dict_key[0] = '\0';
          m->sampler_dict_val[0] = '\0';
          m->sampler_dict_cursor = 0;
          m->sampler_dict_val_is_str = true;
          return MODAL_RESULT_NONE;
        }
        if (cs->type == SAMPLER_TYPE_BOOL && !m->sampler_input[0]) {
          cs->value = (cs->value != 0) ? 0.0 : 1.0;
          return MODAL_RESULT_NONE;
        }
      }
      if (m->sampler_input[0]) {
        if (idx < base_field_count) {
          double val = atof(m->sampler_input);
          if (dvals[idx])
            *dvals[idx] = val;
          else if (ivals[idx])
            *ivals[idx] = (int)val;
        } else {
          CustomSampler *cs = &s->custom[idx - base_field_count];
          if (cs->type == SAMPLER_TYPE_STRING) {
            strncpy(cs->str_value, m->sampler_input,
                    CUSTOM_SAMPLER_STR_LEN - 1);
            cs->str_value[CUSTOM_SAMPLER_STR_LEN - 1] = '\0';
          } else if (cs->type >= SAMPLER_TYPE_LIST_FLOAT) {
            cs->list_count = 0;
            char *copy = strdup(m->sampler_input);
            char *tok = strtok(copy, ",");
            while (tok && cs->list_count < MAX_LIST_ITEMS) {
              while (*tok == ' ')
                tok++;
              if (cs->type == SAMPLER_TYPE_LIST_STRING) {
                strncpy(cs->list_strings[cs->list_count], tok, 63);
                cs->list_strings[cs->list_count][63] = '\0';
                char *end = cs->list_strings[cs->list_count] +
                            strlen(cs->list_strings[cs->list_count]) - 1;
                while (end > cs->list_strings[cs->list_count] && *end == ' ')
                  *end-- = '\0';
              } else {
                cs->list_values[cs->list_count] = atof(tok);
              }
              cs->list_count++;
              tok = strtok(NULL, ",");
            }
            free(copy);
          } else {
            cs->value = atof(m->sampler_input);
          }
        }
        m->sampler_input[0] = '\0';
        m->sampler_input_cursor = 0;
      }
      return MODAL_RESULT_NONE;
    }

    if (ch == KEY_BACKSPACE || ch == 127) {
      if (m->sampler_input_cursor > 0) {
        memmove(&m->sampler_input[m->sampler_input_cursor - 1],
                &m->sampler_input[m->sampler_input_cursor],
                strlen(m->sampler_input) - m->sampler_input_cursor + 1);
        m->sampler_input_cursor--;
      }
      return MODAL_RESULT_NONE;
    }

    bool is_text_sampler = false;
    int idx = m->sampler_field_index;
    if (idx >= base_field_count && idx < total_fields) {
      CustomSampler *cs = &s->custom[idx - base_field_count];
      is_text_sampler = (cs->type == SAMPLER_TYPE_STRING ||
                         cs->type >= SAMPLER_TYPE_LIST_FLOAT);
    }

    bool valid_char = (ch >= '0' && ch <= '9') || ch == '.' || ch == '-';
    if (is_text_sampler)
      valid_char = (ch >= 32 && ch < 127);

    if (valid_char) {
      int len = (int)strlen(m->sampler_input);
      int maxlen = is_text_sampler ? 255 : (int)sizeof(m->sampler_input) - 1;
      if (len < maxlen) {
        memmove(&m->sampler_input[m->sampler_input_cursor + 1],
                &m->sampler_input[m->sampler_input_cursor],
                len - m->sampler_input_cursor + 1);
        m->sampler_input[m->sampler_input_cursor] = (char)ch;
        m->sampler_input_cursor++;
      }
      return MODAL_RESULT_NONE;
    }

    return MODAL_RESULT_NONE;
  }

  return MODAL_RESULT_NONE;
}

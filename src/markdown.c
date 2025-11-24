#include "markdown.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define STYLE_STACK_MAX 64

typedef enum {
  STYLE_ITALIC = 1 << 0,
  STYLE_QUOTE = 1 << 1,
  STYLE_BOLD = 1 << 2,
  STYLE_URL = 1 << 3
} TextStyle;

typedef struct {
  WINDOW *win;
  int row;
  int start_col;
  int max_width;
  int cursor;
  unsigned current_style;
  unsigned style_stack[STYLE_STACK_MAX];
  size_t style_depth;
  attr_t active_attr;
} RenderCtx;

static bool g_supports_color = false;

void markdown_init_colors(void) {
  if (!has_colors()) {
    g_supports_color = false;
    return;
  }
  if (start_color() == ERR) {
    g_supports_color = false;
    return;
  }
  use_default_colors();
  init_pair(1, COLOR_YELLOW, -1); // Italic/Action
  init_pair(2, COLOR_CYAN, -1);   // Quote/Dialogue
  init_pair(3, COLOR_BLUE, -1);   // URL
  g_supports_color = true;
}

bool markdown_has_colors(void) { return g_supports_color; }

static attr_t style_to_attr(unsigned style_flags) {
  attr_t attr = A_NORMAL;

  if (style_flags & STYLE_BOLD) {
    attr |= A_BOLD;
  }
  if (style_flags & STYLE_ITALIC) {
    attr |= A_ITALIC;
    if (g_supports_color && !(style_flags & STYLE_QUOTE) &&
        !(style_flags & STYLE_URL)) {
      attr |= COLOR_PAIR(1);
    }
  }
  if (style_flags & STYLE_QUOTE) {
    if (g_supports_color) {
      attr |= COLOR_PAIR(2);
    }
  }
  if (style_flags & STYLE_URL) {
    attr |= A_UNDERLINE;
    if (g_supports_color) {
      attr |= COLOR_PAIR(3);
    }
  }
  return attr;
}

static void push_style(RenderCtx *ctx, unsigned mask) {
  if (ctx->style_depth >= STYLE_STACK_MAX) {
    return;
  }
  ctx->style_stack[ctx->style_depth++] = mask;
  ctx->current_style |= mask;
}

static void pop_style(RenderCtx *ctx, unsigned mask) {
  if (ctx->style_depth == 0) {
    return;
  }
  for (size_t i = ctx->style_depth; i > 0; --i) {
    if (ctx->style_stack[i - 1] == mask) {
      for (size_t j = i - 1; j < ctx->style_depth - 1; ++j) {
        ctx->style_stack[j] = ctx->style_stack[j + 1];
      }
      ctx->style_depth--;
      break;
    }
  }
  ctx->current_style = 0;
  for (size_t i = 0; i < ctx->style_depth; ++i) {
    ctx->current_style |= ctx->style_stack[i];
  }
}

static void refresh_attr(RenderCtx *ctx) {
  attr_t desired = style_to_attr(ctx->current_style);
  if (ctx->active_attr != desired) {
    wattrset(ctx->win, desired);
    ctx->active_attr = desired;
  }
}

static int utf8_char_len(unsigned char c) {
  if ((c & 0x80) == 0)
    return 1;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 1;
}

static int utf8_display_width(const char *str, int byte_len) {
  wchar_t wc;
  if (mbtowc(&wc, str, byte_len) > 0) {
    int w = wcwidth(wc);
    return w > 0 ? w : 1;
  }
  return 1;
}

static void emit_utf8_char(RenderCtx *ctx, const char *str, int byte_len) {
  int display_width = utf8_display_width(str, byte_len);
  if (ctx->cursor + display_width > ctx->max_width) {
    return;
  }
  refresh_attr(ctx);
  char buf[8];
  if (byte_len > 7)
    byte_len = 7;
  memcpy(buf, str, byte_len);
  buf[byte_len] = '\0';
  mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor, buf);
  ctx->cursor += display_width;
}

static void emit_char(RenderCtx *ctx, char ch) {
  if (ctx->cursor >= ctx->max_width) {
    return;
  }
  refresh_attr(ctx);
  mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ch);
  ctx->cursor++;
}

static void emit_str(RenderCtx *ctx, const char *str, size_t len) {
  size_t i = 0;
  while (i < len && ctx->cursor < ctx->max_width) {
    unsigned char c = (unsigned char)str[i];
    int char_len = utf8_char_len(c);
    if (i + char_len > len)
      break;
    if (char_len == 1) {
      emit_char(ctx, str[i]);
      i++;
    } else {
      emit_utf8_char(ctx, str + i, char_len);
      i += char_len;
    }
  }
}

static bool is_style_active(RenderCtx *ctx, unsigned mask) {
  for (size_t i = 0; i < ctx->style_depth; ++i) {
    if (ctx->style_stack[i] == mask) {
      return true;
    }
  }
  return false;
}

static bool is_url_char(char c) {
  return isalnum((unsigned char)c) || c == '-' || c == '.' || c == '_' ||
         c == '~' || c == ':' || c == '/' || c == '?' || c == '#' || c == '[' ||
         c == ']' || c == '@' || c == '!' || c == '$' || c == '&' ||
         c == '\'' || c == '(' || c == ')' || c == '+' || c == ',' ||
         c == ';' || c == '=' || c == '%';
}

static size_t try_parse_url(const char *text, size_t len, size_t pos) {
  const char *prefixes[] = {"https://", "http://", "www."};
  size_t prefix_lens[] = {8, 7, 4};

  for (int p = 0; p < 3; p++) {
    size_t plen = prefix_lens[p];
    if (pos + plen <= len && strncmp(text + pos, prefixes[p], plen) == 0) {
      size_t end = pos + plen;
      while (end < len && is_url_char(text[end])) {
        end++;
      }
      while (end > pos + plen &&
             (text[end - 1] == '.' || text[end - 1] == ',' ||
              text[end - 1] == ')' || text[end - 1] == '!' ||
              text[end - 1] == '?')) {
        end--;
      }
      if (end > pos + plen) {
        return end - pos;
      }
    }
  }
  return 0;
}

static size_t count_asterisks(const char *text, size_t len, size_t pos) {
  size_t count = 0;
  while (pos + count < len && text[pos + count] == '*') {
    count++;
  }
  return count;
}

static void render_rp_text(RenderCtx *ctx, const char *text, size_t len) {
  for (size_t i = 0; i < len && ctx->cursor < ctx->max_width;) {
    char ch = text[i];

    size_t url_len = try_parse_url(text, len, i);
    if (url_len > 0) {
      push_style(ctx, STYLE_URL);
      emit_str(ctx, text + i, url_len);
      pop_style(ctx, STYLE_URL);
      i += url_len;
      continue;
    }

    if (ch == '*') {
      size_t stars = count_asterisks(text, len, i);

      if (stars >= 3) {
        if (is_style_active(ctx, STYLE_BOLD) &&
            is_style_active(ctx, STYLE_ITALIC)) {
          pop_style(ctx, STYLE_ITALIC);
          pop_style(ctx, STYLE_BOLD);
        } else {
          push_style(ctx, STYLE_BOLD);
          push_style(ctx, STYLE_ITALIC);
        }
        i += 3;
        continue;
      } else if (stars == 2) {
        if (is_style_active(ctx, STYLE_BOLD)) {
          pop_style(ctx, STYLE_BOLD);
        } else {
          push_style(ctx, STYLE_BOLD);
        }
        i += 2;
        continue;
      } else {
        if (is_style_active(ctx, STYLE_ITALIC)) {
          pop_style(ctx, STYLE_ITALIC);
        } else {
          push_style(ctx, STYLE_ITALIC);
        }
        i += 1;
        continue;
      }
    }

    if (ch == '"') {
      if (is_style_active(ctx, STYLE_QUOTE)) {
        emit_char(ctx, ch);
        pop_style(ctx, STYLE_QUOTE);
      } else {
        push_style(ctx, STYLE_QUOTE);
        emit_char(ctx, ch);
      }
      i++;
      continue;
    }

    if (ch == '\n' || ch == '\r' || ch == '\t') {
      emit_char(ctx, ' ');
      i++;
      continue;
    }

    if ((unsigned char)ch < 32) {
      i++;
      continue;
    }

    unsigned char uc = (unsigned char)ch;
    int char_len = utf8_char_len(uc);
    if (i + char_len > len) {
      i++;
      continue;
    }

    if (char_len > 1) {
      emit_utf8_char(ctx, text + i, char_len);
      i += char_len;
    } else {
      emit_char(ctx, ch);
      i++;
    }
  }
}

void markdown_render_line(WINDOW *win, int row, int start_col, int width,
                          const char *text) {
  markdown_render_line_styled(win, row, start_col, width, text, 0);
}

unsigned markdown_render_line_styled(WINDOW *win, int row, int start_col,
                                     int width, const char *text,
                                     unsigned initial_style) {
  if (width <= 0) {
    return initial_style;
  }
  mvwhline(win, row, start_col, ' ', width);
  if (!text || text[0] == '\0') {
    return initial_style;
  }
  RenderCtx ctx = {.win = win,
                   .row = row,
                   .start_col = start_col,
                   .max_width = width,
                   .cursor = 0,
                   .current_style = 0,
                   .style_depth = 0,
                   .active_attr = A_NORMAL};

  if (initial_style & STYLE_BOLD)
    push_style(&ctx, STYLE_BOLD);
  if (initial_style & STYLE_ITALIC)
    push_style(&ctx, STYLE_ITALIC);
  if (initial_style & STYLE_QUOTE)
    push_style(&ctx, STYLE_QUOTE);

  render_rp_text(&ctx, text, strlen(text));
  wattrset(win, A_NORMAL);
  return ctx.current_style;
}

static unsigned compute_style_internal(const char *text, size_t len,
                                       unsigned initial_style) {
  unsigned style = initial_style;
  bool in_bold = (style & STYLE_BOLD) != 0;
  bool in_italic = (style & STYLE_ITALIC) != 0;
  bool in_quote = (style & STYLE_QUOTE) != 0;

  for (size_t i = 0; i < len;) {
    char ch = text[i];

    if (ch == '*') {
      size_t stars = 0;
      while (i + stars < len && text[i + stars] == '*')
        stars++;

      if (stars >= 3) {
        if (in_bold && in_italic) {
          in_bold = false;
          in_italic = false;
        } else {
          in_bold = true;
          in_italic = true;
        }
        i += 3;
      } else if (stars == 2) {
        in_bold = !in_bold;
        i += 2;
      } else {
        in_italic = !in_italic;
        i += 1;
      }
      continue;
    }

    if (ch == '"') {
      in_quote = !in_quote;
      i++;
      continue;
    }

    i++;
  }

  unsigned result = 0;
  if (in_bold)
    result |= STYLE_BOLD;
  if (in_italic)
    result |= STYLE_ITALIC;
  if (in_quote)
    result |= STYLE_QUOTE;
  return result;
}

unsigned markdown_compute_style_after(const char *text, size_t len,
                                      unsigned initial_style) {
  if (!text || len == 0)
    return initial_style;
  return compute_style_internal(text, len, initial_style);
}

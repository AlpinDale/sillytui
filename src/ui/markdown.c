#include "ui/markdown.h"
#include "core/log.h"
#include "ui/syntax/syntax_ncurses.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>

#define STYLE_STACK_MAX 64

#define MD_PAIR_NORMAL 1
#define MD_PAIR_ITALIC 2
#define MD_PAIR_QUOTE 3
#define MD_PAIR_URL 4
#define MD_PAIR_CODE 9
#define MD_PAIR_CODE_BLOCK 10
#define MD_PAIR_STRIKETHROUGH 13
#define MD_PAIR_HEADER_MARKER 15
#define MD_PAIR_NORMAL_SEL 5
#define MD_PAIR_ITALIC_SEL 6
#define MD_PAIR_QUOTE_SEL 7
#define MD_PAIR_URL_SEL 8
#define MD_PAIR_CODE_SEL 11
#define MD_PAIR_CODE_BLOCK_SEL 12
#define MD_PAIR_STRIKETHROUGH_SEL 14
#define MD_PAIR_HEADER_MARKER_SEL 16

#define MD_BG_SELECTED 236

typedef enum {
  STYLE_ITALIC = 1 << 0,
  STYLE_QUOTE = 1 << 1,
  STYLE_BOLD = 1 << 2,
  STYLE_URL = 1 << 3,
  STYLE_CODE = 1 << 4,
  STYLE_CODE_BLOCK = 1 << 5,
  STYLE_STRIKETHROUGH = 1 << 6
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
  int bg_color;
  bool code_block_just_started; // Track if we just entered a code block on this
                                // line
  bool code_block_just_ended;   // Track if we just exited a code block on this
                                // line
} RenderCtx;

static bool g_supports_color = false;
static bool g_code_block_has_syntax = false;
static SyntaxHighlightState g_syntax_state;

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

  init_pair(MD_PAIR_NORMAL, -1, -1);
  init_pair(MD_PAIR_ITALIC, 245, -1);
  init_pair(MD_PAIR_QUOTE, 76, -1);
  init_pair(MD_PAIR_URL, 75, -1);
  init_pair(MD_PAIR_CODE, 252, 238);
  init_pair(MD_PAIR_CODE_BLOCK, 252, 238);
  init_pair(MD_PAIR_STRIKETHROUGH, 88, -1);
  init_pair(MD_PAIR_HEADER_MARKER, 240, -1);

  init_pair(MD_PAIR_NORMAL_SEL, -1, MD_BG_SELECTED);
  init_pair(MD_PAIR_ITALIC_SEL, 245, MD_BG_SELECTED);
  init_pair(MD_PAIR_QUOTE_SEL, 76, MD_BG_SELECTED);
  init_pair(MD_PAIR_URL_SEL, 75, MD_BG_SELECTED);
  init_pair(MD_PAIR_CODE_SEL, 252, MD_BG_SELECTED);
  init_pair(MD_PAIR_CODE_BLOCK_SEL, 252, MD_BG_SELECTED);
  init_pair(MD_PAIR_STRIKETHROUGH_SEL, 88, MD_BG_SELECTED);
  init_pair(MD_PAIR_HEADER_MARKER_SEL, 240, MD_BG_SELECTED);

  syntax_init_colors();
  syntax_highlight_init();
  syntax_state_init(&g_syntax_state);

  g_supports_color = true;
}

bool markdown_has_colors(void) { return g_supports_color; }

static int get_color_pair(unsigned style_flags, bool selected) {
  if (!g_supports_color)
    return 0;

  int base_pair;
  if (style_flags & STYLE_CODE_BLOCK) {
    base_pair = MD_PAIR_CODE_BLOCK;
  } else if (style_flags & STYLE_CODE) {
    base_pair = MD_PAIR_CODE;
  } else if (style_flags & STYLE_URL) {
    base_pair = MD_PAIR_URL;
  } else if (style_flags & STYLE_QUOTE) {
    base_pair = MD_PAIR_QUOTE;
  } else if ((style_flags & STYLE_ITALIC) && !(style_flags & STYLE_QUOTE)) {
    base_pair = MD_PAIR_ITALIC;
  } else {
    base_pair = MD_PAIR_NORMAL;
  }

  if (selected) {
    if (base_pair == MD_PAIR_CODE) {
      return MD_PAIR_CODE_SEL;
    } else if (base_pair == MD_PAIR_CODE_BLOCK) {
      return MD_PAIR_CODE_BLOCK_SEL;
    } else {
      return base_pair + 4;
    }
  }
  return base_pair;
}

static attr_t style_to_attr(unsigned style_flags, bool selected) {
  attr_t attr = A_NORMAL;

  if (style_flags & STYLE_BOLD)
    attr |= A_BOLD;
  if (style_flags & STYLE_ITALIC)
    attr |= A_ITALIC;
  if (style_flags & STYLE_URL)
    attr |= A_UNDERLINE;

  int pair = get_color_pair(style_flags, selected);
  if (pair > 0)
    attr |= COLOR_PAIR(pair);

  return attr;
}

static void push_style(RenderCtx *ctx, unsigned mask) {
  if (ctx->style_depth >= STYLE_STACK_MAX)
    return;
  ctx->style_stack[ctx->style_depth++] = mask;
  ctx->current_style |= mask;
}

static void pop_style(RenderCtx *ctx, unsigned mask) {
  if (ctx->style_depth == 0)
    return;
  for (size_t i = ctx->style_depth; i > 0; --i) {
    if (ctx->style_stack[i - 1] == mask) {
      for (size_t j = i - 1; j < ctx->style_depth - 1; ++j)
        ctx->style_stack[j] = ctx->style_stack[j + 1];
      ctx->style_depth--;
      break;
    }
  }
  ctx->current_style = 0;
  for (size_t i = 0; i < ctx->style_depth; ++i)
    ctx->current_style |= ctx->style_stack[i];
}

static void refresh_attr(RenderCtx *ctx) {
  bool selected = (ctx->bg_color == MD_BG_SELECTED);
  attr_t desired = style_to_attr(ctx->current_style, selected);
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
#ifndef _WIN32
    int w = wcwidth(wc);
    return w > 0 ? w : 1;
#else
    return 1;
#endif
  }
  return 1;
}

static bool is_style_active(RenderCtx *ctx, unsigned mask);

static void emit_utf8_char(RenderCtx *ctx, const char *str, int byte_len) {
  int display_width = utf8_display_width(str, byte_len);
  if (ctx->cursor + display_width > ctx->max_width)
    return;
  refresh_attr(ctx);
  char buf[16];
  if (byte_len > 7)
    byte_len = 7;
  memcpy(buf, str, byte_len);
  if (is_style_active(ctx, STYLE_STRIKETHROUGH)) {
    buf[byte_len] = '\xCC';
    buf[byte_len + 1] = '\xB6';
    buf[byte_len + 2] = '\0';
  } else {
    buf[byte_len] = '\0';
  }
  mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor, buf);
  ctx->cursor += display_width;
}

static void emit_char(RenderCtx *ctx, char ch) {
  if (ctx->cursor >= ctx->max_width)
    return;
  refresh_attr(ctx);
  if (is_style_active(ctx, STYLE_STRIKETHROUGH) && ch != ' ' && ch != '\n' &&
      ch != '\r' && ch != '\t') {
    char combined[8];
    snprintf(combined, sizeof(combined), "%c\xCC\xB6", ch);
    mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor, combined);
    ctx->cursor++;
  } else {
    mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ch);
    ctx->cursor++;
  }
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
    if (ctx->style_stack[i] == mask)
      return true;
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
      while (end < len && is_url_char(text[end]))
        end++;
      while (end > pos + plen &&
             (text[end - 1] == '.' || text[end - 1] == ',' ||
              text[end - 1] == ')' || text[end - 1] == '!' ||
              text[end - 1] == '?'))
        end--;
      if (end > pos + plen)
        return end - pos;
    }
  }
  return 0;
}

static size_t count_asterisks(const char *text, size_t len, size_t pos) {
  size_t count = 0;
  while (pos + count < len && text[pos + count] == '*')
    count++;
  return count;
}

static size_t count_backticks(const char *text, size_t len, size_t pos) {
  size_t count = 0;
  while (pos + count < len && text[pos + count] == '`')
    count++;
  return count;
}

static size_t count_underscores(const char *text, size_t len, size_t pos) {
  size_t count = 0;
  while (pos + count < len && text[pos + count] == '_')
    count++;
  return count;
}

static size_t count_tildes(const char *text, size_t len, size_t pos) {
  size_t count = 0;
  while (pos + count < len && text[pos + count] == '~')
    count++;
  return count;
}

static size_t find_link_end(const char *text, size_t len, size_t start) {
  if (start + 1 >= len || text[start] != '[')
    return 0;

  size_t i = start + 1;
  while (i < len && text[i] != ']') {
    if (text[i] == '\\' && i + 1 < len)
      i += 2;
    else
      i++;
  }
  if (i >= len || text[i] != ']')
    return 0;

  if (i + 1 >= len || text[i + 1] != '(')
    return 0;

  i += 2;
  while (i < len && text[i] != ')') {
    if (text[i] == '\\' && i + 1 < len)
      i += 2;
    else
      i++;
  }
  if (i >= len || text[i] != ')')
    return 0;

  return i + 1 - start;
}

static bool is_horizontal_rule(const char *text, size_t len, size_t pos) {
  if (pos + 3 > len)
    return false;

  char ch = text[pos];
  if (ch != '-' && ch != '*' && ch != '_')
    return false;

  size_t count = 0;
  while (pos + count < len && text[pos + count] == ch && count < 10) {
    count++;
  }

  return count >= 3;
}

static int detect_header_level(const char *text, size_t len) {
  if (len == 0 || text[0] != '#')
    return 0;

  size_t count = 0;
  while (count < len && count < 6 && text[count] == '#')
    count++;

  if (count >= len || text[count] != ' ')
    return 0;

  return (int)count;
}

static int detect_list_item(const char *text, size_t len,
                            size_t *indent_level) {
  if (len == 0)
    return 0;

  size_t i = 0;
  int indent = 0;
  while (i < len && (text[i] == ' ' || text[i] == '\t')) {
    if (text[i] == ' ')
      indent++;
    else
      indent += 4;
    i++;
  }

  if (i >= len)
    return 0;

  char marker = text[i];
  if ((marker == '-' || marker == '*' || marker == '+') && i + 1 < len) {
    if (text[i + 1] == '[' && i + 3 < len) {
      if (text[i + 2] == ' ' || text[i + 2] == 'x' || text[i + 2] == 'X') {
        if (text[i + 3] == ']' &&
            (i + 4 >= len || text[i + 4] == ' ' || text[i + 4] == '\t')) {
          if (indent_level)
            *indent_level = indent;
          return 3;
        }
      }
    } else if (text[i + 1] == ' ' || text[i + 1] == '\t') {
      if (indent_level)
        *indent_level = indent;
      return 1;
    }
  }

  if (marker >= '0' && marker <= '9') {
    while (i < len && text[i] >= '0' && text[i] <= '9')
      i++;
    if (i < len && text[i] == '.' && i + 1 < len &&
        (text[i + 1] == ' ' || text[i + 1] == '\t')) {
      if (indent_level)
        *indent_level = indent;
      return 2;
    }
  }

  return 0;
}

static bool detect_blockquote(const char *text, size_t len, size_t *skip_len) {
  if (len == 0)
    return false;

  size_t i = 0;
  while (i < len && (text[i] == ' ' || text[i] == '\t'))
    i++;

  if (i >= len || text[i] != '>')
    return false;

  i++;
  if (i < len && (text[i] == ' ' || text[i] == '\t'))
    i++;

  if (skip_len)
    *skip_len = i;
  return true;
}

static size_t find_inline_code_end(const char *text, size_t len, size_t start) {
  // Find the closing backtick for inline code
  // Inline code ends at: closing backtick, newline, or end of text
  for (size_t i = start; i < len; i++) {
    if (text[i] == '`') {
      // Check if it's escaped
      if (i > 0 && text[i - 1] == '\\')
        continue;
      return i - start;
    }
    if (text[i] == '\n' || text[i] == '\r')
      return i - start;
  }
  return len - start;
}

static void render_rp_text(RenderCtx *ctx, const char *text, size_t len) {
  int header_level = detect_header_level(text, len);
  bool is_header = header_level > 0 && !is_style_active(ctx, STYLE_CODE_BLOCK);

  size_t list_indent = 0;
  int list_type = detect_list_item(text, len, &list_indent);
  bool is_list = list_type > 0 && !is_style_active(ctx, STYLE_CODE_BLOCK);

  size_t blockquote_skip = 0;
  bool is_blockquote = detect_blockquote(text, len, &blockquote_skip) &&
                       !is_style_active(ctx, STYLE_CODE_BLOCK);

  if (is_blockquote) {
    if (ctx->cursor < ctx->max_width) {
      mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor, "│ ");
      ctx->cursor += 2;
    }
    push_style(ctx, STYLE_QUOTE);
    text += blockquote_skip;
    len -= blockquote_skip;
  }

  if (is_list) {
    size_t i = list_indent;
    if (i < len && (text[i] == ' ' || text[i] == '\t')) {
      while (i < len && (text[i] == ' ' || text[i] == '\t'))
        i++;
    }

    if (i < len) {
      if (list_type == 1) {
        if (ctx->cursor < ctx->max_width) {
          for (size_t j = 0; j < list_indent && ctx->cursor < ctx->max_width;
               j++) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ' ');
            ctx->cursor++;
          }
          mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor, "•");
          ctx->cursor++;
          if (ctx->cursor < ctx->max_width) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ' ');
            ctx->cursor++;
          }
        }
        i++;
        if (i < len && (text[i] == ' ' || text[i] == '\t'))
          i++;
      } else if (list_type == 3) {
        if (ctx->cursor < ctx->max_width) {
          for (size_t j = 0; j < list_indent && ctx->cursor < ctx->max_width;
               j++) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ' ');
            ctx->cursor++;
          }
          bool is_checked = false;
          if (i < len && (text[i] == '-' || text[i] == '*' || text[i] == '+')) {
            if (i + 1 < len && text[i + 1] == '[') {
              if (i + 2 < len && (text[i + 2] == 'x' || text[i + 2] == 'X')) {
                is_checked = true;
              }
            }
          }
          if (is_checked) {
            mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor, "☑");
          } else {
            mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor, "☐");
          }
          ctx->cursor++;
          if (ctx->cursor < ctx->max_width) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ' ');
            ctx->cursor++;
          }
        }
        i++;
        if (i < len && text[i] == '[') {
          i++;
          if (i < len && (text[i] == ' ' || text[i] == 'x' || text[i] == 'X'))
            i++;
          if (i < len && text[i] == ']')
            i++;
        }
        if (i < len && (text[i] == ' ' || text[i] == '\t'))
          i++;
      } else if (list_type == 2) {
        if (ctx->cursor < ctx->max_width) {
          for (size_t j = 0; j < list_indent && ctx->cursor < ctx->max_width;
               j++) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ' ');
            ctx->cursor++;
          }
          while (i < len && text[i] >= '0' && text[i] <= '9' &&
                 ctx->cursor < ctx->max_width) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, text[i]);
            ctx->cursor++;
            i++;
          }
          if (i < len && text[i] == '.' && ctx->cursor < ctx->max_width) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, '.');
            ctx->cursor++;
            i++;
          }
          if (ctx->cursor < ctx->max_width) {
            mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ' ');
            ctx->cursor++;
          }
        }
        if (i < len && (text[i] == ' ' || text[i] == '\t'))
          i++;
      }

      text += i;
      len -= i;
    }
  }

  if (is_header) {
    int marker_pair = (ctx->bg_color == MD_BG_SELECTED)
                          ? MD_PAIR_HEADER_MARKER_SEL
                          : MD_PAIR_HEADER_MARKER;
    if (g_supports_color)
      wattron(ctx->win, COLOR_PAIR(marker_pair));
    for (int j = 0; j < header_level; j++) {
      if (ctx->cursor >= ctx->max_width)
        break;
      mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, '#');
      ctx->cursor++;
    }
    if (g_supports_color)
      wattroff(ctx->win, COLOR_PAIR(marker_pair));

    if (ctx->cursor < ctx->max_width) {
      mvwaddch(ctx->win, ctx->row, ctx->start_col + ctx->cursor, ' ');
      ctx->cursor++;
    }

    size_t i = (size_t)header_level;
    if (i < len && text[i] == ' ')
      i++;

    text += i;
    len -= i;
  }

  for (size_t i = 0; i < len && ctx->cursor < ctx->max_width;) {
    char ch = text[i];

    if (ch == '\\' && i + 1 < len && !is_style_active(ctx, STYLE_CODE) &&
        !is_style_active(ctx, STYLE_CODE_BLOCK)) {
      char next = text[i + 1];
      if (next == '*' || next == '_' || next == '~' || next == '`' ||
          next == '[' || next == ']' || next == '(' || next == ')' ||
          next == '\\') {
        emit_char(ctx, next);
        i += 2;
        continue;
      }
    }

    if (is_horizontal_rule(text, len, i) && !is_style_active(ctx, STYLE_CODE) &&
        !is_style_active(ctx, STYLE_CODE_BLOCK)) {
      int rule_len = ctx->max_width - ctx->cursor;
      if (rule_len > 0) {
        refresh_attr(ctx);
        for (int j = 0; j < rule_len; j++) {
          mvwaddstr(ctx->win, ctx->row, ctx->start_col + ctx->cursor + j, "─");
        }
        ctx->cursor += rule_len;
      }
      while (i < len && (text[i] == '-' || text[i] == '*' || text[i] == '_'))
        i++;
      continue;
    }

    size_t link_len = find_link_end(text, len, i);
    if (link_len > 0 && !is_style_active(ctx, STYLE_CODE) &&
        !is_style_active(ctx, STYLE_CODE_BLOCK)) {
      size_t link_text_start = i + 1;
      size_t link_text_end = link_text_start;
      while (link_text_end < len && text[link_text_end] != ']') {
        if (text[link_text_end] == '\\' && link_text_end + 1 < len)
          link_text_end += 2;
        else
          link_text_end++;
      }
      size_t link_text_len = link_text_end - link_text_start;

      push_style(ctx, STYLE_URL);
      if (link_text_len > 0) {
        RenderCtx link_ctx = *ctx;
        render_rp_text(&link_ctx, text + link_text_start, link_text_len);
        ctx->cursor = link_ctx.cursor;
      }
      pop_style(ctx, STYLE_URL);
      i += link_len;
      continue;
    }

    if (is_style_active(ctx, STYLE_CODE_BLOCK)) {
      if (ch == '`') {
        size_t backticks = count_backticks(text, len, i);
        if (backticks >= 3) {
          pop_style(ctx, STYLE_CODE_BLOCK);
          i += 3;
          continue;
        }
      }
      if (ctx->code_block_just_started) {
        i = len;
        continue;
      }
      if (g_code_block_has_syntax && g_supports_color) {
        syntax_render_line(ctx->win, ctx->row, ctx->start_col + ctx->cursor,
                           ctx->max_width - ctx->cursor, text, &g_syntax_state);
        return;
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
      continue;
    }

    size_t url_len = try_parse_url(text, len, i);
    if (url_len > 0 && !is_style_active(ctx, STYLE_CODE)) {
      push_style(ctx, STYLE_URL);
      emit_str(ctx, text + i, url_len);
      pop_style(ctx, STYLE_URL);
      i += url_len;
      continue;
    }

    if (ch == '~' && !is_style_active(ctx, STYLE_CODE)) {
      size_t tildes = count_tildes(text, len, i);
      if (tildes >= 2) {
        if (is_style_active(ctx, STYLE_STRIKETHROUGH)) {
          pop_style(ctx, STYLE_STRIKETHROUGH);
        } else {
          push_style(ctx, STYLE_STRIKETHROUGH);
        }
        i += 2;
        continue;
      }
    }

    if (ch == '_' && !is_style_active(ctx, STYLE_CODE)) {
      size_t underscores = count_underscores(text, len, i);
      if (underscores >= 2) {
        if (is_style_active(ctx, STYLE_BOLD)) {
          pop_style(ctx, STYLE_BOLD);
        } else {
          push_style(ctx, STYLE_BOLD);
        }
        i += 2;
        continue;
      } else if (underscores == 1) {
        if (is_style_active(ctx, STYLE_ITALIC)) {
          pop_style(ctx, STYLE_ITALIC);
        } else {
          push_style(ctx, STYLE_ITALIC);
        }
        i += 1;
        continue;
      }
    }

    if (ch == '*' && !is_style_active(ctx, STYLE_CODE)) {
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

    // Check for code blocks (triple backticks) first
    if (ch == '`') {
      size_t backticks = count_backticks(text, len, i);
      if (backticks >= 3) {
        // Code block - toggle it (borders drawn in markdown_render_line_bg)
        if (is_style_active(ctx, STYLE_CODE_BLOCK)) {
          pop_style(ctx, STYLE_CODE_BLOCK);
        } else {
          push_style(ctx, STYLE_CODE_BLOCK);
        }
        i += 3;
        continue;
      } else if (backticks == 1) {
        // Inline code
        if (is_style_active(ctx, STYLE_CODE)) {
          pop_style(ctx, STYLE_CODE);
          i++;
          continue;
        } else {
          // Find the end of inline code
          size_t code_len = find_inline_code_end(text, len, i + 1);
          if (code_len > 0) {
            push_style(ctx, STYLE_CODE);
            i++; // Skip opening backtick
            emit_str(ctx, text + i, code_len);
            i += code_len;
            if (i < len && text[i] == '`') {
              pop_style(ctx, STYLE_CODE);
              i++; // Skip closing backtick
            }
            continue;
          }
        }
      }
    }

    if (ch == '"' && !is_style_active(ctx, STYLE_CODE)) {
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

  if (is_blockquote) {
    pop_style(ctx, STYLE_QUOTE);
  }
}

void markdown_render_line(WINDOW *win, int row, int start_col, int width,
                          const char *text) {
  markdown_render_line_styled(win, row, start_col, width, text, 0);
}

unsigned markdown_render_line_styled(WINDOW *win, int row, int start_col,
                                     int width, const char *text,
                                     unsigned initial_style) {
  return markdown_render_line_bg(win, row, start_col, width, text,
                                 initial_style, -1);
}

unsigned markdown_render_line_bg(WINDOW *win, int row, int start_col, int width,
                                 const char *text, unsigned initial_style,
                                 int bg_color) {
  if (width <= 0)
    return initial_style;

  bool in_code_block_start = (initial_style & STYLE_CODE_BLOCK) != 0;
  bool selected = (bg_color == MD_BG_SELECTED);

  // Check if text contains code block markers to determine start/end
  // The key insight: if we're already in a code block (initial_style has
  // STYLE_CODE_BLOCK), then any triple backticks we find are CLOSING.
  // Otherwise, they're OPENING.
  bool code_block_starts = false;
  bool code_block_ends = false;
  bool in_code_block = in_code_block_start;

  // DEBUG: Log initial state (only in debug builds)
#ifdef DEBUG_MARKDOWN
  if (text && strlen(text) > 0 && strlen(text) < 100) {
    log_message(
        LOG_DEBUG, __FILE__, __LINE__,
        "MARKDOWN: line='%.50s', in_code_block_start=%d, initial_style=0x%x",
        text, in_code_block_start ? 1 : 0, initial_style);
  }
#endif

  if (text && text[0] != '\0') {
    // Search for triple backticks in the text
    const char *p = text;
    while (*p) {
      if (*p == '`') {
        size_t backticks = 0;
        while (p[backticks] == '`')
          backticks++;
        if (backticks >= 3) {
          // Found triple backticks - determine if opening or closing based on
          // current state
          if (in_code_block_start) {
            // We're in a code block, so these are CLOSING backticks
            code_block_ends = true;
            in_code_block = false;
            if (g_code_block_has_syntax) {
              g_code_block_has_syntax = false;
              syntax_set_language(NULL);
              syntax_state_init(&g_syntax_state);
            }
#ifdef DEBUG_MARKDOWN
            log_message(LOG_DEBUG, __FILE__, __LINE__,
                        "MARKDOWN: Found CLOSING backticks "
                        "(in_code_block_start was true)");
#endif
          } else {
            // We're not in a code block, so these are OPENING backticks
            code_block_starts = true;
            in_code_block = true;
            const char *lang = p + backticks;
            while (*lang == ' ' || *lang == '\t')
              lang++;

            size_t lang_len = 0;
            while (lang[lang_len] && lang[lang_len] != '\n' &&
                   lang[lang_len] != '\r' && lang[lang_len] != ' ' &&
                   lang[lang_len] != '\t') {
              lang_len++;
            }

            if (lang_len > 0) {
              char lang_name[32];
              if (lang_len > 31)
                lang_len = 31;
              memcpy(lang_name, lang, lang_len);
              lang_name[lang_len] = '\0';

              for (size_t i = 0; i < lang_len; i++) {
                lang_name[i] = tolower((unsigned char)lang_name[i]);
              }

              syntax_set_language(lang_name);
              g_code_block_has_syntax = true;
              syntax_state_init(&g_syntax_state);
            } else {
              g_code_block_has_syntax = false;
              syntax_set_language(NULL);
            }
#ifdef DEBUG_MARKDOWN
            log_message(LOG_DEBUG, __FILE__, __LINE__,
                        "MARKDOWN: Found OPENING backticks "
                        "(in_code_block_start was false)");
#endif
          }
          // Only process the first set of triple backticks on this line
          break;
        }
        p += backticks;
      } else {
        p++;
      }
    }

    // If we're in a code block and didn't find closing backticks, we're still
    // in it
    if (in_code_block_start && !code_block_ends) {
      in_code_block = true;
    }
  } else if (in_code_block_start) {
    // Empty line but we're in a code block - still in it
    in_code_block = true;
  }

  // DEBUG: Log final state
  if (text && strlen(text) > 0 && strlen(text) < 100) {
    log_message(
        LOG_DEBUG, __FILE__, __LINE__,
        "MARKDOWN: code_block_starts=%d, code_block_ends=%d, in_code_block=%d",
        code_block_starts ? 1 : 0, code_block_ends ? 1 : 0,
        in_code_block ? 1 : 0);
  }

  // For lines that are fully in a code block (not starting/ending), draw side
  // borders
  bool is_middle_code_block_line =
      in_code_block && !code_block_starts && !code_block_ends;

  // Fill background
  if (in_code_block || code_block_starts || code_block_ends) {
    // Code block background - skip for syntax highlighted blocks
    if (g_code_block_has_syntax) {
      mvwhline(win, row, start_col, ' ', width);
    } else if (g_supports_color) {
      wattron(win, COLOR_PAIR(MD_PAIR_CODE_BLOCK));
      mvwhline(win, row, start_col, ' ', width);
      wattroff(win, COLOR_PAIR(MD_PAIR_CODE_BLOCK));
    } else {
      mvwhline(win, row, start_col, ' ', width);
    }
  } else if (selected && g_supports_color) {
    wattron(win, COLOR_PAIR(MD_PAIR_NORMAL_SEL));
    mvwhline(win, row, start_col, ' ', width);
    wattroff(win, COLOR_PAIR(MD_PAIR_NORMAL_SEL));
  } else {
    mvwhline(win, row, start_col, ' ', width);
  }

  // Draw code block borders
  if (in_code_block || code_block_starts || code_block_ends) {
    if (g_supports_color)
      wattron(win, COLOR_PAIR(MD_PAIR_CODE_BLOCK));

    // Priority: ending > starting > middle
    // Draw bottom border if ending (facing downwards to close the box)
    if (code_block_ends) {
#ifdef DEBUG_MARKDOWN
      log_message(LOG_DEBUG, __FILE__, __LINE__,
                  "MARKDOWN: Drawing BOTTOM border (└─┘)");
#endif
      mvwaddstr(win, row, start_col, "└");
      for (int x = start_col + 1; x < start_col + width - 1; x++)
        mvwaddstr(win, row, x, "─");
      mvwaddstr(win, row, start_col + width - 1, "┘");
    }
    // Draw top border if starting (facing upwards to open the box)
    else if (code_block_starts) {
#ifdef DEBUG_MARKDOWN
      log_message(LOG_DEBUG, __FILE__, __LINE__,
                  "MARKDOWN: Drawing TOP border (┌─┐)");
#endif
      mvwaddstr(win, row, start_col, "┌");
      for (int x = start_col + 1; x < start_col + width - 1; x++)
        mvwaddstr(win, row, x, "─");
      mvwaddstr(win, row, start_col + width - 1, "┐");
    }
    // Draw left and right borders for middle lines
    else if (is_middle_code_block_line || in_code_block) {
#ifdef DEBUG_MARKDOWN
      // Only log occasionally to avoid spam
      static int log_counter = 0;
      if (++log_counter % 10 == 0) {
        log_message(LOG_DEBUG, __FILE__, __LINE__,
                    "MARKDOWN: Drawing SIDE borders (││)");
      }
#endif
      mvwaddstr(win, row, start_col, "│");
      mvwaddstr(win, row, start_col + width - 1, "│");
    }

    if (g_supports_color)
      wattroff(win, COLOR_PAIR(MD_PAIR_CODE_BLOCK));
  }

  if (!text || text[0] == '\0')
    return initial_style;

  wattrset(win, A_NORMAL);

  int header_level = detect_header_level(text, strlen(text));
  bool is_header = header_level > 0 && !in_code_block;

  RenderCtx ctx = {.win = win,
                   .row = row,
                   .start_col = start_col,
                   .max_width = width,
                   .cursor = 0,
                   .current_style = 0,
                   .style_depth = 0,
                   .active_attr = A_NORMAL,
                   .bg_color = bg_color,
                   .code_block_just_started = code_block_starts,
                   .code_block_just_ended = code_block_ends};

  if (is_header) {
    push_style(&ctx, STYLE_BOLD);
  } else {
    if (initial_style & STYLE_BOLD)
      push_style(&ctx, STYLE_BOLD);
  }
  if (initial_style & STYLE_ITALIC)
    push_style(&ctx, STYLE_ITALIC);
  if (initial_style & STYLE_QUOTE)
    push_style(&ctx, STYLE_QUOTE);
  if (initial_style & STYLE_CODE)
    push_style(&ctx, STYLE_CODE);
  if (initial_style & STYLE_STRIKETHROUGH)
    push_style(&ctx, STYLE_STRIKETHROUGH);
  if (in_code_block_start || code_block_starts) {
    push_style(&ctx, STYLE_CODE_BLOCK);
    ctx.cursor = 1;
    ctx.max_width = width - 2;
  }

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
  bool in_code = (style & STYLE_CODE) != 0;
  bool in_code_block = (style & STYLE_CODE_BLOCK) != 0;
  bool in_strikethrough = (style & STYLE_STRIKETHROUGH) != 0;

  int header_level = detect_header_level(text, len);
  bool is_header = header_level > 0 && !in_code_block;
  if (is_header) {
    in_bold = true;
    size_t i = (size_t)header_level;
    if (i < len && text[i] == ' ')
      i++;
    text += i;
    len -= i;
  }

  size_t blockquote_skip = 0;
  bool is_blockquote =
      detect_blockquote(text, len, &blockquote_skip) && !in_code_block;
  bool blockquote_was_active = is_blockquote;
  if (is_blockquote) {
    text += blockquote_skip;
    len -= blockquote_skip;
  }

  for (size_t i = 0; i < len;) {
    char ch = text[i];

    if (ch == '\\' && i + 1 < len && !in_code && !in_code_block) {
      i += 2;
      continue;
    }

    if (ch == '`') {
      size_t backticks = 0;
      while (i + backticks < len && text[i + backticks] == '`')
        backticks++;
      if (backticks >= 3) {
        in_code_block = !in_code_block;
        i += 3;
        continue;
      } else if (backticks == 1 && !in_code_block) {
        in_code = !in_code;
        i++;
        continue;
      }
    }

    if (in_code_block) {
      i++;
      continue;
    }

    if (ch == '~' && !in_code) {
      size_t tildes = count_tildes(text, len, i);
      if (tildes >= 2) {
        in_strikethrough = !in_strikethrough;
        i += 2;
        continue;
      }
    }

    if (ch == '_' && !in_code) {
      size_t underscores = count_underscores(text, len, i);
      if (underscores >= 2) {
        in_bold = !in_bold;
        i += 2;
        continue;
      } else if (underscores == 1) {
        in_italic = !in_italic;
        i += 1;
        continue;
      }
    }

    if (ch == '*' && !in_code) {
      size_t stars = count_asterisks(text, len, i);

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

    if (ch == '"' && !in_code) {
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
  if (in_quote && !blockquote_was_active)
    result |= STYLE_QUOTE;
  if (in_code)
    result |= STYLE_CODE;
  if (in_code_block)
    result |= STYLE_CODE_BLOCK;
  if (in_strikethrough)
    result |= STYLE_STRIKETHROUGH;
  return result;
}

unsigned markdown_compute_style_after(const char *text, size_t len,
                                      unsigned initial_style) {
  if (!text || len == 0)
    return initial_style;
  return compute_style_internal(text, len, initial_style);
}

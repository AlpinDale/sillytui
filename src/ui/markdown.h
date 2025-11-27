#ifndef MARKDOWN_H
#define MARKDOWN_H

#include <curses.h>
#include <stdbool.h>

void markdown_init_colors(void);
bool markdown_has_colors(void);
void markdown_render_line(WINDOW *win, int row, int start_col, int width,
                          const char *text);
unsigned markdown_render_line_styled(WINDOW *win, int row, int start_col,
                                     int width, const char *text,
                                     unsigned initial_style);
unsigned markdown_render_line_bg(WINDOW *win, int row, int start_col, int width,
                                 const char *text, unsigned initial_style,
                                 int bg_color);
unsigned markdown_compute_style_after(const char *text, size_t len,
                                      unsigned initial_style);

#endif

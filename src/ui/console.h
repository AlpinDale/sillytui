#ifndef CONSOLE_H
#define CONSOLE_H

#include "core/log.h"
#include <stdbool.h>
#include <stddef.h>

#define CONSOLE_MAX_ENTRIES 1000
#define CONSOLE_MAX_MESSAGE_LEN 512

typedef struct {
  char timestamp[32];
  LogLevel level;
  char file[64];
  int line;
  char message[CONSOLE_MAX_MESSAGE_LEN];
} ConsoleLogEntry;

typedef struct {
  ConsoleLogEntry entries[CONSOLE_MAX_ENTRIES];
  size_t count;
  size_t head;       // Ring buffer head (oldest entry)
  size_t tail;       // Ring buffer tail (newest entry)
  int scroll_offset; // How many lines to scroll up from bottom
  bool visible;
  bool auto_scroll;   // Auto-scroll to bottom when new logs arrive
  LogLevel min_level; // Minimum log level to show (filter)
  bool needs_redraw;  // Flag to indicate console needs redrawing
} ConsoleState;

void console_init(ConsoleState *console);
void console_free(ConsoleState *console);
void console_add_log(ConsoleState *console, LogLevel level, const char *file,
                     int line, const char *message);
void console_toggle(ConsoleState *console);
void console_set_visible(ConsoleState *console, bool visible);
bool console_is_visible(const ConsoleState *console);
void console_scroll(ConsoleState *console, int direction);
void console_scroll_to_bottom(ConsoleState *console);
void console_clear(ConsoleState *console);
void console_set_min_level(ConsoleState *console, LogLevel min_level);

#endif

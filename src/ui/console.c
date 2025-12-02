#include "ui/console.h"
#include "core/time.h"
#include <string.h>

void console_init(ConsoleState *console) {
  if (!console)
    return;
  memset(console, 0, sizeof(ConsoleState));
  console->auto_scroll = true;
  console->min_level = LOG_INFO; // Show INFO, WARNING, ERROR by default
}

void console_free(ConsoleState *console) {
  // Nothing to free, all data is in the struct
  (void)console;
}

void console_add_log(ConsoleState *console, LogLevel level, const char *file,
                     int line, const char *message) {
  if (!console || !message)
    return;

  // Filter by minimum level
  if (level < console->min_level)
    return;

  // Get timestamp
  char timestamp[32];
  get_timestamp(timestamp, sizeof(timestamp));

  // Extract filename from path
  const char *filename = file ? strrchr(file, '/') : NULL;
  if (filename)
    filename++;
  else
    filename = file ? file : "?";

  // Add entry to ring buffer
  ConsoleLogEntry *entry;
  if (console->count < CONSOLE_MAX_ENTRIES) {
    // Buffer not full yet, add to tail
    entry = &console->entries[console->count];
    console->count++;
  } else {
    // Buffer full, overwrite oldest entry (at head)
    entry = &console->entries[console->head];
    console->head = (console->head + 1) % CONSOLE_MAX_ENTRIES;
  }

  // Copy data to entry
  strncpy(entry->timestamp, timestamp, sizeof(entry->timestamp) - 1);
  entry->timestamp[sizeof(entry->timestamp) - 1] = '\0';
  entry->level = level;
  strncpy(entry->file, filename, sizeof(entry->file) - 1);
  entry->file[sizeof(entry->file) - 1] = '\0';
  entry->line = line;
  strncpy(entry->message, message, sizeof(entry->message) - 1);
  entry->message[sizeof(entry->message) - 1] = '\0';

  // Auto-scroll to bottom if enabled and already at bottom
  if (console->auto_scroll && console->scroll_offset == 0) {
    console_scroll_to_bottom(console);
  }

  // Mark console as needing redraw if visible
  if (console->visible) {
    console->needs_redraw = true;
  }
}

void console_toggle(ConsoleState *console) {
  if (!console)
    return;
  console->visible = !console->visible;
  if (console->visible) {
    console_scroll_to_bottom(console);
  }
}

void console_set_visible(ConsoleState *console, bool visible) {
  if (!console)
    return;
  console->visible = visible;
  if (visible) {
    console_scroll_to_bottom(console);
  }
}

bool console_is_visible(const ConsoleState *console) {
  return console && console->visible;
}

void console_scroll(ConsoleState *console, int direction) {
  if (!console)
    return;

  int max_scroll = (int)console->count - 1;
  if (max_scroll < 0)
    max_scroll = 0;

  console->scroll_offset += direction;
  if (console->scroll_offset < 0)
    console->scroll_offset = 0;
  if (console->scroll_offset > max_scroll)
    console->scroll_offset = max_scroll;

  // Disable auto-scroll if user manually scrolls
  if (direction != 0)
    console->auto_scroll = (console->scroll_offset == 0);
}

void console_scroll_to_bottom(ConsoleState *console) {
  if (!console)
    return;
  console->scroll_offset = 0;
  console->auto_scroll = true;
}

void console_clear(ConsoleState *console) {
  if (!console)
    return;
  console->count = 0;
  console->head = 0;
  console->tail = 0;
  console->scroll_offset = 0;
}

void console_set_min_level(ConsoleState *console, LogLevel min_level) {
  if (!console)
    return;
  console->min_level = min_level;
}

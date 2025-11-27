#include "core/macros.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *pattern;
  size_t pattern_len;
  const char *replacement;
} MacroEntry;

static size_t count_replacements(const char *input, const MacroEntry *macros,
                                 size_t macro_count) {
  size_t total_growth = 0;
  const char *p = input;

  while (*p) {
    int found = 0;
    for (size_t i = 0; i < macro_count; i++) {
      if (strncmp(p, macros[i].pattern, macros[i].pattern_len) == 0) {
        size_t repl_len =
            macros[i].replacement ? strlen(macros[i].replacement) : 0;
        if (repl_len > macros[i].pattern_len) {
          total_growth += repl_len - macros[i].pattern_len;
        }
        p += macros[i].pattern_len;
        found = 1;
        break;
      }
    }
    if (!found)
      p++;
  }

  return total_growth;
}

char *macro_substitute(const char *input, const char *char_name,
                       const char *user_name) {
  if (!input)
    return NULL;

  if (!char_name)
    char_name = "";
  if (!user_name)
    user_name = "User";

  MacroEntry macros[] = {
      {"{{char}}", 8, char_name},
      {"{{Char}}", 8, char_name},
      {"{{CHAR}}", 8, char_name},
      {"{{user}}", 8, user_name},
      {"{{User}}", 8, user_name},
      {"{{USER}}", 8, user_name},
      {"{{charIfNotGroup}}", 18, char_name},
      {"{{charIfNotGroup}}", 18, char_name},
      {"<USER>", 6, user_name},
      {"<BOT>", 5, char_name},
      {"<CHAR>", 6, char_name},
  };
  size_t macro_count = sizeof(macros) / sizeof(macros[0]);

  size_t input_len = strlen(input);
  size_t extra = count_replacements(input, macros, macro_count);
  size_t output_cap = input_len + extra + 1;

  char *output = malloc(output_cap);
  if (!output)
    return NULL;

  const char *src = input;
  char *dst = output;

  while (*src) {
    int found = 0;
    for (size_t i = 0; i < macro_count; i++) {
      if (strncmp(src, macros[i].pattern, macros[i].pattern_len) == 0) {
        if (macros[i].replacement) {
          size_t repl_len = strlen(macros[i].replacement);
          memcpy(dst, macros[i].replacement, repl_len);
          dst += repl_len;
        }
        src += macros[i].pattern_len;
        found = 1;
        break;
      }
    }
    if (!found) {
      *dst++ = *src++;
    }
  }

  *dst = '\0';
  return output;
}

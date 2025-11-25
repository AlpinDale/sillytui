#ifndef MACROS_H
#define MACROS_H

#include <stddef.h>

char *macro_substitute(const char *input, const char *char_name,
                       const char *user_name);

#endif

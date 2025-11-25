#ifndef PERSONA_H
#define PERSONA_H

#include <stdbool.h>

#define PERSONA_NAME_MAX 64
#define PERSONA_DESC_MAX 4096

typedef struct {
  char name[PERSONA_NAME_MAX];
  char description[PERSONA_DESC_MAX];
} Persona;

bool persona_load(Persona *persona);
bool persona_save(const Persona *persona);
const char *persona_get_name(const Persona *persona);
const char *persona_get_description(const Persona *persona);
void persona_set_name(Persona *persona, const char *name);
void persona_set_description(Persona *persona, const char *description);

#endif

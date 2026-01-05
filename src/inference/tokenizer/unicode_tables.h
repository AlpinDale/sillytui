#ifndef UNICODE_TABLES_H
#define UNICODE_TABLES_H

#include <stdbool.h>
#include <stdint.h>

#define UNICODE_TABLE_ASCII_SIZE 128
#define UNICODE_TABLE_BMP_SIZE 8192

extern const uint8_t UNICODE_ASCII_LETTER[UNICODE_TABLE_ASCII_SIZE];
extern const uint8_t UNICODE_ASCII_NUMBER[UNICODE_TABLE_ASCII_SIZE];

extern const uint8_t UNICODE_BMP_LETTER[UNICODE_TABLE_BMP_SIZE];
extern const uint8_t UNICODE_BMP_NUMBER[UNICODE_TABLE_BMP_SIZE];

static inline bool unicode_bmp_is_letter(uint32_t cp) {
  if (cp < 128)
    return UNICODE_ASCII_LETTER[cp];
  if (cp < 0x10000)
    return (UNICODE_BMP_LETTER[cp >> 3] >> (cp & 7)) & 1;
  return false;
}

static inline bool unicode_bmp_is_number(uint32_t cp) {
  if (cp < 128)
    return UNICODE_ASCII_NUMBER[cp];
  if (cp < 0x10000)
    return (UNICODE_BMP_NUMBER[cp >> 3] >> (cp & 7)) & 1;
  return false;
}

void unicode_tables_init(void);

#endif

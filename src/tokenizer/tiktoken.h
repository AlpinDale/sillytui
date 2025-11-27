#ifndef TIKTOKEN_H
#define TIKTOKEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_TOKEN_BYTES 256
#define MAX_VOCAB_SIZE 250000
#define HASH_TABLE_SIZE 262144

typedef struct {
  uint8_t bytes[MAX_TOKEN_BYTES];
  size_t len;
  uint32_t rank;
} TokenEntry;

typedef struct {
  uint8_t *bytes;
  size_t len;
  uint32_t rank;
  bool occupied;
} HashEntry;

typedef struct {
  TokenEntry *entries;
  size_t count;
  size_t capacity;

  uint32_t *byte_to_rank;

  HashEntry *hash_table;
  size_t hash_size;

  uint32_t eot_token;
  char name[64];
  bool loaded;
} Tokenizer;

typedef struct {
  uint8_t *bytes;
  size_t len;
  size_t cap;
} ByteBuffer;

typedef struct {
  size_t start;
  size_t end;
} TextSpan;

typedef struct {
  TextSpan *spans;
  size_t count;
  size_t cap;
} SpanList;

void tokenizer_init(Tokenizer *t);
void tokenizer_free(Tokenizer *t);

bool tokenizer_load_tiktoken(Tokenizer *t, const char *path);
bool tokenizer_load_tiktoken_from_memory(Tokenizer *t, const uint8_t *data,
                                         size_t len);

int tokenizer_encode(const Tokenizer *t, const char *text, uint32_t *out_tokens,
                     size_t max_tokens);

int tokenizer_count_tokens(const Tokenizer *t, const char *text);

char *tokenizer_decode(const Tokenizer *t, const uint32_t *tokens,
                       size_t count);

bool unicode_is_letter(uint32_t cp);
bool unicode_is_number(uint32_t cp);
bool unicode_is_whitespace(uint32_t cp);

int utf8_decode(const uint8_t *bytes, size_t len, uint32_t *out_cp);
int utf8_encode(uint32_t cp, uint8_t *out);

int pretokenize_cl100k(const char *text, SpanList *spans);
int bpe_encode_piece(const Tokenizer *t, const uint8_t *piece, size_t piece_len,
                     uint32_t *out_tokens, size_t max_tokens);
uint32_t lookup_rank(const Tokenizer *t, const uint8_t *bytes, size_t len);

#endif

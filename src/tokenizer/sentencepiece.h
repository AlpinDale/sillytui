#ifndef SENTENCEPIECE_H
#define SENTENCEPIECE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SP_MAX_PIECE_LEN 128
#define SP_MAX_VOCAB_SIZE 128000

typedef enum {
  SP_PIECE_NORMAL = 1,
  SP_PIECE_UNKNOWN = 2,
  SP_PIECE_CONTROL = 3,
  SP_PIECE_USER_DEFINED = 4,
  SP_PIECE_UNUSED = 5,
  SP_PIECE_BYTE = 6
} SPPieceType;

typedef enum {
  SP_MODEL_UNIGRAM = 1,
  SP_MODEL_BPE = 2,
  SP_MODEL_WORD = 3,
  SP_MODEL_CHAR = 4
} SPModelType;

typedef struct {
  char *piece;
  float score;
  SPPieceType type;
  uint16_t len;
} SPPiece;

typedef struct {
  int32_t *base;
  int32_t *check;
  int32_t *value;
  float *score;
  size_t size;
  size_t capacity;
} DoubleArrayTrie;

typedef struct {
  SPPiece *pieces;
  size_t vocab_size;

  SPModelType model_type;

  int unk_id;
  int bos_id;
  int eos_id;
  int pad_id;

  bool add_dummy_prefix;
  bool remove_extra_whitespaces;

  uint32_t *hash_table;
  size_t hash_size;

  uint8_t first_byte_max_len[256];
  float *scores_array;

  DoubleArrayTrie trie;

  bool loaded;
} SentencePieceProcessor;

void sp_init(SentencePieceProcessor *sp);
void sp_free(SentencePieceProcessor *sp);

bool sp_load(SentencePieceProcessor *sp, const char *model_path);
bool sp_load_from_memory(SentencePieceProcessor *sp, const uint8_t *data,
                         size_t len);

int sp_encode(const SentencePieceProcessor *sp, const char *text,
              uint32_t *out_ids, size_t max_ids);

int sp_encode_as_pieces(const SentencePieceProcessor *sp, const char *text,
                        char **out_pieces, size_t max_pieces);

char *sp_decode(const SentencePieceProcessor *sp, const uint32_t *ids,
                size_t count);

int sp_piece_to_id(const SentencePieceProcessor *sp, const char *piece);
int sp_piece_to_id_n(const SentencePieceProcessor *sp, const char *piece,
                     size_t len);
const char *sp_id_to_piece(const SentencePieceProcessor *sp, int id);
float sp_get_score(const SentencePieceProcessor *sp, int id);

int sp_vocab_size(const SentencePieceProcessor *sp);

#endif

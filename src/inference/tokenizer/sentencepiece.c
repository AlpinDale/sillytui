#include "inference/tokenizer/sentencepiece.h"
#include "inference/tokenizer/simd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 262144

static inline uint32_t sp_hash(const char *str, size_t len) {
  return (uint32_t)simd_hash_bytes((const uint8_t *)str, len);
}

static size_t read_varint(const uint8_t *data, size_t len, uint64_t *out) {
  uint64_t result = 0;
  size_t shift = 0;
  size_t i = 0;
  while (i < len && i < 10) {
    uint8_t b = data[i];
    result |= (uint64_t)(b & 0x7F) << shift;
    i++;
    if ((b & 0x80) == 0) {
      *out = result;
      return i;
    }
    shift += 7;
  }
  *out = result;
  return i;
}

static float read_float32(const uint8_t *data) {
  float f;
  memcpy(&f, data, 4);
  return f;
}

typedef struct {
  const uint8_t *data;
  size_t len;
  size_t pos;
} ProtoReader;

static bool proto_has_more(ProtoReader *r) { return r->pos < r->len; }

static bool proto_read_tag(ProtoReader *r, uint32_t *field, uint32_t *wire) {
  if (r->pos >= r->len)
    return false;
  uint64_t tag;
  size_t n = read_varint(r->data + r->pos, r->len - r->pos, &tag);
  r->pos += n;
  *field = (uint32_t)(tag >> 3);
  *wire = (uint32_t)(tag & 0x7);
  return true;
}

static bool proto_skip_field(ProtoReader *r, uint32_t wire) {
  switch (wire) {
  case 0: {
    uint64_t v;
    r->pos += read_varint(r->data + r->pos, r->len - r->pos, &v);
    return true;
  }
  case 1:
    r->pos += 8;
    return r->pos <= r->len;
  case 2: {
    uint64_t len;
    r->pos += read_varint(r->data + r->pos, r->len - r->pos, &len);
    r->pos += len;
    return r->pos <= r->len;
  }
  case 5:
    r->pos += 4;
    return r->pos <= r->len;
  default:
    return false;
  }
}

static bool proto_read_bytes(ProtoReader *r, const uint8_t **out,
                             size_t *out_len) {
  uint64_t len;
  r->pos += read_varint(r->data + r->pos, r->len - r->pos, &len);
  if (r->pos + len > r->len)
    return false;
  *out = r->data + r->pos;
  *out_len = (size_t)len;
  r->pos += len;
  return true;
}

static bool proto_read_varint(ProtoReader *r, uint64_t *out) {
  r->pos += read_varint(r->data + r->pos, r->len - r->pos, out);
  return r->pos <= r->len;
}

static bool proto_read_float(ProtoReader *r, float *out) {
  if (r->pos + 4 > r->len)
    return false;
  *out = read_float32(r->data + r->pos);
  r->pos += 4;
  return true;
}

static bool parse_sentence_piece(ProtoReader *r, SPPiece *piece) {
  piece->piece = NULL;
  piece->score = 0.0f;
  piece->type = SP_PIECE_NORMAL;
  piece->len = 0;

  while (proto_has_more(r)) {
    uint32_t field, wire;
    if (!proto_read_tag(r, &field, &wire))
      break;

    switch (field) {
    case 1:
      if (wire == 2) {
        const uint8_t *data;
        size_t len;
        if (!proto_read_bytes(r, &data, &len))
          return false;
        piece->piece = malloc(len + 1);
        if (!piece->piece)
          return false;
        memcpy(piece->piece, data, len);
        piece->piece[len] = '\0';
        piece->len = (uint16_t)len;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    case 2:
      if (wire == 5) {
        if (!proto_read_float(r, &piece->score))
          return false;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    case 3:
      if (wire == 0) {
        uint64_t type;
        if (!proto_read_varint(r, &type))
          return false;
        piece->type = (SPPieceType)type;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    default:
      if (!proto_skip_field(r, wire))
        return false;
      break;
    }
  }
  return piece->piece != NULL;
}

static bool parse_trainer_spec(ProtoReader *r, SentencePieceProcessor *sp) {
  while (proto_has_more(r)) {
    uint32_t field, wire;
    if (!proto_read_tag(r, &field, &wire))
      break;

    switch (field) {
    case 3:
      if (wire == 0) {
        uint64_t type;
        if (!proto_read_varint(r, &type))
          return false;
        sp->model_type = (SPModelType)type;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    case 40:
      if (wire == 0) {
        uint64_t v;
        if (!proto_read_varint(r, &v))
          return false;
        sp->unk_id = (int)v;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    case 41:
      if (wire == 0) {
        uint64_t v;
        if (!proto_read_varint(r, &v))
          return false;
        sp->bos_id = (int)v;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    case 42:
      if (wire == 0) {
        uint64_t v;
        if (!proto_read_varint(r, &v))
          return false;
        sp->eos_id = (int)v;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    case 43:
      if (wire == 0) {
        uint64_t v;
        if (!proto_read_varint(r, &v))
          return false;
        sp->pad_id = (int)v;
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    default:
      if (!proto_skip_field(r, wire))
        return false;
      break;
    }
  }
  return true;
}

static bool parse_normalizer_spec(ProtoReader *r, SentencePieceProcessor *sp) {
  while (proto_has_more(r)) {
    uint32_t field, wire;
    if (!proto_read_tag(r, &field, &wire))
      break;

    switch (field) {
    case 3:
      if (wire == 0) {
        uint64_t v;
        if (!proto_read_varint(r, &v))
          return false;
        sp->add_dummy_prefix = (v != 0);
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    case 4:
      if (wire == 0) {
        uint64_t v;
        if (!proto_read_varint(r, &v))
          return false;
        sp->remove_extra_whitespaces = (v != 0);
      } else {
        if (!proto_skip_field(r, wire))
          return false;
      }
      break;
    default:
      if (!proto_skip_field(r, wire))
        return false;
      break;
    }
  }
  return true;
}

typedef struct {
  int32_t children[256];
  int32_t piece_id;
  float score;
} TempTrieNode;

static void dat_ensure_capacity(DoubleArrayTrie *dat, size_t needed) {
  if (needed <= dat->capacity)
    return;
  size_t new_cap = dat->capacity == 0 ? 4096 : dat->capacity;
  while (new_cap < needed)
    new_cap *= 2;

  dat->base = realloc(dat->base, new_cap * sizeof(int32_t));
  dat->check = realloc(dat->check, new_cap * sizeof(int32_t));
  dat->value = realloc(dat->value, new_cap * sizeof(int32_t));
  dat->score = realloc(dat->score, new_cap * sizeof(float));

  for (size_t i = dat->capacity; i < new_cap; i++) {
    dat->base[i] = 0;
    dat->check[i] = -1;
    dat->value[i] = -1;
    dat->score[i] = 0.0f;
  }
  dat->capacity = new_cap;
  if (dat->size < new_cap)
    dat->size = new_cap;
}

static int32_t dat_find_base(DoubleArrayTrie *dat, const uint8_t *labels,
                             int count) {
  int32_t base = 1;
  while (1) {
    bool ok = true;
    int32_t max_needed = 0;
    for (int i = 0; i < count; i++) {
      int32_t pos = base + labels[i];
      if (pos > max_needed)
        max_needed = pos;
    }

    dat_ensure_capacity(dat, max_needed + 1);

    for (int i = 0; i < count; i++) {
      int32_t pos = base + labels[i];
      if (dat->check[pos] >= 0) {
        ok = false;
        break;
      }
    }
    if (ok)
      return base;
    base++;
  }
}

static void dat_build_recursive(DoubleArrayTrie *dat, TempTrieNode *nodes,
                                int32_t node_idx, int32_t dat_idx) {
  TempTrieNode *node = &nodes[node_idx];

  dat->value[dat_idx] = node->piece_id;
  dat->score[dat_idx] = node->score;

  uint8_t labels[256];
  int32_t child_nodes[256];
  int count = 0;

  for (int c = 0; c < 256; c++) {
    if (node->children[c] >= 0) {
      labels[count] = (uint8_t)c;
      child_nodes[count] = node->children[c];
      count++;
    }
  }

  if (count == 0) {
    dat->base[dat_idx] = 0;
    return;
  }

  int32_t base = dat_find_base(dat, labels, count);
  dat->base[dat_idx] = base;

  int32_t child_dat_indices[256];
  for (int i = 0; i < count; i++) {
    child_dat_indices[i] = base + labels[i];
    dat->check[child_dat_indices[i]] = dat_idx;
  }

  for (int i = 0; i < count; i++) {
    dat_build_recursive(dat, nodes, child_nodes[i], child_dat_indices[i]);
  }
}

static void build_double_array_trie(SentencePieceProcessor *sp,
                                    TempTrieNode *nodes,
                                    size_t node_count __attribute__((unused))) {
  DoubleArrayTrie *dat = &sp->trie;
  dat->size = 0;
  dat->capacity = 0;
  dat->base = NULL;
  dat->check = NULL;
  dat->value = NULL;
  dat->score = NULL;

  dat_ensure_capacity(dat, 512);
  dat->check[0] = -1;

  dat_build_recursive(dat, nodes, 0, 0);
}

static void build_hash_table(SentencePieceProcessor *sp) {
  sp->hash_size = HASH_TABLE_SIZE;
  sp->hash_table = calloc(sp->hash_size, sizeof(uint32_t));
  if (!sp->hash_table)
    return;

  for (size_t i = 0; i < sp->hash_size; i++) {
    sp->hash_table[i] = UINT32_MAX;
  }

  memset(sp->first_byte_max_len, 0, 256);
  sp->scores_array = malloc(sp->vocab_size * sizeof(float));

  size_t temp_trie_size = 1;
  size_t temp_trie_cap = 4096;
  TempTrieNode *temp_trie = calloc(temp_trie_cap, sizeof(TempTrieNode));
  if (!temp_trie)
    return;

  for (size_t i = 0; i < temp_trie_cap; i++) {
    memset(temp_trie[i].children, 0xFF, sizeof(temp_trie[i].children));
    temp_trie[i].piece_id = -1;
  }

  for (size_t i = 0; i < sp->vocab_size; i++) {
    if (!sp->pieces[i].piece)
      continue;

    uint32_t h =
        sp_hash(sp->pieces[i].piece, sp->pieces[i].len) & (sp->hash_size - 1);
    while (sp->hash_table[h] != UINT32_MAX) {
      h = (h + 1) & (sp->hash_size - 1);
    }
    sp->hash_table[h] = (uint32_t)i;

    uint8_t first_byte = (uint8_t)sp->pieces[i].piece[0];
    if (sp->pieces[i].len > sp->first_byte_max_len[first_byte]) {
      sp->first_byte_max_len[first_byte] = (uint8_t)sp->pieces[i].len;
    }

    if (sp->scores_array) {
      sp->scores_array[i] = sp->pieces[i].score;
    }

    int32_t node = 0;
    for (size_t j = 0; j < sp->pieces[i].len; j++) {
      uint8_t c = (uint8_t)sp->pieces[i].piece[j];
      int32_t next = temp_trie[node].children[c];
      if (next < 0) {
        if (temp_trie_size >= temp_trie_cap) {
          size_t new_cap = temp_trie_cap * 2;
          TempTrieNode *new_trie =
              realloc(temp_trie, new_cap * sizeof(TempTrieNode));
          if (!new_trie) {
            free(temp_trie);
            return;
          }
          temp_trie = new_trie;
          for (size_t k = temp_trie_cap; k < new_cap; k++) {
            memset(temp_trie[k].children, 0xFF, sizeof(temp_trie[k].children));
            temp_trie[k].piece_id = -1;
          }
          temp_trie_cap = new_cap;
        }
        next = (int32_t)temp_trie_size++;
        temp_trie[node].children[c] = next;
      }
      node = next;
    }
    temp_trie[node].piece_id = (int32_t)i;
    temp_trie[node].score = sp->pieces[i].score;
  }

  build_double_array_trie(sp, temp_trie, temp_trie_size);
  free(temp_trie);
}

void sp_init(SentencePieceProcessor *sp) {
  memset(sp, 0, sizeof(*sp));
  sp->unk_id = 0;
  sp->bos_id = 1;
  sp->eos_id = 2;
  sp->pad_id = -1;
  sp->add_dummy_prefix = true;
  sp->remove_extra_whitespaces = true;
  sp->model_type = SP_MODEL_UNIGRAM;
}

void sp_free(SentencePieceProcessor *sp) {
  if (sp->pieces) {
    for (size_t i = 0; i < sp->vocab_size; i++) {
      free(sp->pieces[i].piece);
    }
    free(sp->pieces);
  }
  free(sp->hash_table);
  free(sp->scores_array);
  free(sp->trie.base);
  free(sp->trie.check);
  free(sp->trie.value);
  free(sp->trie.score);
  memset(sp, 0, sizeof(*sp));
}

bool sp_load_from_memory(SentencePieceProcessor *sp, const uint8_t *data,
                         size_t len) {
  sp_free(sp);
  sp_init(sp);

  size_t pieces_capacity = 1024;
  sp->pieces = malloc(pieces_capacity * sizeof(SPPiece));
  if (!sp->pieces)
    return false;
  sp->vocab_size = 0;

  ProtoReader r = {data, len, 0};

  while (proto_has_more(&r)) {
    uint32_t field, wire;
    if (!proto_read_tag(&r, &field, &wire))
      break;

    switch (field) {
    case 1:
      if (wire == 2) {
        const uint8_t *piece_data;
        size_t piece_len;
        if (!proto_read_bytes(&r, &piece_data, &piece_len))
          goto error;

        if (sp->vocab_size >= pieces_capacity) {
          pieces_capacity *= 2;
          SPPiece *new_pieces =
              realloc(sp->pieces, pieces_capacity * sizeof(SPPiece));
          if (!new_pieces)
            goto error;
          sp->pieces = new_pieces;
        }

        ProtoReader piece_reader = {piece_data, piece_len, 0};
        if (!parse_sentence_piece(&piece_reader, &sp->pieces[sp->vocab_size])) {
          goto error;
        }
        sp->vocab_size++;
      } else {
        if (!proto_skip_field(&r, wire))
          goto error;
      }
      break;
    case 2:
      if (wire == 2) {
        const uint8_t *spec_data;
        size_t spec_len;
        if (!proto_read_bytes(&r, &spec_data, &spec_len))
          goto error;
        ProtoReader spec_reader = {spec_data, spec_len, 0};
        if (!parse_trainer_spec(&spec_reader, sp))
          goto error;
      } else {
        if (!proto_skip_field(&r, wire))
          goto error;
      }
      break;
    case 3:
      if (wire == 2) {
        const uint8_t *spec_data;
        size_t spec_len;
        if (!proto_read_bytes(&r, &spec_data, &spec_len))
          goto error;
        ProtoReader spec_reader = {spec_data, spec_len, 0};
        if (!parse_normalizer_spec(&spec_reader, sp))
          goto error;
      } else {
        if (!proto_skip_field(&r, wire))
          goto error;
      }
      break;
    default:
      if (!proto_skip_field(&r, wire))
        goto error;
      break;
    }
  }

  build_hash_table(sp);
  sp->loaded = true;
  return true;

error:
  sp_free(sp);
  return false;
}

bool sp_load(SentencePieceProcessor *sp, const char *model_path) {
  FILE *f = fopen(model_path, "rb");
  if (!f)
    return false;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *data = malloc(size);
  if (!data) {
    fclose(f);
    return false;
  }

  if (fread(data, 1, size, f) != (size_t)size) {
    free(data);
    fclose(f);
    return false;
  }
  fclose(f);

  bool result = sp_load_from_memory(sp, data, size);
  free(data);
  return result;
}

int sp_piece_to_id_n(const SentencePieceProcessor *sp, const char *piece,
                     size_t len) {
  if (!sp->hash_table || !piece)
    return -1;

  uint32_t h = sp_hash(piece, len) & (sp->hash_size - 1);

  for (size_t i = 0; i < sp->hash_size; i++) {
    uint32_t idx = sp->hash_table[h];
    if (idx == UINT32_MAX)
      return -1;

    if (sp->pieces[idx].len == len &&
        memcmp(sp->pieces[idx].piece, piece, len) == 0)
      return (int)idx;

    h = (h + 1) & (sp->hash_size - 1);
  }
  return -1;
}

int sp_piece_to_id(const SentencePieceProcessor *sp, const char *piece) {
  if (!piece)
    return -1;
  return sp_piece_to_id_n(sp, piece, strlen(piece));
}

const char *sp_id_to_piece(const SentencePieceProcessor *sp, int id) {
  if (id < 0 || (size_t)id >= sp->vocab_size)
    return NULL;
  return sp->pieces[id].piece;
}

float sp_get_score(const SentencePieceProcessor *sp, int id) {
  if (id < 0 || (size_t)id >= sp->vocab_size)
    return 0.0f;
  return sp->pieces[id].score;
}

int sp_vocab_size(const SentencePieceProcessor *sp) {
  return (int)sp->vocab_size;
}

static size_t normalize_text(const SentencePieceProcessor *sp, const char *text,
                             char *out, size_t out_cap) {
  const uint8_t *p = (const uint8_t *)text;
  uint8_t *o = (uint8_t *)out;
  uint8_t *o_end = o + out_cap - 4;

  while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    p++;

  if (!*p) {
    out[0] = '\0';
    return 0;
  }

  if (sp->add_dummy_prefix && o < o_end) {
    *o++ = 0xE2;
    *o++ = 0x96;
    *o++ = 0x81;
  }

  while (*p && o < o_end) {
    uint8_t c = *p;

    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        p++;
      if (*p && o < o_end) {
        *o++ = 0xE2;
        *o++ = 0x96;
        *o++ = 0x81;
      }
    } else if (c < 0x80) {
      *o++ = c;
      p++;
    } else {
      int len = (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
      for (int i = 0; i < len && p[i]; i++) {
        *o++ = p[i];
      }
      p += len;
    }
  }

  *o = '\0';
  return o - (uint8_t *)out;
}

typedef struct BPESymbol {
  int prev;
  int next;
  const char *piece;
  size_t len;
  bool frozen;
} BPESymbol;

typedef struct BPEPair {
  int left;
  int right;
  float score;
  size_t combined_len;
} BPEPair;

static int pair_compare(const void *a, const void *b) {
  const BPEPair *pa = a;
  const BPEPair *pb = b;
  if (pa->score < pb->score)
    return 1;
  if (pa->score > pb->score)
    return -1;
  if (pa->left > pb->left)
    return 1;
  if (pa->left < pb->left)
    return -1;
  return 0;
}

static int find_piece_id(const SentencePieceProcessor *sp, const char *piece,
                         size_t len) {
  char buf[SP_MAX_PIECE_LEN + 1];
  if (len > SP_MAX_PIECE_LEN)
    return -1;
  memcpy(buf, piece, len);
  buf[len] = '\0';
  return sp_piece_to_id(sp, buf);
}

static int sp_encode_bpe(const SentencePieceProcessor *sp, const char *text,
                         uint32_t *out_ids, size_t max_ids) {
  char normalized[8192];
  size_t norm_len = normalize_text(sp, text, normalized, sizeof(normalized));
  if (norm_len == 0)
    return 0;

  BPESymbol symbols[4096];
  int num_symbols = 0;

  const char *p = normalized;
  while (*p && num_symbols < 4096) {
    int mblen = 1;
    if ((*p & 0x80) == 0)
      mblen = 1;
    else if ((*p & 0xE0) == 0xC0)
      mblen = 2;
    else if ((*p & 0xF0) == 0xE0)
      mblen = 3;
    else if ((*p & 0xF8) == 0xF0)
      mblen = 4;

    symbols[num_symbols].piece = p;
    symbols[num_symbols].len = mblen;
    symbols[num_symbols].prev = num_symbols > 0 ? num_symbols - 1 : -1;
    symbols[num_symbols].next = -1;
    symbols[num_symbols].frozen = false;

    if (num_symbols > 0)
      symbols[num_symbols - 1].next = num_symbols;

    num_symbols++;
    p += mblen;
  }

  if (num_symbols == 0)
    return 0;

  BPEPair pairs[8192];
  int num_pairs = 0;

  for (int i = 0; i < num_symbols - 1; i++) {
    if (symbols[i].frozen || symbols[i + 1].frozen)
      continue;

    char combined[SP_MAX_PIECE_LEN * 2 + 1];
    size_t combined_len = symbols[i].len + symbols[i + 1].len;
    if (combined_len > SP_MAX_PIECE_LEN * 2)
      continue;

    memcpy(combined, symbols[i].piece, symbols[i].len);
    memcpy(combined + symbols[i].len, symbols[i + 1].piece, symbols[i + 1].len);
    combined[combined_len] = '\0';

    int id = sp_piece_to_id(sp, combined);
    if (id >= 0) {
      pairs[num_pairs].left = i;
      pairs[num_pairs].right = i + 1;
      pairs[num_pairs].score = sp->pieces[id].score;
      pairs[num_pairs].combined_len = combined_len;
      num_pairs++;
    }
  }

  while (num_pairs > 0) {
    qsort(pairs, num_pairs, sizeof(BPEPair), pair_compare);

    BPEPair best = pairs[0];

    if (symbols[best.left].len == 0 || symbols[best.right].len == 0 ||
        symbols[best.left].len + symbols[best.right].len != best.combined_len) {
      memmove(&pairs[0], &pairs[1], (num_pairs - 1) * sizeof(BPEPair));
      num_pairs--;
      continue;
    }

    symbols[best.left].len = best.combined_len;
    symbols[best.left].next = symbols[best.right].next;
    if (symbols[best.right].next >= 0) {
      symbols[symbols[best.right].next].prev = best.left;
    }
    symbols[best.right].len = 0;

    memmove(&pairs[0], &pairs[1], (num_pairs - 1) * sizeof(BPEPair));
    num_pairs--;

    int prev = symbols[best.left].prev;
    if (prev >= 0 && !symbols[prev].frozen) {
      char combined[SP_MAX_PIECE_LEN * 2 + 1];
      size_t combined_len = symbols[prev].len + symbols[best.left].len;
      if (combined_len <= SP_MAX_PIECE_LEN * 2) {
        memcpy(combined, symbols[prev].piece, symbols[prev].len);
        memcpy(combined + symbols[prev].len, symbols[best.left].piece,
               symbols[best.left].len);
        combined[combined_len] = '\0';

        int id = sp_piece_to_id(sp, combined);
        if (id >= 0 && num_pairs < 8192) {
          pairs[num_pairs].left = prev;
          pairs[num_pairs].right = best.left;
          pairs[num_pairs].score = sp->pieces[id].score;
          pairs[num_pairs].combined_len = combined_len;
          num_pairs++;
        }
      }
    }

    int next = symbols[best.left].next;
    if (next >= 0 && !symbols[next].frozen) {
      char combined[SP_MAX_PIECE_LEN * 2 + 1];
      size_t combined_len = symbols[best.left].len + symbols[next].len;
      if (combined_len <= SP_MAX_PIECE_LEN * 2) {
        memcpy(combined, symbols[best.left].piece, symbols[best.left].len);
        memcpy(combined + symbols[best.left].len, symbols[next].piece,
               symbols[next].len);
        combined[combined_len] = '\0';

        int id = sp_piece_to_id(sp, combined);
        if (id >= 0 && num_pairs < 8192) {
          pairs[num_pairs].left = best.left;
          pairs[num_pairs].right = next;
          pairs[num_pairs].score = sp->pieces[id].score;
          pairs[num_pairs].combined_len = combined_len;
          num_pairs++;
        }
      }
    }
  }

  int out_count = 0;
  for (int idx = 0; idx >= 0 && idx < num_symbols; idx = symbols[idx].next) {
    if (symbols[idx].len == 0)
      continue;

    int id = find_piece_id(sp, symbols[idx].piece, symbols[idx].len);
    if (id < 0) {
      for (size_t i = 0; i < symbols[idx].len && out_count < (int)max_ids;
           i++) {
        char byte_piece[8];
        snprintf(byte_piece, sizeof(byte_piece), "<0x%02X>",
                 (uint8_t)symbols[idx].piece[i]);
        int byte_id = sp_piece_to_id(sp, byte_piece);
        if (byte_id >= 0) {
          out_ids[out_count++] = byte_id;
        } else {
          out_ids[out_count++] = sp->unk_id;
        }
      }
    } else {
      if (out_count < (int)max_ids) {
        out_ids[out_count++] = id;
      }
    }
  }

  return out_count;
}

#define VITERBI_MAX_CHARS 8192

static inline int compute_char_positions(const char *text, size_t len,
                                         int *positions, int max_chars) {
  int num_chars = 0;
  size_t i = 0;

  if (simd_is_all_ascii((const uint8_t *)text, len)) {
    int limit = (len < (size_t)(max_chars - 1)) ? (int)len : (max_chars - 1);
    for (int j = 0; j < limit; j++) {
      positions[j] = j;
    }
    positions[limit] = (int)len;
    return limit;
  }

  while (i < len && num_chars < max_chars - 1) {
    positions[num_chars] = (int)i;
    unsigned char c = text[i];
    int mblen = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
    i += mblen;
    num_chars++;
  }
  positions[num_chars] = (int)i;
  return num_chars;
}

static int sp_encode_unigram(const SentencePieceProcessor *sp, const char *text,
                             uint32_t *out_ids, size_t max_ids) {
  char normalized[8192];
  size_t norm_len = normalize_text(sp, text, normalized, sizeof(normalized));
  if (norm_len == 0)
    return 0;

  int char_positions[VITERBI_MAX_CHARS];
  int num_chars = compute_char_positions(normalized, norm_len, char_positions,
                                         VITERBI_MAX_CHARS);

  if (num_chars == 0)
    return 0;

  float best_scores[VITERBI_MAX_CHARS];
  int best_prev_pos[VITERBI_MAX_CHARS];
  int best_id[VITERBI_MAX_CHARS];

  for (int i = 0; i <= num_chars; i++) {
    best_scores[i] = -1e30f;
    best_prev_pos[i] = -1;
    best_id[i] = -1;
  }
  best_scores[0] = 0.0f;

  const float unk_penalty = -10.0f;

  const DoubleArrayTrie *dat = &sp->trie;

  for (int pos = 0; pos < num_chars; pos++) {
    float base_score = best_scores[pos];
    if (base_score < -1e29f)
      continue;

    int byte_start = char_positions[pos];
    int max_byte_pos = (int)norm_len;

    int32_t node = 0;
    bool found_any = false;
    int char_idx = pos;

    for (int byte_pos = byte_start; byte_pos < max_byte_pos;) {
      uint8_t c = (uint8_t)normalized[byte_pos];
      int32_t next = dat->base[node] + c;

      if (next < 0 || (size_t)next >= dat->size || dat->check[next] != node)
        break;

      node = next;
      byte_pos++;

      while (byte_pos < max_byte_pos && char_idx + 1 < num_chars &&
             char_positions[char_idx + 1] < byte_pos) {
        char_idx++;
      }
      if (char_positions[char_idx + 1] == byte_pos) {
        char_idx++;
      } else {
        continue;
      }

      int end_pos = char_idx;
      if (end_pos > pos && end_pos <= num_chars) {
        int32_t piece_id = dat->value[node];
        if (piece_id >= 0) {
          found_any = true;
          float score = dat->score[node];
          float new_score = base_score + score;
          if (new_score > best_scores[end_pos]) {
            best_scores[end_pos] = new_score;
            best_prev_pos[end_pos] = pos;
            best_id[end_pos] = piece_id;
          }
        }
      }

      if (end_pos - pos >= 32)
        break;
    }

    if (!found_any) {
      float new_score = base_score + unk_penalty;
      if (new_score > best_scores[pos + 1]) {
        best_scores[pos + 1] = new_score;
        best_prev_pos[pos + 1] = pos;
        best_id[pos + 1] = sp->unk_id;
      }
    }
  }

  int result_ids[4096];
  int result_count = 0;

  int cur_pos = num_chars;
  while (cur_pos > 0 && result_count < 4096) {
    int prev_pos = best_prev_pos[cur_pos];
    if (prev_pos < 0)
      break;

    result_ids[result_count++] = best_id[cur_pos];
    cur_pos = prev_pos;
  }

  int out_count = 0;
  for (int i = result_count - 1; i >= 0 && out_count < (int)max_ids; i--) {
    out_ids[out_count++] = result_ids[i];
  }

  return out_count;
}

int sp_encode(const SentencePieceProcessor *sp, const char *text,
              uint32_t *out_ids, size_t max_ids) {
  if (!sp->loaded || !text || !out_ids)
    return -1;

  if (sp->model_type == SP_MODEL_BPE) {
    return sp_encode_bpe(sp, text, out_ids, max_ids);
  }

  return sp_encode_unigram(sp, text, out_ids, max_ids);
}

char *sp_decode(const SentencePieceProcessor *sp, const uint32_t *ids,
                size_t count) {
  if (!sp->loaded || !ids)
    return NULL;

  size_t buf_cap = 1024;
  char *buf = malloc(buf_cap);
  if (!buf)
    return NULL;
  size_t buf_len = 0;

  for (size_t i = 0; i < count; i++) {
    const char *piece = sp_id_to_piece(sp, ids[i]);
    if (!piece)
      continue;

    size_t piece_len = strlen(piece);
    while (buf_len + piece_len + 1 > buf_cap) {
      buf_cap *= 2;
      char *new_buf = realloc(buf, buf_cap);
      if (!new_buf) {
        free(buf);
        return NULL;
      }
      buf = new_buf;
    }

    const char *p = piece;
    while (*p) {
      if (p[0] == '\xe2' && p[1] == '\x96' && p[2] == '\x81') {
        if (buf_len > 0 || i > 0) {
          buf[buf_len++] = ' ';
        }
        p += 3;
      } else {
        buf[buf_len++] = *p++;
      }
    }
  }

  buf[buf_len] = '\0';
  return buf;
}

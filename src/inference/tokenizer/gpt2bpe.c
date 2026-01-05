#include "gpt2bpe.h"
#include "simd.h"
#include "unicode_tables.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VOCAB_HASH_SIZE (1 << 19)
#define MERGE_HASH_SIZE (1 << 20)

static void init_byte_encoder(GPT2BPETokenizer *tok) {
  uint8_t bs[256];
  int n = 0;

  for (int b = '!'; b <= '~'; b++)
    bs[n++] = (uint8_t)b;
  for (int b = 0xA1; b <= 0xAC; b++)
    bs[n++] = (uint8_t)b;
  for (int b = 0xAE; b <= 0xFF; b++)
    bs[n++] = (uint8_t)b;

  for (int i = 0; i < n; i++) {
    tok->byte_encoder[bs[i]] = bs[i];
  }

  int extra = 256;
  for (int b = 0; b < 256; b++) {
    bool found = false;
    for (int i = 0; i < n; i++) {
      if (bs[i] == b) {
        found = true;
        break;
      }
    }
    if (!found) {
      tok->byte_encoder[b] = (uint16_t)extra++;
    }
  }

  memset(tok->byte_decoder, 0, 512);
  for (int b = 0; b < 256; b++) {
    tok->byte_decoder[tok->byte_encoder[b]] = (uint8_t)b;
  }

  for (int b = 0; b < 256; b++) {
    uint16_t enc = tok->byte_encoder[b];
    if (enc < 128) {
      tok->byte_to_utf8[b][0] = (uint8_t)enc;
      tok->byte_to_utf8_len[b] = 1;
    } else if (enc < 0x800) {
      tok->byte_to_utf8[b][0] = (uint8_t)(0xC0 | (enc >> 6));
      tok->byte_to_utf8[b][1] = (uint8_t)(0x80 | (enc & 0x3F));
      tok->byte_to_utf8_len[b] = 2;
    } else {
      tok->byte_to_utf8[b][0] = (uint8_t)(0xE0 | (enc >> 12));
      tok->byte_to_utf8[b][1] = (uint8_t)(0x80 | ((enc >> 6) & 0x3F));
      tok->byte_to_utf8[b][2] = (uint8_t)(0x80 | (enc & 0x3F));
      tok->byte_to_utf8_len[b] = 3;
    }
  }
}

static uint32_t gpt2_hash(const char *str, size_t len) {
  return simd_hash_bytes((const uint8_t *)str, len);
}

static uint64_t merge_hash_key(const char *first, size_t first_len,
                               const char *second, size_t second_len) {
  uint32_t h1 = gpt2_hash(first, first_len);
  uint32_t h2 = gpt2_hash(second, second_len);
  return ((uint64_t)h1 << 32) | h2;
}

static void build_vocab_hash(GPT2BPETokenizer *tok) {
  tok->vocab_hash_size = VOCAB_HASH_SIZE;
  tok->vocab_hash = calloc(tok->vocab_hash_size, sizeof(uint32_t));
  if (!tok->vocab_hash)
    return;

  for (size_t i = 0; i < tok->vocab_hash_size; i++) {
    tok->vocab_hash[i] = UINT32_MAX;
  }

  for (size_t i = 0; i < tok->vocab_size; i++) {
    if (!tok->tokens[i].token)
      continue;
    uint32_t h = gpt2_hash(tok->tokens[i].token, tok->tokens[i].len) &
                 (tok->vocab_hash_size - 1);
    while (tok->vocab_hash[h] != UINT32_MAX) {
      h = (h + 1) & (tok->vocab_hash_size - 1);
    }
    tok->vocab_hash[h] = (uint32_t)i;
  }
}

static void build_merge_hash(GPT2BPETokenizer *tok) {
  tok->merge_hash_size = MERGE_HASH_SIZE;
  tok->merge_hash = calloc(tok->merge_hash_size, sizeof(uint32_t));
  if (!tok->merge_hash)
    return;

  for (size_t i = 0; i < tok->merge_hash_size; i++) {
    tok->merge_hash[i] = UINT32_MAX;
  }

  for (size_t i = 0; i < tok->num_merges; i++) {
    uint64_t key =
        merge_hash_key(tok->merges[i].first, tok->merges[i].first_len,
                       tok->merges[i].second, tok->merges[i].second_len);
    uint32_t h = (uint32_t)(key & (tok->merge_hash_size - 1));
    while (tok->merge_hash[h] != UINT32_MAX) {
      h = (h + 1) & (tok->merge_hash_size - 1);
    }
    tok->merge_hash[h] = (uint32_t)i;
  }
}

static inline __attribute__((always_inline)) int
lookup_merge(const GPT2BPETokenizer *tok, const char *first, size_t first_len,
             const char *second, size_t second_len) {
  uint64_t key = merge_hash_key(first, first_len, second, second_len);
  uint32_t h = (uint32_t)(key & (tok->merge_hash_size - 1));
  const uint32_t *hash = tok->merge_hash;
  const GPT2Merge *merges = tok->merges;
  uint32_t mask = (uint32_t)(tok->merge_hash_size - 1);

  while (hash[h] != UINT32_MAX) {
    uint32_t idx = hash[h];
    if (merges[idx].first_len == first_len &&
        merges[idx].second_len == second_len &&
        memcmp(merges[idx].first, first, first_len) == 0 &&
        memcmp(merges[idx].second, second, second_len) == 0) {
      return (int)idx;
    }
    h = (h + 1) & mask;
  }
  return -1;
}

void gpt2_init(GPT2BPETokenizer *tok) {
  memset(tok, 0, sizeof(*tok));
  tok->unk_id = -1;
  tok->bos_id = -1;
  tok->eos_id = -1;
  tok->pad_id = -1;
  init_byte_encoder(tok);
}

void gpt2_free(GPT2BPETokenizer *tok) {
  if (tok->tokens) {
    for (size_t i = 0; i < tok->vocab_size; i++) {
      free(tok->tokens[i].token);
    }
    free(tok->tokens);
  }
  free(tok->merges);
  free(tok->vocab_hash);
  free(tok->merge_hash);
  free(tok->trie);
  free(tok->cache_keys);
  free(tok->cache_values);
  free(tok->cache_counts);
  memset(tok, 0, sizeof(*tok));
}

#define BPE_CACHE_SIZE 16384
#define BPE_CACHE_MAX_TOKENS 64
#define BPE_CACHE_MAX_LEN 512

static void init_bpe_cache(GPT2BPETokenizer *tok) {
  tok->cache_size = BPE_CACHE_SIZE;
  tok->cache_keys = calloc(BPE_CACHE_SIZE, sizeof(uint32_t));
  tok->cache_values =
      calloc(BPE_CACHE_SIZE * BPE_CACHE_MAX_TOKENS, sizeof(uint32_t));
  tok->cache_counts = calloc(BPE_CACHE_SIZE, sizeof(uint8_t));
}

static int cache_lookup(const GPT2BPETokenizer *tok, size_t len, uint32_t hash,
                        uint32_t *out_ids) {
  if (!tok->cache_keys || len > BPE_CACHE_MAX_LEN)
    return -1;

  uint32_t idx = hash & (BPE_CACHE_SIZE - 1);
  if (tok->cache_keys[idx] == hash && tok->cache_counts[idx] > 0) {
    int count = tok->cache_counts[idx];
    memcpy(out_ids, &tok->cache_values[idx * BPE_CACHE_MAX_TOKENS],
           count * sizeof(uint32_t));
    return count;
  }
  return -1;
}

static void cache_store(GPT2BPETokenizer *tok, uint32_t hash,
                        const uint32_t *ids, int count) {
  if (!tok->cache_keys || count > BPE_CACHE_MAX_TOKENS || count <= 0)
    return;

  uint32_t idx = hash & (BPE_CACHE_SIZE - 1);
  tok->cache_keys[idx] = hash;
  tok->cache_counts[idx] = (uint8_t)count;
  memcpy(&tok->cache_values[idx * BPE_CACHE_MAX_TOKENS], ids,
         count * sizeof(uint32_t));
}

static int trie_alloc_node(GPT2BPETokenizer *tok) {
  if (tok->trie_size >= tok->trie_cap) {
    size_t new_cap = tok->trie_cap ? tok->trie_cap * 2 : 4096;
    GPT2TrieNode *new_trie = realloc(tok->trie, new_cap * sizeof(GPT2TrieNode));
    if (!new_trie)
      return -1;
    tok->trie = new_trie;
    tok->trie_cap = new_cap;
  }
  int idx = (int)tok->trie_size++;
  memset(&tok->trie[idx], 0xFF, sizeof(GPT2TrieNode));
  tok->trie[idx].token_id = -1;
  return idx;
}

static void build_vocab_trie(GPT2BPETokenizer *tok) {
  tok->trie = NULL;
  tok->trie_size = 0;
  tok->trie_cap = 0;

  int root = trie_alloc_node(tok);
  if (root < 0)
    return;

  for (size_t i = 0; i < tok->vocab_size; i++) {
    const char *token = tok->tokens[i].token;
    size_t len = tok->tokens[i].len;

    int node = 0;
    for (size_t j = 0; j < len; j++) {
      uint8_t c = (uint8_t)token[j];
      if (tok->trie[node].children[c] < 0) {
        int child = trie_alloc_node(tok);
        if (child < 0)
          return;
        tok->trie[node].children[c] = child;
      }
      node = tok->trie[node].children[c];
    }
    tok->trie[node].token_id = (int32_t)i;
  }
}

static char *read_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(len + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t read = fread(buf, 1, len, f);
  fclose(f);

  buf[read] = '\0';
  if (out_len)
    *out_len = read;
  return buf;
}

static bool parse_vocab_json(GPT2BPETokenizer *tok, const char *json) {
  tok->tokens = calloc(GPT2_MAX_VOCAB_SIZE, sizeof(GPT2Token));
  if (!tok->tokens)
    return false;

  const char *p = json;
  while (*p && *p != '{')
    p++;
  if (!*p)
    return false;
  p++;

  size_t max_id = 0;

  while (*p) {
    while (*p &&
           (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ','))
      p++;

    if (*p == '}')
      break;

    if (*p != '"')
      break;
    p++;

    char token_buf[GPT2_MAX_TOKEN_LEN * 4];
    size_t token_len = 0;

    while (*p && *p != '"') {
      if (*p == '\\' && *(p + 1)) {
        p++;
        switch (*p) {
        case 'n':
          token_buf[token_len++] = '\n';
          break;
        case 'r':
          token_buf[token_len++] = '\r';
          break;
        case 't':
          token_buf[token_len++] = '\t';
          break;
        case '\\':
          token_buf[token_len++] = '\\';
          break;
        case '"':
          token_buf[token_len++] = '"';
          break;
        case 'u': {
          if (p[1] && p[2] && p[3] && p[4]) {
            char hex[5] = {p[1], p[2], p[3], p[4], 0};
            uint32_t codepoint = (uint32_t)strtol(hex, NULL, 16);
            p += 4;
            if (codepoint < 0x80) {
              token_buf[token_len++] = (char)codepoint;
            } else if (codepoint < 0x800) {
              token_buf[token_len++] = (char)(0xC0 | (codepoint >> 6));
              token_buf[token_len++] = (char)(0x80 | (codepoint & 0x3F));
            } else {
              token_buf[token_len++] = (char)(0xE0 | (codepoint >> 12));
              token_buf[token_len++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
              token_buf[token_len++] = (char)(0x80 | (codepoint & 0x3F));
            }
          }
          break;
        }
        default:
          token_buf[token_len++] = *p;
          break;
        }
        p++;
      } else {
        token_buf[token_len++] = *p++;
      }
      if (token_len >= sizeof(token_buf) - 4) {
        while (*p && *p != '"') {
          if (*p == '\\' && *(p + 1))
            p++;
          p++;
        }
        break;
      }
    }

    if (*p == '"')
      p++;

    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
      p++;
    if (*p == ':')
      p++;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
      p++;

    long id = strtol(p, (char **)&p, 10);
    if (id < 0 || id >= GPT2_MAX_VOCAB_SIZE)
      continue;

    tok->tokens[id].token = malloc(token_len + 1);
    if (tok->tokens[id].token) {
      memcpy(tok->tokens[id].token, token_buf, token_len);
      tok->tokens[id].token[token_len] = '\0';
      tok->tokens[id].len = (uint16_t)token_len;
    }

    if ((size_t)id >= max_id)
      max_id = id + 1;
  }

  tok->vocab_size = max_id;
  return true;
}

static bool parse_merges_txt(GPT2BPETokenizer *tok, const char *txt) {
  tok->merges = calloc(GPT2_MAX_MERGES, sizeof(GPT2Merge));
  if (!tok->merges)
    return false;

  const char *p = txt;
  size_t num_merges = 0;

  while (*p && *p != '\n')
    p++;
  if (*p == '\n')
    p++;

  while (*p && num_merges < GPT2_MAX_MERGES) {
    while (*p == ' ' || *p == '\t')
      p++;

    if (*p == '\n' || *p == '\r') {
      while (*p == '\n' || *p == '\r')
        p++;
      continue;
    }

    if (!*p)
      break;

    const char *first_start = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
      p++;
    size_t first_len = p - first_start;

    while (*p == ' ' || *p == '\t')
      p++;

    const char *second_start = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
      p++;
    size_t second_len = p - second_start;

    if (first_len > 0 && second_len > 0 && first_len < GPT2_MAX_TOKEN_LEN &&
        second_len < GPT2_MAX_TOKEN_LEN) {
      memcpy(tok->merges[num_merges].first, first_start, first_len);
      tok->merges[num_merges].first[first_len] = '\0';
      tok->merges[num_merges].first_len = (uint16_t)first_len;

      memcpy(tok->merges[num_merges].second, second_start, second_len);
      tok->merges[num_merges].second[second_len] = '\0';
      tok->merges[num_merges].second_len = (uint16_t)second_len;

      num_merges++;
    }

    while (*p && *p != '\n')
      p++;
    if (*p == '\n')
      p++;
  }

  tok->num_merges = num_merges;
  return true;
}

bool gpt2_load(GPT2BPETokenizer *tok, const char *vocab_path,
               const char *merges_path) {
  size_t vocab_len, merges_len;
  char *vocab_json = read_file(vocab_path, &vocab_len);
  char *merges_txt = read_file(merges_path, &merges_len);

  if (!vocab_json || !merges_txt) {
    free(vocab_json);
    free(merges_txt);
    return false;
  }

  if (!parse_vocab_json(tok, vocab_json)) {
    free(vocab_json);
    free(merges_txt);
    return false;
  }

  if (!parse_merges_txt(tok, merges_txt)) {
    free(vocab_json);
    free(merges_txt);
    return false;
  }

  free(vocab_json);
  free(merges_txt);

  build_vocab_hash(tok);
  build_merge_hash(tok);

  for (size_t i = 0; i < tok->vocab_size; i++) {
    if (tok->tokens[i].token) {
      if (strcmp(tok->tokens[i].token, "<|endoftext|>") == 0) {
        if (tok->unk_id < 0)
          tok->unk_id = (int)i;
        if (tok->eos_id < 0)
          tok->eos_id = (int)i;
      }
    }
  }

  build_vocab_trie(tok);
  init_bpe_cache(tok);

  tok->loaded = true;
  return true;
}

int gpt2_token_to_id(const GPT2BPETokenizer *tok, const char *token) {
  if (!tok->vocab_hash)
    return -1;

  size_t len = strlen(token);
  uint32_t h = gpt2_hash(token, len) & (tok->vocab_hash_size - 1);

  while (tok->vocab_hash[h] != UINT32_MAX) {
    uint32_t idx = tok->vocab_hash[h];
    if (tok->tokens[idx].len == len &&
        memcmp(tok->tokens[idx].token, token, len) == 0) {
      return (int)idx;
    }
    h = (h + 1) & (tok->vocab_hash_size - 1);
  }
  return tok->unk_id;
}

const char *gpt2_id_to_token(const GPT2BPETokenizer *tok, int id) {
  if (id < 0 || (size_t)id >= tok->vocab_size)
    return NULL;
  return tok->tokens[id].token;
}

int gpt2_vocab_size(const GPT2BPETokenizer *tok) {
  return (int)tok->vocab_size;
}

static int decode_utf8_char(const uint8_t *s, size_t len, uint32_t *codepoint) {
  if (len == 0)
    return 0;
  if ((s[0] & 0x80) == 0) {
    *codepoint = s[0];
    return 1;
  } else if ((s[0] & 0xE0) == 0xC0 && len >= 2) {
    *codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    return 2;
  } else if ((s[0] & 0xF0) == 0xE0 && len >= 3) {
    *codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    return 3;
  } else if ((s[0] & 0xF8) == 0xF0 && len >= 4) {
    *codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
                 ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    return 4;
  }
  *codepoint = s[0];
  return 1;
}

static bool match_pattern(const char *text, size_t len, size_t *match_len) {
  const uint8_t *s = (const uint8_t *)text;
  size_t pos = 0;
  uint32_t cp;

  if (pos < len && s[pos] == '\'') {
    pos++;
    if (pos < len) {
      char c = tolower(s[pos]);
      if (c == 's' || c == 't' || c == 'd' || c == 'm') {
        *match_len = pos + 1;
        return true;
      }
      if (pos + 1 < len) {
        char c2 = tolower(s[pos + 1]);
        if ((c == 'r' && c2 == 'e') || (c == 'v' && c2 == 'e') ||
            (c == 'l' && c2 == 'l')) {
          *match_len = pos + 2;
          return true;
        }
      }
    }
  }

  pos = 0;

  if (pos < len) {
    int char_len = decode_utf8_char(s + pos, len - pos, &cp);
    if (cp != '\r' && cp != '\n' && !unicode_bmp_is_letter(cp) &&
        !unicode_bmp_is_number(cp)) {
      pos += char_len;
    }
  }

  size_t letters_matched = 0;
  while (pos < len) {
    int char_len = decode_utf8_char(s + pos, len - pos, &cp);
    if (!unicode_bmp_is_letter(cp))
      break;
    pos += char_len;
    letters_matched++;
  }

  if (letters_matched > 0) {
    *match_len = pos;
    return true;
  }

  pos = 0;
  size_t digits = 0;
  while (pos < len && digits < 3) {
    int char_len = decode_utf8_char(s + pos, len - pos, &cp);
    if (!unicode_bmp_is_number(cp))
      break;
    pos += char_len;
    digits++;
  }
  if (digits > 0) {
    *match_len = pos;
    return true;
  }

  pos = 0;
  if (pos < len && s[pos] == ' ')
    pos++;

  size_t punct_count = 0;
  while (pos < len) {
    int char_len = decode_utf8_char(s + pos, len - pos, &cp);
    if ((cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r') ||
        unicode_bmp_is_letter(cp) || unicode_bmp_is_number(cp))
      break;
    pos += char_len;
    punct_count++;
  }

  if (punct_count > 0) {
    while (pos < len && (s[pos] == '\r' || s[pos] == '\n'))
      pos++;
    *match_len = pos;
    return true;
  }

  pos = 0;
  size_t ws_count = 0;
  while (pos < len && (s[pos] == ' ' || s[pos] == '\t')) {
    pos++;
    ws_count++;
  }
  size_t nl_count = 0;
  while (pos < len && (s[pos] == '\r' || s[pos] == '\n')) {
    pos++;
    nl_count++;
  }

  if (nl_count > 0) {
    *match_len = pos;
    return true;
  }

  if (ws_count > 0) {
    bool has_letter_after = false;
    if (pos < len) {
      int char_len = decode_utf8_char(s + pos, len - pos, &cp);
      (void)char_len;
      has_letter_after = unicode_bmp_is_letter(cp);
    }

    if (has_letter_after && ws_count > 1) {
      *match_len = ws_count - 1;
    } else {
      *match_len = ws_count;
    }
    return true;
  }

  if (len > 0) {
    int char_len = decode_utf8_char(s, len, &cp);
    *match_len = char_len;
    return true;
  }

  return false;
}

typedef struct {
  const char *str;
  uint16_t len;
  int16_t next;
  int16_t prev;
  int32_t rank;
  bool deleted;
} BPEPart;

typedef struct {
  int32_t rank;
  int16_t idx;
} HeapEntry;

static inline void heap_push(HeapEntry *heap, int *heap_size, int32_t rank,
                             int16_t idx) {
  int i = (*heap_size)++;
  heap[i].rank = rank;
  heap[i].idx = idx;
  while (i > 0) {
    int parent = (i - 1) / 2;
    if (heap[parent].rank <= heap[i].rank)
      break;
    HeapEntry tmp = heap[parent];
    heap[parent] = heap[i];
    heap[i] = tmp;
    i = parent;
  }
}

static inline HeapEntry heap_pop(HeapEntry *heap, int *heap_size) {
  HeapEntry result = heap[0];
  (*heap_size)--;
  if (*heap_size > 0) {
    heap[0] = heap[*heap_size];
    int i = 0;
    while (1) {
      int left = 2 * i + 1;
      int right = 2 * i + 2;
      int smallest = i;
      if (left < *heap_size && heap[left].rank < heap[smallest].rank)
        smallest = left;
      if (right < *heap_size && heap[right].rank < heap[smallest].rank)
        smallest = right;
      if (smallest == i)
        break;
      HeapEntry tmp = heap[i];
      heap[i] = heap[smallest];
      heap[smallest] = tmp;
      i = smallest;
    }
  }
  return result;
}

static int gpt2_token_to_id_n(const GPT2BPETokenizer *tok, const char *token,
                              size_t len) {
  if (!tok->vocab_hash)
    return -1;

  uint32_t h =
      simd_hash_bytes((const uint8_t *)token, len) & (tok->vocab_hash_size - 1);

  while (tok->vocab_hash[h] != UINT32_MAX) {
    uint32_t idx = tok->vocab_hash[h];
    if (tok->tokens[idx].len == len &&
        memcmp(tok->tokens[idx].token, token, len) == 0) {
      return (int)idx;
    }
    h = (h + 1) & (tok->vocab_hash_size - 1);
  }
  return tok->unk_id;
}

static int trie_lookup_whole(const GPT2BPETokenizer *tok, const char *text,
                             size_t len) {
  if (!tok->trie || len == 0)
    return -1;

  const GPT2TrieNode *trie = tok->trie;
  int node = 0;

  for (size_t i = 0; i < len; i++) {
    uint8_t c = (uint8_t)text[i];
    int child = trie[node].children[c];
    if (child < 0)
      return -1;
    node = child;
  }

  return trie[node].token_id;
}

static int bpe_encode_piece_ids(const GPT2BPETokenizer *tok, const char *piece,
                                size_t piece_len, uint32_t *out_ids,
                                size_t max_ids) {
  if (piece_len == 0)
    return 0;

  BPEPart parts[512];
  int num_parts = 0;

  size_t i = 0;
  while (i < piece_len && num_parts < 511) {
    uint32_t cp;
    int char_len =
        decode_utf8_char((const uint8_t *)piece + i, piece_len - i, &cp);
    parts[num_parts].str = piece + i;
    parts[num_parts].len = (uint16_t)char_len;
    parts[num_parts].prev = (int16_t)(num_parts - 1);
    parts[num_parts].next = (int16_t)(num_parts + 1);
    parts[num_parts].rank = -1;
    parts[num_parts].deleted = false;
    num_parts++;
    i += char_len;
  }

  if (num_parts == 0)
    return 0;

  parts[num_parts - 1].next = -1;

  HeapEntry heap[512];
  int heap_size = 0;

  for (int j = 0; j < num_parts - 1; j++) {
    int next = parts[j].next;
    if (next >= 0) {
      int32_t rank = lookup_merge(tok, parts[j].str, parts[j].len,
                                  parts[next].str, parts[next].len);
      parts[j].rank = rank;
      if (rank >= 0) {
        heap_push(heap, &heap_size, rank, (int16_t)j);
      }
    }
  }

  while (heap_size > 0) {
    HeapEntry top = heap_pop(heap, &heap_size);

    int idx = top.idx;
    if (parts[idx].deleted || parts[idx].rank != top.rank)
      continue;

    int next = parts[idx].next;
    if (next < 0 || parts[next].deleted)
      continue;

    parts[idx].len += parts[next].len;
    parts[next].deleted = true;

    int next_next = parts[next].next;
    parts[idx].next = (int16_t)next_next;
    if (next_next >= 0) {
      parts[next_next].prev = (int16_t)idx;
    }

    if (next_next >= 0 && !parts[next_next].deleted) {
      int32_t new_rank =
          lookup_merge(tok, parts[idx].str, parts[idx].len,
                       parts[next_next].str, parts[next_next].len);
      parts[idx].rank = new_rank;
      if (new_rank >= 0) {
        heap_push(heap, &heap_size, new_rank, (int16_t)idx);
      }
    } else {
      parts[idx].rank = -1;
    }

    int prev = parts[idx].prev;
    if (prev >= 0 && !parts[prev].deleted) {
      int32_t new_rank = lookup_merge(tok, parts[prev].str, parts[prev].len,
                                      parts[idx].str, parts[idx].len);
      parts[prev].rank = new_rank;
      if (new_rank >= 0) {
        heap_push(heap, &heap_size, new_rank, (int16_t)prev);
      }
    }
  }

  int count = 0;
  for (int j = 0; j >= 0 && (size_t)count < max_ids;) {
    if (!parts[j].deleted) {
      int id = gpt2_token_to_id_n(tok, parts[j].str, parts[j].len);
      if (id >= 0) {
        out_ids[count++] = (uint32_t)id;
      }
    }
    j = parts[j].next;
    if (j < 0)
      break;
  }

  return count;
}

int gpt2_encode(const GPT2BPETokenizer *tok, const char *text,
                uint32_t *out_ids, size_t max_ids) {
  if (!tok->loaded || !text || !out_ids || max_ids == 0)
    return 0;

  size_t text_len = strlen(text);
  size_t pos = 0;
  size_t num_ids = 0;

  while (pos < text_len && num_ids < max_ids) {
    size_t match_len = 0;
    if (!match_pattern(text + pos, text_len - pos, &match_len) ||
        match_len == 0) {
      pos++;
      continue;
    }

    char encoded[2048];
    size_t encoded_len = 0;
    for (size_t i = 0; i < match_len && encoded_len < sizeof(encoded) - 4;
         i++) {
      uint8_t b = (uint8_t)text[pos + i];
      uint8_t len = tok->byte_to_utf8_len[b];
      memcpy(encoded + encoded_len, tok->byte_to_utf8[b], len);
      encoded_len += len;
    }
    encoded[encoded_len] = '\0';

    int whole_token = trie_lookup_whole(tok, encoded, encoded_len);
    if (whole_token >= 0) {
      out_ids[num_ids++] = (uint32_t)whole_token;
    } else {
      uint32_t hash = simd_hash_bytes((const uint8_t *)encoded, encoded_len);
      int cached = cache_lookup(tok, encoded_len, hash, out_ids + num_ids);
      if (cached > 0) {
        num_ids += cached;
      } else {
        int num_tokens = bpe_encode_piece_ids(
            tok, encoded, encoded_len, out_ids + num_ids, max_ids - num_ids);
        cache_store((GPT2BPETokenizer *)tok, hash, out_ids + num_ids,
                    num_tokens);
        num_ids += num_tokens;
      }
    }

    pos += match_len;
  }

  return (int)num_ids;
}

char *gpt2_decode(const GPT2BPETokenizer *tok, const uint32_t *ids,
                  size_t count) {
  if (!tok->loaded || !ids || count == 0)
    return NULL;

  size_t buf_size = count * GPT2_MAX_TOKEN_LEN;
  char *result = malloc(buf_size);
  if (!result)
    return NULL;

  size_t out_pos = 0;

  for (size_t i = 0; i < count; i++) {
    const char *token = gpt2_id_to_token(tok, ids[i]);
    if (!token)
      continue;

    size_t token_len = strlen(token);
    const uint8_t *t = (const uint8_t *)token;
    size_t j = 0;

    while (j < token_len && out_pos < buf_size - 1) {
      uint32_t cp;
      int char_len = decode_utf8_char(t + j, token_len - j, &cp);

      if (cp < 512) {
        result[out_pos++] = (char)tok->byte_decoder[cp];
      } else {
        result[out_pos++] = '?';
      }
      j += char_len;
    }
  }

  result[out_pos] = '\0';
  return result;
}

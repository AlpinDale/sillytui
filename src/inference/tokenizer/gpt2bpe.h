#ifndef GPT2BPE_H
#define GPT2BPE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GPT2_MAX_VOCAB_SIZE 200000
#define GPT2_MAX_MERGES 400000
#define GPT2_MAX_TOKEN_LEN 256
#define GPT2_TRIE_SIZE (1 << 20)

typedef struct {
  char *token;
  uint16_t len;
} GPT2Token;

typedef struct {
  char first[GPT2_MAX_TOKEN_LEN];
  char second[GPT2_MAX_TOKEN_LEN];
  uint16_t first_len;
  uint16_t second_len;
} GPT2Merge;

typedef struct {
  int32_t children[256];
  int32_t token_id;
} GPT2TrieNode;

typedef struct {
  GPT2Token *tokens;
  size_t vocab_size;

  GPT2Merge *merges;
  size_t num_merges;

  uint32_t *vocab_hash;
  size_t vocab_hash_size;

  uint32_t *merge_hash;
  size_t merge_hash_size;

  uint16_t byte_encoder[256];
  uint8_t byte_decoder[512];

  uint8_t byte_to_utf8[256][4];
  uint8_t byte_to_utf8_len[256];

  GPT2TrieNode *trie;
  size_t trie_size;
  size_t trie_cap;

  uint32_t *cache_keys;
  uint32_t *cache_values;
  uint8_t *cache_counts;
  size_t cache_size;

  int unk_id;
  int bos_id;
  int eos_id;
  int pad_id;

  bool loaded;
} GPT2BPETokenizer;

void gpt2_init(GPT2BPETokenizer *tok);
void gpt2_free(GPT2BPETokenizer *tok);

bool gpt2_load(GPT2BPETokenizer *tok, const char *vocab_path,
               const char *merges_path);

int gpt2_encode(const GPT2BPETokenizer *tok, const char *text,
                uint32_t *out_ids, size_t max_ids);

char *gpt2_decode(const GPT2BPETokenizer *tok, const uint32_t *ids,
                  size_t count);

int gpt2_token_to_id(const GPT2BPETokenizer *tok, const char *token);
const char *gpt2_id_to_token(const GPT2BPETokenizer *tok, int id);

int gpt2_vocab_size(const GPT2BPETokenizer *tok);

#endif

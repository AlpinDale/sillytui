/*
 * Example usage of the tokenizer.
 *
 * To compile (from the project root directory):
 * cc -O3 -o tokenize examples/tokenize.c src/tokenizer.c src/simd.c \
 *     src/simd_arm64.S src/unicode_tables.c -I.
 *
 * Usage: ./tokenize <text>
 * Example: ./tokenize "Hello, world!"
 */

#include "../src/simd.h"
#include "../src/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <text>\n", argv[0]);
    fprintf(stderr, "Example: %s \"Hello, world!\"\n", argv[0]);
    return 1;
  }

  char *text;
  if (argc == 2) {
    text = argv[1];
  } else {
    size_t total_len = 0;
    for (int i = 1; i < argc; i++) {
      total_len += strlen(argv[i]) + 1;
    }
    text = malloc(total_len);
    if (!text) {
      fprintf(stderr, "Memory allocation failed\n");
      return 1;
    }
    text[0] = '\0';
    for (int i = 1; i < argc; i++) {
      if (i > 1)
        strcat(text, " ");
      strcat(text, argv[i]);
    }
  }

  Tokenizer tok;
  tokenizer_init(&tok);
  simd_init();

  if (!tokenizer_load_tiktoken(&tok, "data/cl100k_base.tiktoken")) {
    char alt_path[256];
    snprintf(alt_path, sizeof(alt_path), "%s/../data/cl100k_base.tiktoken",
             argc > 0 ? argv[0] : ".");
    if (!tokenizer_load_tiktoken(&tok, alt_path)) {
      fprintf(stderr, "Failed to load tokenizer vocabulary\n");
      fprintf(stderr, "Make sure cl100k_base.tiktoken is in ./data/\n");
      if (argc > 2)
        free(text);
      return 1;
    }
  }

  uint32_t tokens[4096];
  int token_count = tokenizer_encode(&tok, text, tokens, 4096);

  if (token_count < 0) {
    fprintf(stderr, "Tokenization failed\n");
    tokenizer_free(&tok);
    if (argc > 2)
      free(text);
    return 1;
  }

  printf("Text: \"%s\"\n", text);
  printf("Token count: %d\n\n", token_count);

  printf("Tokens: [");
  for (int i = 0; i < token_count; i++) {
    if (i > 0)
      printf(", ");
    printf("%u", tokens[i]);
  }
  printf("]\n\n");

  printf("Decoded tokens:\n");
  for (int i = 0; i < token_count; i++) {
    uint32_t single_token[1] = {tokens[i]};
    char *decoded = tokenizer_decode(&tok, single_token, 1);
    if (decoded) {
      printf("  [%d] %u -> \"%s\"\n", i, tokens[i], decoded);
      free(decoded);
    }
  }

  char *roundtrip = tokenizer_decode(&tok, tokens, token_count);
  if (roundtrip) {
    printf("\nRoundtrip: \"%s\"\n", roundtrip);
    printf("Match: %s\n", strcmp(text, roundtrip) == 0 ? "True" : "False");
    free(roundtrip);
  }

  tokenizer_free(&tok);
  if (argc > 2)
    free(text);

  return 0;
}

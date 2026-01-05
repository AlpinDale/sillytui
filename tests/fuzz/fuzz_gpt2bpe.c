#include "inference/tokenizer/gpt2bpe.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static GPT2BPETokenizer g_tok;
static int g_initialized = 0;

static void init_tokenizer(void) {
  if (g_initialized)
    return;
  gpt2_init(&g_tok);
  if (!gpt2_load(&g_tok, "tokenizers/llama3/vocab.json",
                 "tokenizers/llama3/merges.txt")) {
    exit(1);
  }
  g_initialized = 1;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  init_tokenizer();

  char *text = malloc(size + 1);
  if (!text)
    return 0;
  memcpy(text, data, size);
  text[size] = '\0';

  uint32_t tokens[4096];
  int count = gpt2_encode(&g_tok, text, tokens, 4096);

  if (count > 0) {
    char *decoded = gpt2_decode(&g_tok, tokens, count);
    if (decoded) {
      free(decoded);
    }
  }

  free(text);
  return 0;
}

#ifndef LIBFUZZER
int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    perror("fopen");
    return 1;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *data = malloc(size);
  if (!data) {
    fclose(f);
    return 1;
  }

  fread(data, 1, size, f);
  fclose(f);

  LLVMFuzzerTestOneInput(data, size);

  free(data);
  return 0;
}
#endif

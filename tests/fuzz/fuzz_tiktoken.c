#include "tokenizer/simd.h"
#include "tokenizer/tiktoken.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static Tokenizer g_tok;
static int g_initialized = 0;

static void init_tokenizer(void) {
  if (g_initialized)
    return;
  simd_init();
  tokenizer_init(&g_tok);
  if (!tokenizer_load_tiktoken(&g_tok,
                               "tokenizers/openai/cl100k_base.tiktoken")) {
    exit(1);
  }
  g_initialized = 1;
}

#ifdef __AFL_FUZZ_TESTCASE_LEN
__AFL_FUZZ_INIT();
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  init_tokenizer();

  char *text = malloc(size + 1);
  if (!text)
    return 0;
  memcpy(text, data, size);
  text[size] = '\0';

  uint32_t tokens[4096];
  int count = tokenizer_encode(&g_tok, text, tokens, 4096);

  if (count > 0) {
    char *decoded = tokenizer_decode(&g_tok, tokens, count);
    if (decoded) {
      if (strcmp(text, decoded) != 0) {
      }
      free(decoded);
    }
  }

  free(text);
  return 0;
}

#ifndef __AFL_COMPILER
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
#endif

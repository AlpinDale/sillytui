#include "../test_framework.h"
#include "test_helper.h"
#include "tokenizer/gpt2bpe.h"
#include "tokenizer/simd.h"
#include "tokenizer/tiktoken.h"

#define CL100K_PATH "tokenizers/openai/cl100k_base.tiktoken"
#define O200K_PATH "tokenizers/openai/o200k_base.tiktoken"
#define LLAMA3_VOCAB "tokenizers/llama3/vocab.json"
#define LLAMA3_MERGES "tokenizers/llama3/merges.txt"
#define QWEN3_VOCAB "tokenizers/qwen3/vocab.json"
#define QWEN3_MERGES "tokenizers/qwen3/merges.txt"

TEST(tiktoken_load_cl100k) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  bool loaded = tokenizer_load_tiktoken(&tok, CL100K_PATH);
  if (!loaded) {
    tokenizer_free(&tok);
    printf("(skipped - vocab not found) ");
    PASS();
  }

  ASSERT_TRUE(tok.loaded);
  ASSERT(tok.count > 90000);

  tokenizer_free(&tok);
  PASS();
}

TEST(tiktoken_encode_decode_roundtrip) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok, CL100K_PATH)) {
    tokenizer_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  const char *text = "Hello, world! This is a test.";
  uint32_t tokens[256];
  int count = tokenizer_encode(&tok, text, tokens, 256);
  ASSERT(count > 0);

  char *decoded = tokenizer_decode(&tok, tokens, count);
  ASSERT_NOT_NULL(decoded);
  ASSERT_EQ_STR(text, decoded);

  free(decoded);
  tokenizer_free(&tok);
  PASS();
}

TEST(tiktoken_encode_unicode) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok, CL100K_PATH)) {
    tokenizer_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  const char *text = "こんにちは世界";
  uint32_t tokens[256];
  int count = tokenizer_encode(&tok, text, tokens, 256);
  ASSERT(count > 0);

  char *decoded = tokenizer_decode(&tok, tokens, count);
  ASSERT_NOT_NULL(decoded);
  ASSERT_EQ_STR(text, decoded);

  free(decoded);
  tokenizer_free(&tok);
  PASS();
}

TEST(tiktoken_encode_emoji) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok, CL100K_PATH)) {
    tokenizer_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  const char *text = "Hello 😀🎉 World!";
  uint32_t tokens[256];
  int count = tokenizer_encode(&tok, text, tokens, 256);
  ASSERT(count > 0);

  char *decoded = tokenizer_decode(&tok, tokens, count);
  ASSERT_NOT_NULL(decoded);
  ASSERT_EQ_STR(text, decoded);

  free(decoded);
  tokenizer_free(&tok);
  PASS();
}

TEST(tiktoken_count_tokens) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok, CL100K_PATH)) {
    tokenizer_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  int count = tokenizer_count_tokens(
      &tok, "The quick brown fox jumps over the lazy dog.");
  ASSERT(count >= 9 && count <= 12);

  tokenizer_free(&tok);
  PASS();
}

TEST(tiktoken_o200k_load) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  bool loaded = tokenizer_load_tiktoken(&tok, O200K_PATH);
  if (!loaded) {
    tokenizer_free(&tok);
    printf("(skipped - vocab not found) ");
    PASS();
  }

  ASSERT_TRUE(tok.loaded);
  ASSERT(tok.count > 190000);

  tokenizer_free(&tok);
  PASS();
}

TEST(gpt2bpe_load_llama3) {
  GPT2BPETokenizer tok;
  gpt2_init(&tok);

  bool loaded = gpt2_load(&tok, LLAMA3_VOCAB, LLAMA3_MERGES);
  if (!loaded) {
    gpt2_free(&tok);
    printf("(skipped - vocab not found) ");
    PASS();
  }

  ASSERT(tok.vocab_size > 100000);

  gpt2_free(&tok);
  PASS();
}

TEST(gpt2bpe_encode_decode_roundtrip) {
  GPT2BPETokenizer tok;
  gpt2_init(&tok);

  if (!gpt2_load(&tok, LLAMA3_VOCAB, LLAMA3_MERGES)) {
    gpt2_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  const char *text = "Hello, world! This is a test.";
  uint32_t tokens[256];
  int count = gpt2_encode(&tok, text, tokens, 256);
  ASSERT(count > 0);

  char *decoded = gpt2_decode(&tok, tokens, count);
  ASSERT_NOT_NULL(decoded);
  ASSERT_EQ_STR(text, decoded);

  free(decoded);
  gpt2_free(&tok);
  PASS();
}

TEST(gpt2bpe_encode_unicode) {
  GPT2BPETokenizer tok;
  gpt2_init(&tok);

  if (!gpt2_load(&tok, LLAMA3_VOCAB, LLAMA3_MERGES)) {
    gpt2_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  const char *text = "日本語テスト";
  uint32_t tokens[256];
  int count = gpt2_encode(&tok, text, tokens, 256);
  ASSERT(count > 0);

  char *decoded = gpt2_decode(&tok, tokens, count);
  ASSERT_NOT_NULL(decoded);
  ASSERT_EQ_STR(text, decoded);

  free(decoded);
  gpt2_free(&tok);
  PASS();
}

TEST(gpt2bpe_load_qwen3) {
  GPT2BPETokenizer tok;
  gpt2_init(&tok);

  bool loaded = gpt2_load(&tok, QWEN3_VOCAB, QWEN3_MERGES);
  if (!loaded) {
    gpt2_free(&tok);
    printf("(skipped - vocab not found) ");
    PASS();
  }

  ASSERT(tok.vocab_size > 100000);

  gpt2_free(&tok);
  PASS();
}

TEST(tokenizer_empty_string) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok, CL100K_PATH)) {
    tokenizer_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  uint32_t tokens[256];
  int count = tokenizer_encode(&tok, "", tokens, 256);
  ASSERT_EQ_INT(0, count);

  tokenizer_free(&tok);
  PASS();
}

TEST(tokenizer_very_long_text) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok, CL100K_PATH)) {
    tokenizer_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  char long_text[50000];
  for (int i = 0; i < 49999; i++) {
    long_text[i] = 'a' + (i % 26);
    if (i % 100 == 99)
      long_text[i] = ' ';
  }
  long_text[49999] = '\0';

  int count = tokenizer_count_tokens(&tok, long_text);
  ASSERT(count > 1000);

  tokenizer_free(&tok);
  PASS();
}

TEST(tokenizer_special_whitespace) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok, CL100K_PATH)) {
    tokenizer_free(&tok);
    printf("(skipped) ");
    PASS();
  }

  const char *text = "Hello\t\n\r  World";
  uint32_t tokens[256];
  int count = tokenizer_encode(&tok, text, tokens, 256);
  ASSERT(count > 0);

  char *decoded = tokenizer_decode(&tok, tokens, count);
  ASSERT_NOT_NULL(decoded);
  ASSERT_EQ_STR(text, decoded);

  free(decoded);
  tokenizer_free(&tok);
  PASS();
}

void run_tokenizer_integration_tests(void) {
  TEST_SUITE("Tokenizer Integration");
  RUN_TEST(tiktoken_load_cl100k);
  RUN_TEST(tiktoken_encode_decode_roundtrip);
  RUN_TEST(tiktoken_encode_unicode);
  RUN_TEST(tiktoken_encode_emoji);
  RUN_TEST(tiktoken_count_tokens);
  RUN_TEST(tiktoken_o200k_load);
  RUN_TEST(gpt2bpe_load_llama3);
  RUN_TEST(gpt2bpe_encode_decode_roundtrip);
  RUN_TEST(gpt2bpe_encode_unicode);
  RUN_TEST(gpt2bpe_load_qwen3);
  RUN_TEST(tokenizer_empty_string);
  RUN_TEST(tokenizer_very_long_text);
  RUN_TEST(tokenizer_special_whitespace);
}

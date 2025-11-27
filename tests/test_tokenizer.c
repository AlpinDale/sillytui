#include "test_framework.h"
#include "tokenizer/tiktoken.h"

TEST(tokenizer_init_sets_defaults) {
  Tokenizer tok;
  tokenizer_init(&tok);
  ASSERT_EQ_SIZE(0, tok.count);
  ASSERT_NULL(tok.entries);
  ASSERT_FALSE(tok.loaded);
  tokenizer_free(&tok);
  PASS();
}

TEST(tokenizer_free_null_safe) {
  Tokenizer tok;
  tokenizer_init(&tok);
  tokenizer_free(&tok);
  tokenizer_free(&tok);
  PASS();
}

TEST(tokenizer_count_tokens_returns_negative_if_unloaded) {
  Tokenizer tok;
  tokenizer_init(&tok);
  int count = tokenizer_count_tokens(&tok, "Hello");
  ASSERT(count <= 0);
  tokenizer_free(&tok);
  PASS();
}

TEST(unicode_is_letter_ascii) {
  ASSERT_TRUE(unicode_is_letter('A'));
  ASSERT_TRUE(unicode_is_letter('Z'));
  ASSERT_TRUE(unicode_is_letter('a'));
  ASSERT_TRUE(unicode_is_letter('z'));
  ASSERT_FALSE(unicode_is_letter('0'));
  ASSERT_FALSE(unicode_is_letter(' '));
  PASS();
}

TEST(unicode_is_number_ascii) {
  ASSERT_TRUE(unicode_is_number('0'));
  ASSERT_TRUE(unicode_is_number('9'));
  ASSERT_FALSE(unicode_is_number('a'));
  ASSERT_FALSE(unicode_is_number(' '));
  PASS();
}

TEST(unicode_is_whitespace_basic) {
  ASSERT_TRUE(unicode_is_whitespace(' '));
  ASSERT_TRUE(unicode_is_whitespace('\t'));
  ASSERT_TRUE(unicode_is_whitespace('\n'));
  ASSERT_TRUE(unicode_is_whitespace('\r'));
  ASSERT_FALSE(unicode_is_whitespace('a'));
  ASSERT_FALSE(unicode_is_whitespace('0'));
  PASS();
}

TEST(utf8_decode_ascii) {
  const uint8_t s[] = {'A'};
  uint32_t cp;
  int len = utf8_decode(s, 1, &cp);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT(65, (int)cp);
  PASS();
}

TEST(utf8_decode_2byte) {
  const uint8_t s[] = {0xC3, 0xA9};
  uint32_t cp;
  int len = utf8_decode(s, 2, &cp);
  ASSERT_EQ_INT(2, len);
  ASSERT_EQ_INT(0xE9, (int)cp);
  PASS();
}

TEST(utf8_decode_3byte) {
  const uint8_t s[] = {0xE4, 0xB8, 0xAD};
  uint32_t cp;
  int len = utf8_decode(s, 3, &cp);
  ASSERT_EQ_INT(3, len);
  ASSERT_EQ_INT(0x4E2D, (int)cp);
  PASS();
}

TEST(utf8_decode_4byte) {
  const uint8_t s[] = {0xF0, 0x9F, 0x98, 0x80};
  uint32_t cp;
  int len = utf8_decode(s, 4, &cp);
  ASSERT_EQ_INT(4, len);
  ASSERT_EQ_INT(0x1F600, (int)cp);
  PASS();
}

TEST(utf8_encode_ascii) {
  uint8_t buf[8];
  int len = utf8_encode(65, buf);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT('A', buf[0]);
  PASS();
}

TEST(utf8_encode_2byte) {
  uint8_t buf[8];
  int len = utf8_encode(0xE9, buf);
  ASSERT_EQ_INT(2, len);
  ASSERT_EQ_INT(0xC3, buf[0]);
  ASSERT_EQ_INT(0xA9, buf[1]);
  PASS();
}

TEST(utf8_encode_3byte) {
  uint8_t buf[8];
  int len = utf8_encode(0x4E2D, buf);
  ASSERT_EQ_INT(3, len);
  ASSERT_EQ_INT(0xE4, buf[0]);
  ASSERT_EQ_INT(0xB8, buf[1]);
  ASSERT_EQ_INT(0xAD, buf[2]);
  PASS();
}

TEST(utf8_encode_4byte) {
  uint8_t buf[8];
  int len = utf8_encode(0x1F600, buf);
  ASSERT_EQ_INT(4, len);
  ASSERT_EQ_INT(0xF0, buf[0]);
  ASSERT_EQ_INT(0x9F, buf[1]);
  ASSERT_EQ_INT(0x98, buf[2]);
  ASSERT_EQ_INT(0x80, buf[3]);
  PASS();
}

TEST(utf8_roundtrip) {
  uint32_t codepoints[] = {0x41, 0xE9, 0x4E2D, 0x1F600};
  for (int i = 0; i < 4; i++) {
    uint8_t buf[8];
    int enc_len = utf8_encode(codepoints[i], buf);
    uint32_t decoded;
    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);
    ASSERT_EQ_INT(enc_len, dec_len);
    ASSERT_EQ_INT((int)codepoints[i], (int)decoded);
  }
  PASS();
}

TEST(pretokenize_cl100k_empty) {
  SpanList spans = {0};
  int count = pretokenize_cl100k("", &spans);
  ASSERT_EQ_INT(0, count);
  free(spans.spans);
  PASS();
}

TEST(pretokenize_cl100k_simple) {
  SpanList spans = {0};
  int count = pretokenize_cl100k("hello", &spans);
  ASSERT(count >= 0);
  ASSERT(spans.count > 0 || count > 0);
  free(spans.spans);
  PASS();
}

TEST(pretokenize_cl100k_with_spaces) {
  SpanList spans = {0};
  int count = pretokenize_cl100k("hello world", &spans);
  ASSERT(count >= 0);
  ASSERT(spans.count >= 2 || count >= 2);
  free(spans.spans);
  PASS();
}

void run_tokenizer_tests(void) {
  TEST_SUITE("Tokenizer Core");
  RUN_TEST(tokenizer_init_sets_defaults);
  RUN_TEST(tokenizer_free_null_safe);
  RUN_TEST(tokenizer_count_tokens_returns_negative_if_unloaded);
  RUN_TEST(unicode_is_letter_ascii);
  RUN_TEST(unicode_is_number_ascii);
  RUN_TEST(unicode_is_whitespace_basic);
  RUN_TEST(utf8_decode_ascii);
  RUN_TEST(utf8_decode_2byte);
  RUN_TEST(utf8_decode_3byte);
  RUN_TEST(utf8_decode_4byte);
  RUN_TEST(utf8_encode_ascii);
  RUN_TEST(utf8_encode_2byte);
  RUN_TEST(utf8_encode_3byte);
  RUN_TEST(utf8_encode_4byte);
  RUN_TEST(utf8_roundtrip);
  RUN_TEST(pretokenize_cl100k_empty);
  RUN_TEST(pretokenize_cl100k_simple);
  RUN_TEST(pretokenize_cl100k_with_spaces);
}

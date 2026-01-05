#include "inference/tokenizer/simd.h"
#include "test_framework.h"

TEST(simd_init_no_crash) {
  simd_init();
  PASS();
}

TEST(simd_hash_bytes_empty) {
  uint64_t hash = simd_hash_bytes((const uint8_t *)"", 0);
  (void)hash;
  PASS();
}

TEST(simd_hash_bytes_short) {
  const char *data = "hello";
  uint64_t hash = simd_hash_bytes((const uint8_t *)data, 5);
  ASSERT(hash != 0);
  PASS();
}

TEST(simd_hash_bytes_deterministic) {
  const char *data = "test string";
  uint64_t hash1 = simd_hash_bytes((const uint8_t *)data, 11);
  uint64_t hash2 = simd_hash_bytes((const uint8_t *)data, 11);
  ASSERT_EQ(hash1, hash2);
  PASS();
}

TEST(simd_hash_bytes_different_data) {
  const char *data1 = "hello";
  const char *data2 = "world";
  uint64_t hash1 = simd_hash_bytes((const uint8_t *)data1, 5);
  uint64_t hash2 = simd_hash_bytes((const uint8_t *)data2, 5);
  ASSERT(hash1 != hash2);
  PASS();
}

TEST(simd_hash_bytes_long) {
  char data[1024];
  for (int i = 0; i < 1024; i++) {
    data[i] = (char)(i % 256);
  }
  uint64_t hash = simd_hash_bytes((const uint8_t *)data, 1024);
  ASSERT(hash != 0);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii) {
  const char *data = "Hello, World!";
  size_t pos = simd_find_non_ascii((const uint8_t *)data, 13);
  ASSERT_EQ_SIZE(13, pos);
  PASS();
}

TEST(simd_find_non_ascii_starts_with_non_ascii) {
  const uint8_t data[] = {0x80, 'H', 'i'};
  size_t pos = simd_find_non_ascii(data, 3);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_mid_non_ascii) {
  const uint8_t data[] = {'H', 'e', 'l', 0xC0, 'o'};
  size_t pos = simd_find_non_ascii(data, 5);
  ASSERT_EQ_SIZE(3, pos);
  PASS();
}

TEST(simd_find_non_ascii_empty) {
  size_t pos = simd_find_non_ascii((const uint8_t *)"", 0);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_is_all_ascii_true) {
  const char *data = "All ASCII characters here!";
  ASSERT_TRUE(simd_is_all_ascii((const uint8_t *)data, strlen(data)));
  PASS();
}

TEST(simd_is_all_ascii_false) {
  const uint8_t data[] = {'H', 'i', 0xE2, 0x80, 0x99};
  ASSERT_FALSE(simd_is_all_ascii(data, 5));
  PASS();
}

TEST(simd_is_all_ascii_empty) {
  ASSERT_TRUE(simd_is_all_ascii((const uint8_t *)"", 0));
  PASS();
}

TEST(simd_count_utf8_chars_ascii) {
  const char *data = "Hello";
  size_t count = simd_count_utf8_chars((const uint8_t *)data, 5);
  ASSERT_EQ_SIZE(5, count);
  PASS();
}

TEST(simd_count_utf8_chars_mixed) {
  const uint8_t data[] = {0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87};
  size_t count = simd_count_utf8_chars(data, 6);
  ASSERT_EQ_SIZE(2, count);
  PASS();
}

TEST(simd_count_utf8_chars_empty) {
  size_t count = simd_count_utf8_chars((const uint8_t *)"", 0);
  ASSERT_EQ_SIZE(0, count);
  PASS();
}

TEST(simd_argmin_u32_basic) {
  uint32_t values[] = {5, 3, 8, 1, 9};
  uint32_t out_min;
  size_t idx = simd_argmin_u32(values, 5, &out_min);
  ASSERT_EQ_SIZE(3, idx);
  ASSERT_EQ_INT(1, (int)out_min);
  PASS();
}

TEST(simd_argmin_u32_first) {
  uint32_t values[] = {1, 3, 5, 7};
  uint32_t out_min;
  size_t idx = simd_argmin_u32(values, 4, &out_min);
  ASSERT_EQ_SIZE(0, idx);
  ASSERT_EQ_INT(1, (int)out_min);
  PASS();
}

TEST(simd_argmin_u32_last) {
  uint32_t values[] = {9, 7, 5, 3, 1};
  uint32_t out_min;
  size_t idx = simd_argmin_u32(values, 5, &out_min);
  ASSERT_EQ_SIZE(4, idx);
  ASSERT_EQ_INT(1, (int)out_min);
  PASS();
}

TEST(simd_argmin_u32_single) {
  uint32_t values[] = {42};
  uint32_t out_min;
  size_t idx = simd_argmin_u32(values, 1, &out_min);
  ASSERT_EQ_SIZE(0, idx);
  ASSERT_EQ_INT(42, (int)out_min);
  PASS();
}

TEST(simd_match_ascii_letters_all_letters) {
  const char *data = "HelloWorld";
  size_t count = simd_match_ascii_letters((const uint8_t *)data, 10);
  ASSERT_EQ_SIZE(10, count);
  PASS();
}

TEST(simd_match_ascii_letters_mixed) {
  const char *data = "Hello123";
  size_t count = simd_match_ascii_letters((const uint8_t *)data, 8);
  ASSERT_EQ_SIZE(5, count);
  PASS();
}

TEST(simd_match_ascii_letters_none) {
  const char *data = "12345";
  size_t count = simd_match_ascii_letters((const uint8_t *)data, 5);
  ASSERT_EQ_SIZE(0, count);
  PASS();
}

TEST(simd_match_ascii_letters_empty) {
  size_t count = simd_match_ascii_letters((const uint8_t *)"", 0);
  ASSERT_EQ_SIZE(0, count);
  PASS();
}

TEST(simd_base64_decode_basic) {
  const char *input = "SGVsbG8=";
  uint8_t output[16];
  size_t len = simd_base64_decode(input, 8, output, 16);
  ASSERT_EQ_SIZE(5, len);
  output[len] = '\0';
  ASSERT_EQ_STR("Hello", (char *)output);
  PASS();
}

TEST(simd_base64_decode_no_padding) {
  const char *input = "SGk";
  uint8_t output[16];
  size_t len = simd_base64_decode(input, 3, output, 16);
  ASSERT_EQ_SIZE(2, len);
  output[len] = '\0';
  ASSERT_EQ_STR("Hi", (char *)output);
  PASS();
}

TEST(simd_base64_decode_one_pad) {
  const char *input = "SGVsbG9X";
  uint8_t output[16];
  size_t len = simd_base64_decode(input, 8, output, 16);
  ASSERT_EQ_SIZE(6, len);
  output[len] = '\0';
  ASSERT_EQ_STR("HelloW", (char *)output);
  PASS();
}

TEST(simd_base64_decode_empty) {
  uint8_t output[16];
  size_t len = simd_base64_decode("", 0, output, 16);
  ASSERT_EQ_SIZE(0, len);
  PASS();
}

// =============================================================================
// SIMD vs Fallback Comparison Tests
// =============================================================================

// Helper macro to skip test if SIMD not available
#define SKIP_IF_NO_SIMD()                                                      \
  do {                                                                         \
    if (!simd_available()) {                                                   \
      PASS();                                                                  \
      return;                                                                  \
    }                                                                          \
  } while (0)

// Test simd_find_non_ascii against fallback
TEST(compare_find_all_ascii) {
  SKIP_IF_NO_SIMD();
  const uint8_t data[64] =
      "Hello World This Is A Test String With More Than 32 Chars!";
  ASSERT_EQ_SIZE(simd_find_non_ascii(data, 64),
                 find_non_ascii_fallback(data, 64));
  PASS();
}

TEST(compare_find_non_ascii_at_0) {
  SKIP_IF_NO_SIMD();
  const uint8_t data[] = {0x80, 'H', 'e', 'l', 'l', 'o'};
  ASSERT_EQ_SIZE(simd_find_non_ascii(data, 6),
                 find_non_ascii_fallback(data, 6));
  PASS();
}

TEST(compare_find_non_ascii_at_31) {
  SKIP_IF_NO_SIMD();
  uint8_t data[64];
  for (int i = 0; i < 64; i++)
    data[i] = 'A';
  data[31] = 0xFF;
  ASSERT_EQ_SIZE(simd_find_non_ascii(data, 64),
                 find_non_ascii_fallback(data, 64));
  PASS();
}

TEST(compare_find_non_ascii_at_32) {
  SKIP_IF_NO_SIMD();
  uint8_t data[64];
  for (int i = 0; i < 64; i++)
    data[i] = 'A';
  data[32] = 0xFF;
  ASSERT_EQ_SIZE(simd_find_non_ascii(data, 64),
                 find_non_ascii_fallback(data, 64));
  PASS();
}

TEST(compare_find_non_ascii_at_33) {
  SKIP_IF_NO_SIMD();
  uint8_t data[64];
  for (int i = 0; i < 64; i++)
    data[i] = 'A';
  data[33] = 0xFF;
  ASSERT_EQ_SIZE(simd_find_non_ascii(data, 64),
                 find_non_ascii_fallback(data, 64));
  PASS();
}

// Test simd_is_all_ascii against fallback
TEST(compare_is_all_ascii_true_small) {
  SKIP_IF_NO_SIMD();
  const uint8_t data[] = "Hello";
  ASSERT_EQ(simd_is_all_ascii(data, 5), is_all_ascii_fallback(data, 5));
  PASS();
}

TEST(compare_is_all_ascii_true_32) {
  SKIP_IF_NO_SIMD();
  uint8_t data[32];
  for (int i = 0; i < 32; i++)
    data[i] = 'A';
  ASSERT_EQ(simd_is_all_ascii(data, 32), is_all_ascii_fallback(data, 32));
  PASS();
}

TEST(compare_is_all_ascii_false_at_31) {
  SKIP_IF_NO_SIMD();
  uint8_t data[32];
  for (int i = 0; i < 32; i++)
    data[i] = 'A';
  data[31] = 0x80;
  ASSERT_EQ(simd_is_all_ascii(data, 32), is_all_ascii_fallback(data, 32));
  PASS();
}

TEST(compare_is_all_ascii_false_at_0) {
  SKIP_IF_NO_SIMD();
  uint8_t data[32];
  for (int i = 0; i < 32; i++)
    data[i] = 'A';
  data[0] = 0x80;
  ASSERT_EQ(simd_is_all_ascii(data, 32), is_all_ascii_fallback(data, 32));
  PASS();
}

// Test simd_count_utf8_chars against fallback
TEST(compare_count_ascii) {
  SKIP_IF_NO_SIMD();
  const uint8_t data[] = "Hello World!";
  ASSERT_EQ_SIZE(simd_count_utf8_chars(data, 12),
                 count_utf8_chars_fallback(data, 12));
  PASS();
}

TEST(compare_count_utf8_2byte) {
  SKIP_IF_NO_SIMD();
  // "café" = c a f 0xC3 0xA9
  const uint8_t data[] = {'c', 'a', 'f', 0xC3, 0xA9};
  ASSERT_EQ_SIZE(simd_count_utf8_chars(data, 5),
                 count_utf8_chars_fallback(data, 5));
  PASS();
}

TEST(compare_count_utf8_3byte) {
  SKIP_IF_NO_SIMD();
  // Chinese "中文" = 0xE4 0xB8 0xAD 0xE6 0x96 0x87
  const uint8_t data[] = {0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87};
  ASSERT_EQ_SIZE(simd_count_utf8_chars(data, 6),
                 count_utf8_chars_fallback(data, 6));
  PASS();
}

TEST(compare_count_utf8_4byte) {
  SKIP_IF_NO_SIMD();
  // Emoji "😀" = 0xF0 0x9F 0x98 0x80
  const uint8_t data[] = {0xF0, 0x9F, 0x98, 0x80};
  ASSERT_EQ_SIZE(simd_count_utf8_chars(data, 4),
                 count_utf8_chars_fallback(data, 4));
  PASS();
}

TEST(compare_count_utf8_mixed_32) {
  SKIP_IF_NO_SIMD();
  // Mix ASCII and multibyte across 32-byte boundary
  uint8_t data[64] = "Hello ";
  // Add Chinese characters
  size_t pos = 6;
  data[pos++] = 0xE4;
  data[pos++] = 0xB8;
  data[pos++] = 0xAD; // 中
  data[pos++] = 0xE6;
  data[pos++] = 0x96;
  data[pos++] = 0x87; // 文
  // Fill rest with ASCII
  for (; pos < 64; pos++)
    data[pos] = 'A';
  ASSERT_EQ_SIZE(simd_count_utf8_chars(data, 64),
                 count_utf8_chars_fallback(data, 64));
  PASS();
}

// Test simd_argmin_u32 against fallback
TEST(compare_argmin_single) {
  SKIP_IF_NO_SIMD();
  uint32_t data[] = {42};
  uint32_t min_simd, min_fb;
  size_t idx_simd = simd_argmin_u32(data, 1, &min_simd);
  size_t idx_fb = argmin_u32_fallback(data, 1, &min_fb);
  ASSERT_EQ_SIZE(idx_fb, idx_simd);
  ASSERT_EQ_INT(min_fb, min_simd);
  PASS();
}

TEST(compare_argmin_at_0) {
  SKIP_IF_NO_SIMD();
  uint32_t data[] = {1, 5, 9, 13, 17};
  uint32_t min_simd, min_fb;
  size_t idx_simd = simd_argmin_u32(data, 5, &min_simd);
  size_t idx_fb = argmin_u32_fallback(data, 5, &min_fb);
  ASSERT_EQ_SIZE(idx_fb, idx_simd);
  ASSERT_EQ_INT(min_fb, min_simd);
  PASS();
}

TEST(compare_argmin_at_7) {
  SKIP_IF_NO_SIMD();
  uint32_t data[8] = {10, 9, 8, 7, 6, 5, 4, 3};
  uint32_t min_simd, min_fb;
  size_t idx_simd = simd_argmin_u32(data, 8, &min_simd);
  size_t idx_fb = argmin_u32_fallback(data, 8, &min_fb);
  ASSERT_EQ_SIZE(idx_fb, idx_simd);
  ASSERT_EQ_INT(min_fb, min_simd);
  PASS();
}

TEST(compare_argmin_at_8) {
  SKIP_IF_NO_SIMD();
  uint32_t data[16] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 11, 12, 13, 14, 15, 16, 17};
  uint32_t min_simd, min_fb;
  size_t idx_simd = simd_argmin_u32(data, 16, &min_simd);
  size_t idx_fb = argmin_u32_fallback(data, 16, &min_fb);
  ASSERT_EQ_SIZE(idx_fb, idx_simd);
  ASSERT_EQ_INT(min_fb, min_simd);
  PASS();
}

TEST(compare_argmin_all_equal) {
  SKIP_IF_NO_SIMD();
  uint32_t data[16];
  for (int i = 0; i < 16; i++)
    data[i] = 42;
  uint32_t min_simd, min_fb;
  size_t idx_simd = simd_argmin_u32(data, 16, &min_simd);
  size_t idx_fb = argmin_u32_fallback(data, 16, &min_fb);
  ASSERT_EQ_SIZE(idx_fb, idx_simd);
  ASSERT_EQ_INT(min_fb, min_simd);
  PASS();
}

// Test simd_match_ascii_letters against fallback
TEST(compare_match_all_letters) {
  SKIP_IF_NO_SIMD();
  const uint8_t data[] = "HelloWorldThisIsATest";
  ASSERT_EQ_SIZE(simd_match_ascii_letters(data, 21),
                 match_ascii_letters_fallback(data, 21));
  PASS();
}

TEST(compare_match_no_letters) {
  SKIP_IF_NO_SIMD();
  const uint8_t data[] = "123456789";
  ASSERT_EQ_SIZE(simd_match_ascii_letters(data, 9),
                 match_ascii_letters_fallback(data, 9));
  PASS();
}

TEST(compare_match_stops_at_31) {
  SKIP_IF_NO_SIMD();
  uint8_t data[64];
  for (int i = 0; i < 31; i++)
    data[i] = 'A';
  data[31] = '1';
  for (int i = 32; i < 64; i++)
    data[i] = 'B';
  ASSERT_EQ_SIZE(simd_match_ascii_letters(data, 64),
                 match_ascii_letters_fallback(data, 64));
  PASS();
}

TEST(compare_match_stops_at_32) {
  SKIP_IF_NO_SIMD();
  uint8_t data[64];
  for (int i = 0; i < 32; i++)
    data[i] = 'A';
  data[32] = '1';
  for (int i = 33; i < 64; i++)
    data[i] = 'B';
  ASSERT_EQ_SIZE(simd_match_ascii_letters(data, 64),
                 match_ascii_letters_fallback(data, 64));
  PASS();
}

TEST(compare_match_stops_at_33) {
  SKIP_IF_NO_SIMD();
  uint8_t data[64];
  for (int i = 0; i < 33; i++)
    data[i] = 'A';
  data[33] = '1';
  for (int i = 34; i < 64; i++)
    data[i] = 'B';
  ASSERT_EQ_SIZE(simd_match_ascii_letters(data, 64),
                 match_ascii_letters_fallback(data, 64));
  PASS();
}

TEST(compare_match_mixed_case) {
  SKIP_IF_NO_SIMD();
  const uint8_t data[] = "HeLLo WoRLd";
  ASSERT_EQ_SIZE(simd_match_ascii_letters(data, 11),
                 match_ascii_letters_fallback(data, 11));
  PASS();
}

// Test base64_decode against fallback
TEST(compare_base64_empty) {
  SKIP_IF_NO_SIMD();
  uint8_t out1[64], out2[64];
  size_t len1 = simd_base64_decode("", 0, out1, 64);
  size_t len2 = base64_decode_fallback("", 0, out2, 64);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

TEST(compare_base64_small) {
  SKIP_IF_NO_SIMD();
  const char *input = "SGVsbG8="; // "Hello"
  uint8_t out1[64], out2[64];
  size_t len1 = simd_base64_decode(input, strlen(input), out1, 64);
  size_t len2 = base64_decode_fallback(input, strlen(input), out2, 64);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

TEST(compare_base64_28_bytes) {
  SKIP_IF_NO_SIMD();
  // 28 input chars (7 complete blocks = 21 output bytes)
  const char *input = "VGhpcyBpcyBhIHRlc3Qgc3Ry";
  uint8_t out1[64], out2[64];
  size_t len1 = simd_base64_decode(input, strlen(input), out1, 64);
  size_t len2 = base64_decode_fallback(input, strlen(input), out2, 64);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

TEST(compare_base64_32_bytes) {
  SKIP_IF_NO_SIMD();
  // Exactly 32 input chars (8 complete blocks = 24 output bytes, AVX2 boundary)
  const char *input = "VGhpcyBpcyBhIHRlc3Qgc3RyaW5n";
  uint8_t out1[64], out2[64];
  size_t len1 = simd_base64_decode(input, strlen(input), out1, 64);
  size_t len2 = base64_decode_fallback(input, strlen(input), out2, 64);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

TEST(compare_base64_36_bytes) {
  SKIP_IF_NO_SIMD();
  // 36 input chars (9 complete blocks = 27 output bytes, just over AVX2
  // boundary)
  const char *input = "VGhpcyBpcyBhIHRlc3Qgc3RyaW5nISE=";
  uint8_t out1[64], out2[64];
  size_t len1 = simd_base64_decode(input, strlen(input), out1, 64);
  size_t len2 = base64_decode_fallback(input, strlen(input), out2, 64);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

TEST(compare_base64_64_bytes) {
  SKIP_IF_NO_SIMD();
  // 64 input chars (2x AVX2 boundary)
  const char *input =
      "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZyBhbmQ=";
  uint8_t out1[128], out2[128];
  size_t len1 = simd_base64_decode(input, 64, out1, 128);
  size_t len2 = base64_decode_fallback(input, 64, out2, 128);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

TEST(compare_base64_with_padding) {
  SKIP_IF_NO_SIMD();
  const char *input = "YWJjZGU="; // "abcde"
  uint8_t out1[64], out2[64];
  size_t len1 = simd_base64_decode(input, strlen(input), out1, 64);
  size_t len2 = base64_decode_fallback(input, strlen(input), out2, 64);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

TEST(compare_base64_no_padding) {
  SKIP_IF_NO_SIMD();
  const char *input = "YWJjZGU"; // "abcde" without padding
  uint8_t out1[64], out2[64];
  size_t len1 = simd_base64_decode(input, strlen(input), out1, 64);
  size_t len2 = base64_decode_fallback(input, strlen(input), out2, 64);
  ASSERT_EQ_SIZE(len1, len2);
  ASSERT_TRUE(memcmp(out1, out2, len1) == 0);
  PASS();
}

void run_simd_tests(void) {
  TEST_SUITE("SIMD Utilities");
  RUN_TEST(simd_init_no_crash);
  RUN_TEST(simd_hash_bytes_empty);
  RUN_TEST(simd_hash_bytes_short);
  RUN_TEST(simd_hash_bytes_deterministic);
  RUN_TEST(simd_hash_bytes_different_data);
  RUN_TEST(simd_hash_bytes_long);
  RUN_TEST(simd_find_non_ascii_all_ascii);
  RUN_TEST(simd_find_non_ascii_starts_with_non_ascii);
  RUN_TEST(simd_find_non_ascii_mid_non_ascii);
  RUN_TEST(simd_find_non_ascii_empty);
  RUN_TEST(simd_is_all_ascii_true);
  RUN_TEST(simd_is_all_ascii_false);
  RUN_TEST(simd_is_all_ascii_empty);
  RUN_TEST(simd_count_utf8_chars_ascii);
  RUN_TEST(simd_count_utf8_chars_mixed);
  RUN_TEST(simd_count_utf8_chars_empty);
  RUN_TEST(simd_argmin_u32_basic);
  RUN_TEST(simd_argmin_u32_first);
  RUN_TEST(simd_argmin_u32_last);
  RUN_TEST(simd_argmin_u32_single);
  RUN_TEST(simd_match_ascii_letters_all_letters);
  RUN_TEST(simd_match_ascii_letters_mixed);
  RUN_TEST(simd_match_ascii_letters_none);
  RUN_TEST(simd_match_ascii_letters_empty);
  RUN_TEST(simd_base64_decode_basic);
  RUN_TEST(simd_base64_decode_no_padding);
  RUN_TEST(simd_base64_decode_one_pad);
  RUN_TEST(simd_base64_decode_empty);
  RUN_TEST(compare_find_all_ascii);
  RUN_TEST(compare_find_non_ascii_at_0);
  RUN_TEST(compare_find_non_ascii_at_31);
  RUN_TEST(compare_find_non_ascii_at_32);
  RUN_TEST(compare_find_non_ascii_at_33);
  RUN_TEST(compare_is_all_ascii_true_small);
  RUN_TEST(compare_is_all_ascii_true_32);
  RUN_TEST(compare_is_all_ascii_false_at_31);
  RUN_TEST(compare_is_all_ascii_false_at_0);
  RUN_TEST(compare_count_ascii);
  RUN_TEST(compare_count_utf8_2byte);
  RUN_TEST(compare_count_utf8_3byte);
  RUN_TEST(compare_count_utf8_4byte);
  RUN_TEST(compare_count_utf8_mixed_32);
  RUN_TEST(compare_argmin_single);
  RUN_TEST(compare_argmin_at_0);
  RUN_TEST(compare_argmin_at_7);
  RUN_TEST(compare_argmin_at_8);
  RUN_TEST(compare_argmin_all_equal);
  RUN_TEST(compare_match_all_letters);
  RUN_TEST(compare_match_no_letters);
  RUN_TEST(compare_match_stops_at_31);
  RUN_TEST(compare_match_stops_at_32);
  RUN_TEST(compare_match_stops_at_33);
  RUN_TEST(compare_match_mixed_case);
  RUN_TEST(compare_base64_empty);
  RUN_TEST(compare_base64_small);
  RUN_TEST(compare_base64_28_bytes);
  RUN_TEST(compare_base64_32_bytes);
  RUN_TEST(compare_base64_36_bytes);
  RUN_TEST(compare_base64_64_bytes);
  RUN_TEST(compare_base64_with_padding);
  RUN_TEST(compare_base64_no_padding);
}

#include "test_framework.h"
#include "tokenizer/simd.h"

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
}

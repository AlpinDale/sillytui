#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"tokenizer/simd.h\"\n");
  printf("#include <stdlib.h>\n");
  printf("#include <string.h>\n\n");

  for (int len = 0; len <= 128; len++) {
    printf("TEST(simd_hash_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len > 0 ? len : 1);
    printf("  for (int i = 0; i < %d; i++) buf[i] = (uint8_t)(i * 7);\n", len);
    printf("  uint64_t h1 = simd_hash_bytes(buf, %d);\n", len);
    printf("  uint64_t h2 = simd_hash_bytes(buf, %d);\n", len);
    printf("  ASSERT(h1 == h2);\n");
    if (len > 0) {
      printf("  buf[0] ^= 1;\n");
      printf("  uint64_t h3 = simd_hash_bytes(buf, %d);\n", len);
      printf("  if (%d > 0) ASSERT(h1 != h3);\n", len);
    }
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int len = 1; len <= 64; len++) {
    printf("TEST(simd_find_non_ascii_all_ascii_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len);
    printf("  for (int i = 0; i < %d; i++) buf[i] = 'a' + (i %% 26);\n", len);
    printf("  size_t pos = simd_find_non_ascii(buf, %d);\n", len);
    printf("  ASSERT_EQ_SIZE(%d, pos);\n", len);
    printf("  PASS();\n");
    printf("}\n\n");

    printf("TEST(simd_find_non_ascii_first_non_ascii_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len);
    printf("  for (int i = 0; i < %d; i++) buf[i] = 'a' + (i %% 26);\n", len);
    printf("  buf[0] = 0x80;\n");
    printf("  size_t pos = simd_find_non_ascii(buf, %d);\n", len);
    printf("  ASSERT_EQ_SIZE(0, pos);\n");
    printf("  PASS();\n");
    printf("}\n\n");

    printf("TEST(simd_find_non_ascii_last_non_ascii_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len);
    printf("  for (int i = 0; i < %d; i++) buf[i] = 'a' + (i %% 26);\n", len);
    printf("  buf[%d - 1] = 0x80;\n", len);
    printf("  size_t pos = simd_find_non_ascii(buf, %d);\n", len);
    printf("  ASSERT_EQ_SIZE(%d, pos);\n", len - 1);
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int len = 0; len <= 64; len++) {
    printf("TEST(simd_count_utf8_chars_ascii_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len > 0 ? len : 1);
    printf("  for (int i = 0; i < %d; i++) buf[i] = 'a' + (i %% 26);\n", len);
    printf("  size_t count = simd_count_utf8_chars(buf, %d);\n", len);
    printf("  ASSERT_EQ_SIZE(%d, count);\n", len);
    printf("  PASS();\n");
    printf("}\n\n");
  }

  for (int len = 0; len <= 64; len++) {
    printf("TEST(simd_match_ascii_letters_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len > 0 ? len : 1);
    printf("  for (int i = 0; i < %d; i++) buf[i] = 'A' + (i %% 26);\n", len);
    printf("  size_t matched = simd_match_ascii_letters(buf, %d);\n", len);
    printf("  ASSERT_EQ_SIZE(%d, matched);\n", len);
    printf("  PASS();\n");
    printf("}\n\n");

    printf("TEST(simd_match_ascii_letters_none_len_%d) {\n", len);
    printf("  uint8_t buf[%d];\n", len > 0 ? len : 1);
    printf("  for (int i = 0; i < %d; i++) buf[i] = '0' + (i %% 10);\n", len);
    printf("  size_t matched = simd_match_ascii_letters(buf, %d);\n", len);
    printf("  ASSERT_EQ_SIZE(0, matched);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("TEST(simd_is_all_ascii_exhaustive) {\n");
  printf("  uint8_t buf[256];\n");
  printf("  for (int i = 0; i < 128; i++) {\n");
  printf("    buf[0] = (uint8_t)i;\n");
  printf("    ASSERT_TRUE(simd_is_all_ascii(buf, 1));\n");
  printf("  }\n");
  printf("  for (int i = 128; i < 256; i++) {\n");
  printf("    buf[0] = (uint8_t)i;\n");
  printf("    ASSERT_FALSE(simd_is_all_ascii(buf, 1));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(simd_argmin_u32_exhaustive) {\n");
  printf("  uint32_t arr[16];\n");
  printf("  uint32_t out_min;\n");
  printf("  for (int min_pos = 0; min_pos < 16; min_pos++) {\n");
  printf("    for (int i = 0; i < 16; i++) arr[i] = 100;\n");
  printf("    arr[min_pos] = 1;\n");
  printf("    size_t result = simd_argmin_u32(arr, 16, &out_min);\n");
  printf("    ASSERT_EQ_SIZE((size_t)min_pos, result);\n");
  printf("    ASSERT_EQ_INT(1, (int)out_min);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_simd_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"SIMD Generated Tests\");\n");
  for (int len = 0; len <= 128; len++) {
    printf("  RUN_TEST(simd_hash_len_%d);\n", len);
  }
  for (int len = 1; len <= 64; len++) {
    printf("  RUN_TEST(simd_find_non_ascii_all_ascii_len_%d);\n", len);
    printf("  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_%d);\n", len);
    printf("  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_%d);\n", len);
  }
  for (int len = 0; len <= 64; len++) {
    printf("  RUN_TEST(simd_count_utf8_chars_ascii_len_%d);\n", len);
  }
  for (int len = 0; len <= 64; len++) {
    printf("  RUN_TEST(simd_match_ascii_letters_len_%d);\n", len);
    printf("  RUN_TEST(simd_match_ascii_letters_none_len_%d);\n", len);
  }
  printf("  RUN_TEST(simd_is_all_ascii_exhaustive);\n");
  printf("  RUN_TEST(simd_argmin_u32_exhaustive);\n");
  printf("}\n");

  return 0;
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"inference/tokenizer/tiktoken.h\"\n\n");

  printf("TEST(utf8_decode_empty_buffer) {\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode((const uint8_t *)\"\", 0, &cp);\n");
  printf("  ASSERT_EQ_INT(0, len);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_all_ascii) {\n");
  printf("  for (uint32_t c = 0; c < 128; c++) {\n");
  printf("    uint8_t buf[1] = {(uint8_t)c};\n");
  printf("    uint32_t cp;\n");
  printf("    int len = utf8_decode(buf, 1, &cp);\n");
  printf("    ASSERT_EQ_INT(1, len);\n");
  printf("    ASSERT_EQ_INT((int)c, (int)cp);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_2byte_boundary_low) {\n");
  printf("  const uint8_t s[] = {0xC2, 0x80};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 2, &cp);\n");
  printf("  ASSERT_EQ_INT(2, len);\n");
  printf("  ASSERT_EQ_INT(0x80, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_2byte_boundary_high) {\n");
  printf("  const uint8_t s[] = {0xDF, 0xBF};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 2, &cp);\n");
  printf("  ASSERT_EQ_INT(2, len);\n");
  printf("  ASSERT_EQ_INT(0x7FF, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_3byte_boundary_low) {\n");
  printf("  const uint8_t s[] = {0xE0, 0xA0, 0x80};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 3, &cp);\n");
  printf("  ASSERT_EQ_INT(3, len);\n");
  printf("  ASSERT_EQ_INT(0x800, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_3byte_boundary_high) {\n");
  printf("  const uint8_t s[] = {0xEF, 0xBF, 0xBF};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 3, &cp);\n");
  printf("  ASSERT_EQ_INT(3, len);\n");
  printf("  ASSERT_EQ_INT(0xFFFF, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_4byte_boundary_low) {\n");
  printf("  const uint8_t s[] = {0xF0, 0x90, 0x80, 0x80};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 4, &cp);\n");
  printf("  ASSERT_EQ_INT(4, len);\n");
  printf("  ASSERT_EQ_INT(0x10000, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_4byte_boundary_high) {\n");
  printf("  const uint8_t s[] = {0xF4, 0x8F, 0xBF, 0xBF};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 4, &cp);\n");
  printf("  ASSERT_EQ_INT(4, len);\n");
  printf("  ASSERT_EQ_INT(0x10FFFF, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_truncated_2byte) {\n");
  printf("  const uint8_t s[] = {0xC3};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 1, &cp);\n");
  printf("  ASSERT_EQ_INT(1, len);\n");
  printf("  ASSERT_EQ_INT(0xFFFD, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_truncated_3byte_1) {\n");
  printf("  const uint8_t s[] = {0xE4};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 1, &cp);\n");
  printf("  ASSERT_EQ_INT(1, len);\n");
  printf("  ASSERT_EQ_INT(0xFFFD, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_truncated_3byte_2) {\n");
  printf("  const uint8_t s[] = {0xE4, 0xB8};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 2, &cp);\n");
  printf("  ASSERT_EQ_INT(1, len);\n");
  printf("  ASSERT_EQ_INT(0xFFFD, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_truncated_4byte_1) {\n");
  printf("  const uint8_t s[] = {0xF0};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 1, &cp);\n");
  printf("  ASSERT_EQ_INT(1, len);\n");
  printf("  ASSERT_EQ_INT(0xFFFD, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_truncated_4byte_2) {\n");
  printf("  const uint8_t s[] = {0xF0, 0x9F};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 2, &cp);\n");
  printf("  ASSERT_EQ_INT(1, len);\n");
  printf("  ASSERT_EQ_INT(0xFFFD, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_decode_truncated_4byte_3) {\n");
  printf("  const uint8_t s[] = {0xF0, 0x9F, 0x98};\n");
  printf("  uint32_t cp;\n");
  printf("  int len = utf8_decode(s, 3, &cp);\n");
  printf("  ASSERT_EQ_INT(1, len);\n");
  printf("  ASSERT_EQ_INT(0xFFFD, (int)cp);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_encode_all_ascii) {\n");
  printf("  for (uint32_t c = 0; c < 128; c++) {\n");
  printf("    uint8_t buf[4];\n");
  printf("    int len = utf8_encode(c, buf);\n");
  printf("    ASSERT_EQ_INT(1, len);\n");
  printf("    ASSERT_EQ_INT((int)c, (int)buf[0]);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_encode_2byte_boundary_low) {\n");
  printf("  uint8_t buf[4];\n");
  printf("  int len = utf8_encode(0x80, buf);\n");
  printf("  ASSERT_EQ_INT(2, len);\n");
  printf("  ASSERT_EQ_INT(0xC2, buf[0]);\n");
  printf("  ASSERT_EQ_INT(0x80, buf[1]);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_encode_2byte_boundary_high) {\n");
  printf("  uint8_t buf[4];\n");
  printf("  int len = utf8_encode(0x7FF, buf);\n");
  printf("  ASSERT_EQ_INT(2, len);\n");
  printf("  ASSERT_EQ_INT(0xDF, buf[0]);\n");
  printf("  ASSERT_EQ_INT(0xBF, buf[1]);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_encode_3byte_boundary_low) {\n");
  printf("  uint8_t buf[4];\n");
  printf("  int len = utf8_encode(0x800, buf);\n");
  printf("  ASSERT_EQ_INT(3, len);\n");
  printf("  ASSERT_EQ_INT(0xE0, buf[0]);\n");
  printf("  ASSERT_EQ_INT(0xA0, buf[1]);\n");
  printf("  ASSERT_EQ_INT(0x80, buf[2]);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_encode_3byte_boundary_high) {\n");
  printf("  uint8_t buf[4];\n");
  printf("  int len = utf8_encode(0xFFFF, buf);\n");
  printf("  ASSERT_EQ_INT(3, len);\n");
  printf("  ASSERT_EQ_INT(0xEF, buf[0]);\n");
  printf("  ASSERT_EQ_INT(0xBF, buf[1]);\n");
  printf("  ASSERT_EQ_INT(0xBF, buf[2]);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_encode_4byte_boundary_low) {\n");
  printf("  uint8_t buf[4];\n");
  printf("  int len = utf8_encode(0x10000, buf);\n");
  printf("  ASSERT_EQ_INT(4, len);\n");
  printf("  ASSERT_EQ_INT(0xF0, buf[0]);\n");
  printf("  ASSERT_EQ_INT(0x90, buf[1]);\n");
  printf("  ASSERT_EQ_INT(0x80, buf[2]);\n");
  printf("  ASSERT_EQ_INT(0x80, buf[3]);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_encode_4byte_boundary_high) {\n");
  printf("  uint8_t buf[4];\n");
  printf("  int len = utf8_encode(0x10FFFF, buf);\n");
  printf("  ASSERT_EQ_INT(4, len);\n");
  printf("  ASSERT_EQ_INT(0xF4, buf[0]);\n");
  printf("  ASSERT_EQ_INT(0x8F, buf[1]);\n");
  printf("  ASSERT_EQ_INT(0xBF, buf[2]);\n");
  printf("  ASSERT_EQ_INT(0xBF, buf[3]);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_roundtrip_exhaustive_1byte) {\n");
  printf("  for (uint32_t cp = 0; cp < 0x80; cp++) {\n");
  printf("    uint8_t buf[4];\n");
  printf("    int enc_len = utf8_encode(cp, buf);\n");
  printf("    uint32_t decoded;\n");
  printf("    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);\n");
  printf("    ASSERT_EQ_INT(enc_len, dec_len);\n");
  printf("    ASSERT_EQ_INT((int)cp, (int)decoded);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_roundtrip_exhaustive_2byte) {\n");
  printf("  for (uint32_t cp = 0x80; cp < 0x800; cp++) {\n");
  printf("    uint8_t buf[4];\n");
  printf("    int enc_len = utf8_encode(cp, buf);\n");
  printf("    uint32_t decoded;\n");
  printf("    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);\n");
  printf("    ASSERT_EQ_INT(enc_len, dec_len);\n");
  printf("    ASSERT_EQ_INT((int)cp, (int)decoded);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_roundtrip_sample_3byte) {\n");
  printf(
      "  uint32_t samples[] = {0x800, 0x1000, 0x2000, 0x3000, 0x4E00, 0x4E2D, "
      "0x65E5, 0x672C, 0x8A9E, 0xFFFF};\n");
  printf("  for (size_t i = 0; i < 10; i++) {\n");
  printf("    uint8_t buf[4];\n");
  printf("    int enc_len = utf8_encode(samples[i], buf);\n");
  printf("    uint32_t decoded;\n");
  printf("    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);\n");
  printf("    ASSERT_EQ_INT(enc_len, dec_len);\n");
  printf("    ASSERT_EQ_INT((int)samples[i], (int)decoded);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(utf8_roundtrip_sample_4byte) {\n");
  printf("  uint32_t samples[] = {0x10000, 0x1F600, 0x1F64F, 0x1F680, "
         "0x10FFFF};\n");
  printf("  for (size_t i = 0; i < 5; i++) {\n");
  printf("    uint8_t buf[4];\n");
  printf("    int enc_len = utf8_encode(samples[i], buf);\n");
  printf("    uint32_t decoded;\n");
  printf("    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);\n");
  printf("    ASSERT_EQ_INT(enc_len, dec_len);\n");
  printf("    ASSERT_EQ_INT((int)samples[i], (int)decoded);\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_utf8_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"UTF-8 Generated Tests\");\n");
  printf("  RUN_TEST(utf8_decode_empty_buffer);\n");
  printf("  RUN_TEST(utf8_decode_all_ascii);\n");
  printf("  RUN_TEST(utf8_decode_2byte_boundary_low);\n");
  printf("  RUN_TEST(utf8_decode_2byte_boundary_high);\n");
  printf("  RUN_TEST(utf8_decode_3byte_boundary_low);\n");
  printf("  RUN_TEST(utf8_decode_3byte_boundary_high);\n");
  printf("  RUN_TEST(utf8_decode_4byte_boundary_low);\n");
  printf("  RUN_TEST(utf8_decode_4byte_boundary_high);\n");
  printf("  RUN_TEST(utf8_decode_truncated_2byte);\n");
  printf("  RUN_TEST(utf8_decode_truncated_3byte_1);\n");
  printf("  RUN_TEST(utf8_decode_truncated_3byte_2);\n");
  printf("  RUN_TEST(utf8_decode_truncated_4byte_1);\n");
  printf("  RUN_TEST(utf8_decode_truncated_4byte_2);\n");
  printf("  RUN_TEST(utf8_decode_truncated_4byte_3);\n");
  printf("  RUN_TEST(utf8_encode_all_ascii);\n");
  printf("  RUN_TEST(utf8_encode_2byte_boundary_low);\n");
  printf("  RUN_TEST(utf8_encode_2byte_boundary_high);\n");
  printf("  RUN_TEST(utf8_encode_3byte_boundary_low);\n");
  printf("  RUN_TEST(utf8_encode_3byte_boundary_high);\n");
  printf("  RUN_TEST(utf8_encode_4byte_boundary_low);\n");
  printf("  RUN_TEST(utf8_encode_4byte_boundary_high);\n");
  printf("  RUN_TEST(utf8_roundtrip_exhaustive_1byte);\n");
  printf("  RUN_TEST(utf8_roundtrip_exhaustive_2byte);\n");
  printf("  RUN_TEST(utf8_roundtrip_sample_3byte);\n");
  printf("  RUN_TEST(utf8_roundtrip_sample_4byte);\n");
  printf("}\n");

  return 0;
}

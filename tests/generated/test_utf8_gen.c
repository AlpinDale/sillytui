#include "inference/tokenizer/tiktoken.h"
#include "test_framework.h"

TEST(utf8_decode_empty_buffer) {
  uint32_t cp;
  int len = utf8_decode((const uint8_t *)"", 0, &cp);
  ASSERT_EQ_INT(0, len);
  PASS();
}

TEST(utf8_decode_all_ascii) {
  for (uint32_t c = 0; c < 128; c++) {
    uint8_t buf[1] = {(uint8_t)c};
    uint32_t cp;
    int len = utf8_decode(buf, 1, &cp);
    ASSERT_EQ_INT(1, len);
    ASSERT_EQ_INT((int)c, (int)cp);
  }
  PASS();
}

TEST(utf8_decode_2byte_boundary_low) {
  const uint8_t s[] = {0xC2, 0x80};
  uint32_t cp;
  int len = utf8_decode(s, 2, &cp);
  ASSERT_EQ_INT(2, len);
  ASSERT_EQ_INT(0x80, (int)cp);
  PASS();
}

TEST(utf8_decode_2byte_boundary_high) {
  const uint8_t s[] = {0xDF, 0xBF};
  uint32_t cp;
  int len = utf8_decode(s, 2, &cp);
  ASSERT_EQ_INT(2, len);
  ASSERT_EQ_INT(0x7FF, (int)cp);
  PASS();
}

TEST(utf8_decode_3byte_boundary_low) {
  const uint8_t s[] = {0xE0, 0xA0, 0x80};
  uint32_t cp;
  int len = utf8_decode(s, 3, &cp);
  ASSERT_EQ_INT(3, len);
  ASSERT_EQ_INT(0x800, (int)cp);
  PASS();
}

TEST(utf8_decode_3byte_boundary_high) {
  const uint8_t s[] = {0xEF, 0xBF, 0xBF};
  uint32_t cp;
  int len = utf8_decode(s, 3, &cp);
  ASSERT_EQ_INT(3, len);
  ASSERT_EQ_INT(0xFFFF, (int)cp);
  PASS();
}

TEST(utf8_decode_4byte_boundary_low) {
  const uint8_t s[] = {0xF0, 0x90, 0x80, 0x80};
  uint32_t cp;
  int len = utf8_decode(s, 4, &cp);
  ASSERT_EQ_INT(4, len);
  ASSERT_EQ_INT(0x10000, (int)cp);
  PASS();
}

TEST(utf8_decode_4byte_boundary_high) {
  const uint8_t s[] = {0xF4, 0x8F, 0xBF, 0xBF};
  uint32_t cp;
  int len = utf8_decode(s, 4, &cp);
  ASSERT_EQ_INT(4, len);
  ASSERT_EQ_INT(0x10FFFF, (int)cp);
  PASS();
}

TEST(utf8_decode_truncated_2byte) {
  const uint8_t s[] = {0xC3};
  uint32_t cp;
  int len = utf8_decode(s, 1, &cp);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT(0xFFFD, (int)cp);
  PASS();
}

TEST(utf8_decode_truncated_3byte_1) {
  const uint8_t s[] = {0xE4};
  uint32_t cp;
  int len = utf8_decode(s, 1, &cp);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT(0xFFFD, (int)cp);
  PASS();
}

TEST(utf8_decode_truncated_3byte_2) {
  const uint8_t s[] = {0xE4, 0xB8};
  uint32_t cp;
  int len = utf8_decode(s, 2, &cp);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT(0xFFFD, (int)cp);
  PASS();
}

TEST(utf8_decode_truncated_4byte_1) {
  const uint8_t s[] = {0xF0};
  uint32_t cp;
  int len = utf8_decode(s, 1, &cp);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT(0xFFFD, (int)cp);
  PASS();
}

TEST(utf8_decode_truncated_4byte_2) {
  const uint8_t s[] = {0xF0, 0x9F};
  uint32_t cp;
  int len = utf8_decode(s, 2, &cp);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT(0xFFFD, (int)cp);
  PASS();
}

TEST(utf8_decode_truncated_4byte_3) {
  const uint8_t s[] = {0xF0, 0x9F, 0x98};
  uint32_t cp;
  int len = utf8_decode(s, 3, &cp);
  ASSERT_EQ_INT(1, len);
  ASSERT_EQ_INT(0xFFFD, (int)cp);
  PASS();
}

TEST(utf8_encode_all_ascii) {
  for (uint32_t c = 0; c < 128; c++) {
    uint8_t buf[4];
    int len = utf8_encode(c, buf);
    ASSERT_EQ_INT(1, len);
    ASSERT_EQ_INT((int)c, (int)buf[0]);
  }
  PASS();
}

TEST(utf8_encode_2byte_boundary_low) {
  uint8_t buf[4];
  int len = utf8_encode(0x80, buf);
  ASSERT_EQ_INT(2, len);
  ASSERT_EQ_INT(0xC2, buf[0]);
  ASSERT_EQ_INT(0x80, buf[1]);
  PASS();
}

TEST(utf8_encode_2byte_boundary_high) {
  uint8_t buf[4];
  int len = utf8_encode(0x7FF, buf);
  ASSERT_EQ_INT(2, len);
  ASSERT_EQ_INT(0xDF, buf[0]);
  ASSERT_EQ_INT(0xBF, buf[1]);
  PASS();
}

TEST(utf8_encode_3byte_boundary_low) {
  uint8_t buf[4];
  int len = utf8_encode(0x800, buf);
  ASSERT_EQ_INT(3, len);
  ASSERT_EQ_INT(0xE0, buf[0]);
  ASSERT_EQ_INT(0xA0, buf[1]);
  ASSERT_EQ_INT(0x80, buf[2]);
  PASS();
}

TEST(utf8_encode_3byte_boundary_high) {
  uint8_t buf[4];
  int len = utf8_encode(0xFFFF, buf);
  ASSERT_EQ_INT(3, len);
  ASSERT_EQ_INT(0xEF, buf[0]);
  ASSERT_EQ_INT(0xBF, buf[1]);
  ASSERT_EQ_INT(0xBF, buf[2]);
  PASS();
}

TEST(utf8_encode_4byte_boundary_low) {
  uint8_t buf[4];
  int len = utf8_encode(0x10000, buf);
  ASSERT_EQ_INT(4, len);
  ASSERT_EQ_INT(0xF0, buf[0]);
  ASSERT_EQ_INT(0x90, buf[1]);
  ASSERT_EQ_INT(0x80, buf[2]);
  ASSERT_EQ_INT(0x80, buf[3]);
  PASS();
}

TEST(utf8_encode_4byte_boundary_high) {
  uint8_t buf[4];
  int len = utf8_encode(0x10FFFF, buf);
  ASSERT_EQ_INT(4, len);
  ASSERT_EQ_INT(0xF4, buf[0]);
  ASSERT_EQ_INT(0x8F, buf[1]);
  ASSERT_EQ_INT(0xBF, buf[2]);
  ASSERT_EQ_INT(0xBF, buf[3]);
  PASS();
}

TEST(utf8_roundtrip_exhaustive_1byte) {
  for (uint32_t cp = 0; cp < 0x80; cp++) {
    uint8_t buf[4];
    int enc_len = utf8_encode(cp, buf);
    uint32_t decoded;
    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);
    ASSERT_EQ_INT(enc_len, dec_len);
    ASSERT_EQ_INT((int)cp, (int)decoded);
  }
  PASS();
}

TEST(utf8_roundtrip_exhaustive_2byte) {
  for (uint32_t cp = 0x80; cp < 0x800; cp++) {
    uint8_t buf[4];
    int enc_len = utf8_encode(cp, buf);
    uint32_t decoded;
    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);
    ASSERT_EQ_INT(enc_len, dec_len);
    ASSERT_EQ_INT((int)cp, (int)decoded);
  }
  PASS();
}

TEST(utf8_roundtrip_sample_3byte) {
  uint32_t samples[] = {0x800,  0x1000, 0x2000, 0x3000, 0x4E00,
                        0x4E2D, 0x65E5, 0x672C, 0x8A9E, 0xFFFF};
  for (size_t i = 0; i < 10; i++) {
    uint8_t buf[4];
    int enc_len = utf8_encode(samples[i], buf);
    uint32_t decoded;
    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);
    ASSERT_EQ_INT(enc_len, dec_len);
    ASSERT_EQ_INT((int)samples[i], (int)decoded);
  }
  PASS();
}

TEST(utf8_roundtrip_sample_4byte) {
  uint32_t samples[] = {0x10000, 0x1F600, 0x1F64F, 0x1F680, 0x10FFFF};
  for (size_t i = 0; i < 5; i++) {
    uint8_t buf[4];
    int enc_len = utf8_encode(samples[i], buf);
    uint32_t decoded;
    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);
    ASSERT_EQ_INT(enc_len, dec_len);
    ASSERT_EQ_INT((int)samples[i], (int)decoded);
  }
  PASS();
}

void run_utf8_generated_tests(void) {
  TEST_SUITE("UTF-8 Generated Tests");
  RUN_TEST(utf8_decode_empty_buffer);
  RUN_TEST(utf8_decode_all_ascii);
  RUN_TEST(utf8_decode_2byte_boundary_low);
  RUN_TEST(utf8_decode_2byte_boundary_high);
  RUN_TEST(utf8_decode_3byte_boundary_low);
  RUN_TEST(utf8_decode_3byte_boundary_high);
  RUN_TEST(utf8_decode_4byte_boundary_low);
  RUN_TEST(utf8_decode_4byte_boundary_high);
  RUN_TEST(utf8_decode_truncated_2byte);
  RUN_TEST(utf8_decode_truncated_3byte_1);
  RUN_TEST(utf8_decode_truncated_3byte_2);
  RUN_TEST(utf8_decode_truncated_4byte_1);
  RUN_TEST(utf8_decode_truncated_4byte_2);
  RUN_TEST(utf8_decode_truncated_4byte_3);
  RUN_TEST(utf8_encode_all_ascii);
  RUN_TEST(utf8_encode_2byte_boundary_low);
  RUN_TEST(utf8_encode_2byte_boundary_high);
  RUN_TEST(utf8_encode_3byte_boundary_low);
  RUN_TEST(utf8_encode_3byte_boundary_high);
  RUN_TEST(utf8_encode_4byte_boundary_low);
  RUN_TEST(utf8_encode_4byte_boundary_high);
  RUN_TEST(utf8_roundtrip_exhaustive_1byte);
  RUN_TEST(utf8_roundtrip_exhaustive_2byte);
  RUN_TEST(utf8_roundtrip_sample_3byte);
  RUN_TEST(utf8_roundtrip_sample_4byte);
}

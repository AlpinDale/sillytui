#include "chat/history.h"
#include "core/config.h"
#include "test_framework.h"
#include "tokenizer/tiktoken.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

TEST(edge_utf8_sequence_len_0) {
  uint8_t buf[1];
  for (int i = 0; i < 0; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 0, &cp);
  ASSERT_EQ_INT(0, result);
  PASS();
}

TEST(edge_utf8_sequence_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 1, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 2, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 3, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 5, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 6, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 7, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 8, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 9, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 10, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 11, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 12, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 13, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 14, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 15, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 16, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 17, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 18, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 19, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 20, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 21, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 22, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 23, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 24, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 25, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 26, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 27, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 28, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 29, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 30, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 31, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 32, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 33, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 34, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 35, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 36, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 37, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 38, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 39, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 40, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 41, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 42, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 43, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 44, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 45, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 46, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 47, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 48, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 49, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 50, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 51, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 52, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 53, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 54, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 55, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 56, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 57, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 58, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 59, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 60, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 61, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 62, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 63, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_sequence_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = 'x';
  uint32_t cp;
  int result = utf8_decode(buf, 64, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC0) {
  uint8_t buf[4] = {0xC0, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC1) {
  uint8_t buf[4] = {0xC1, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC2) {
  uint8_t buf[4] = {0xC2, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC3) {
  uint8_t buf[4] = {0xC3, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC4) {
  uint8_t buf[4] = {0xC4, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC5) {
  uint8_t buf[4] = {0xC5, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC6) {
  uint8_t buf[4] = {0xC6, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC7) {
  uint8_t buf[4] = {0xC7, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC8) {
  uint8_t buf[4] = {0xC8, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xC9) {
  uint8_t buf[4] = {0xC9, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xCA) {
  uint8_t buf[4] = {0xCA, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xCB) {
  uint8_t buf[4] = {0xCB, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xCC) {
  uint8_t buf[4] = {0xCC, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xCD) {
  uint8_t buf[4] = {0xCD, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xCE) {
  uint8_t buf[4] = {0xCE, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xCF) {
  uint8_t buf[4] = {0xCF, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD0) {
  uint8_t buf[4] = {0xD0, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD1) {
  uint8_t buf[4] = {0xD1, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD2) {
  uint8_t buf[4] = {0xD2, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD3) {
  uint8_t buf[4] = {0xD3, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD4) {
  uint8_t buf[4] = {0xD4, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD5) {
  uint8_t buf[4] = {0xD5, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD6) {
  uint8_t buf[4] = {0xD6, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD7) {
  uint8_t buf[4] = {0xD7, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD8) {
  uint8_t buf[4] = {0xD8, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xD9) {
  uint8_t buf[4] = {0xD9, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xDA) {
  uint8_t buf[4] = {0xDA, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xDB) {
  uint8_t buf[4] = {0xDB, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xDC) {
  uint8_t buf[4] = {0xDC, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xDD) {
  uint8_t buf[4] = {0xDD, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xDE) {
  uint8_t buf[4] = {0xDE, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xDF) {
  uint8_t buf[4] = {0xDF, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE0) {
  uint8_t buf[4] = {0xE0, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE1) {
  uint8_t buf[4] = {0xE1, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE2) {
  uint8_t buf[4] = {0xE2, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE3) {
  uint8_t buf[4] = {0xE3, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE4) {
  uint8_t buf[4] = {0xE4, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE5) {
  uint8_t buf[4] = {0xE5, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE6) {
  uint8_t buf[4] = {0xE6, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE7) {
  uint8_t buf[4] = {0xE7, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE8) {
  uint8_t buf[4] = {0xE8, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xE9) {
  uint8_t buf[4] = {0xE9, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xEA) {
  uint8_t buf[4] = {0xEA, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xEB) {
  uint8_t buf[4] = {0xEB, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xEC) {
  uint8_t buf[4] = {0xEC, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xED) {
  uint8_t buf[4] = {0xED, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xEE) {
  uint8_t buf[4] = {0xEE, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xEF) {
  uint8_t buf[4] = {0xEF, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF0) {
  uint8_t buf[4] = {0xF0, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF1) {
  uint8_t buf[4] = {0xF1, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF2) {
  uint8_t buf[4] = {0xF2, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF3) {
  uint8_t buf[4] = {0xF3, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF4) {
  uint8_t buf[4] = {0xF4, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF5) {
  uint8_t buf[4] = {0xF5, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF6) {
  uint8_t buf[4] = {0xF6, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF7) {
  uint8_t buf[4] = {0xF7, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF8) {
  uint8_t buf[4] = {0xF8, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xF9) {
  uint8_t buf[4] = {0xF9, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xFA) {
  uint8_t buf[4] = {0xFA, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xFB) {
  uint8_t buf[4] = {0xFB, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xFC) {
  uint8_t buf[4] = {0xFC, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xFD) {
  uint8_t buf[4] = {0xFD, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xFE) {
  uint8_t buf[4] = {0xFE, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_utf8_leading_byte_0xFF) {
  uint8_t buf[4] = {0xFF, 0x80, 0x80, 0x80};
  uint32_t cp;
  int result = utf8_decode(buf, 4, &cp);
  ASSERT(result >= 1);
  PASS();
}

TEST(edge_history_100_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 100; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Msg %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(100, h.count);
  for (int i = 99; i >= 0; i--) {
    const char *msg = history_get(&h, (size_t)i);
    ASSERT_NOT_NULL(msg);
  }
  history_free(&h);
  PASS();
}

TEST(edge_history_swipe_stress) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int i = 0; i < 32; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", i);
    history_add_swipe(&h, idx, buf);
  }
  for (int i = 0; i < 33; i++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, (size_t)i));
    const char *swipe = history_get_swipe(&h, idx, (size_t)i);
    ASSERT_NOT_NULL(swipe);
  }
  history_free(&h);
  PASS();
}

TEST(edge_history_interleaved_ops) {
  ChatHistory h;
  history_init(&h);
  for (int iter = 0; iter < 50; iter++) {
    history_add(&h, "A");
    history_add(&h, "B");
    history_delete(&h, 0);
    history_update(&h, 0, "Updated");
  }
  history_free(&h);
  PASS();
}

TEST(edge_config_max_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 32; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "M%d", i);
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ModelConfig extra = {0};
  snprintf(extra.name, sizeof(extra.name), "Extra");
  ASSERT_FALSE(config_add_model(&mf, &extra));
  PASS();
}

TEST(edge_config_set_active_boundary) {
  ModelsFile mf = {0};
  for (int i = 0; i < 5; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "M%d", i);
    config_add_model(&mf, &m);
  }
  config_set_active(&mf, 0);
  ASSERT_EQ_INT(0, mf.active_index);
  config_set_active(&mf, 4);
  ASSERT_EQ_INT(4, mf.active_index);
  config_set_active(&mf, 100);
  ASSERT_EQ_INT(4, mf.active_index);
  PASS();
}

TEST(edge_pretokenize_unicode_mix) {
  SpanList spans = {0};
  const char *inputs[] = {
      "Hello日本語", "日本語Hello", "Hello 日本語 World",
      "123日本456",  "日本123語",   " 日本 ",
      "日",          "日本",
  };
  for (size_t i = 0; i < 8; i++) {
    spans.count = 0;
    int result = pretokenize_cl100k(inputs[i], &spans);
    ASSERT(result >= 0);
    size_t covered = 0;
    for (size_t j = 0; j < spans.count; j++) {
      covered += spans.spans[j].end - spans.spans[j].start;
    }
    ASSERT_EQ_SIZE(strlen(inputs[i]), covered);
    free(spans.spans);
    spans.spans = NULL;
    spans.cap = 0;
  }
  PASS();
}

TEST(edge_pretokenize_special_chars) {
  SpanList spans = {0};
  const char *inputs[] = {
      "\n\n\n", "\r\n\r\n", "\t\t\t", "   \n   ", "a\nb\nc", "line1\r\nline2",
  };
  for (size_t i = 0; i < 6; i++) {
    spans.count = 0;
    int result = pretokenize_cl100k(inputs[i], &spans);
    ASSERT(result >= 0);
    free(spans.spans);
    spans.spans = NULL;
    spans.cap = 0;
  }
  PASS();
}

TEST(edge_pretokenize_spaces_1) {
  char buf[2];
  for (int i = 0; i < 1; i++)
    buf[i] = ' ';
  buf[1] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_11) {
  char buf[12];
  for (int i = 0; i < 11; i++)
    buf[i] = ' ';
  buf[11] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_21) {
  char buf[22];
  for (int i = 0; i < 21; i++)
    buf[i] = ' ';
  buf[21] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_31) {
  char buf[32];
  for (int i = 0; i < 31; i++)
    buf[i] = ' ';
  buf[31] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_41) {
  char buf[42];
  for (int i = 0; i < 41; i++)
    buf[i] = ' ';
  buf[41] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_51) {
  char buf[52];
  for (int i = 0; i < 51; i++)
    buf[i] = ' ';
  buf[51] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_61) {
  char buf[62];
  for (int i = 0; i < 61; i++)
    buf[i] = ' ';
  buf[61] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_71) {
  char buf[72];
  for (int i = 0; i < 71; i++)
    buf[i] = ' ';
  buf[71] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_81) {
  char buf[82];
  for (int i = 0; i < 81; i++)
    buf[i] = ' ';
  buf[81] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_spaces_91) {
  char buf[92];
  for (int i = 0; i < 91; i++)
    buf[i] = ' ';
  buf[91] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_1) {
  char buf[2];
  for (int i = 0; i < 1; i++)
    buf[i] = '0' + (i % 10);
  buf[1] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_11) {
  char buf[12];
  for (int i = 0; i < 11; i++)
    buf[i] = '0' + (i % 10);
  buf[11] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_21) {
  char buf[22];
  for (int i = 0; i < 21; i++)
    buf[i] = '0' + (i % 10);
  buf[21] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_31) {
  char buf[32];
  for (int i = 0; i < 31; i++)
    buf[i] = '0' + (i % 10);
  buf[31] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_41) {
  char buf[42];
  for (int i = 0; i < 41; i++)
    buf[i] = '0' + (i % 10);
  buf[41] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_51) {
  char buf[52];
  for (int i = 0; i < 51; i++)
    buf[i] = '0' + (i % 10);
  buf[51] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_61) {
  char buf[62];
  for (int i = 0; i < 61; i++)
    buf[i] = '0' + (i % 10);
  buf[61] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_71) {
  char buf[72];
  for (int i = 0; i < 71; i++)
    buf[i] = '0' + (i % 10);
  buf[71] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_81) {
  char buf[82];
  for (int i = 0; i < 81; i++)
    buf[i] = '0' + (i % 10);
  buf[81] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_pretokenize_digits_91) {
  char buf[92];
  for (int i = 0; i < 91; i++)
    buf[i] = '0' + (i % 10);
  buf[91] = '\0';
  SpanList spans = {0};
  int result = pretokenize_cl100k(buf, &spans);
  ASSERT(result >= 0);
  free(spans.spans);
  PASS();
}

TEST(edge_api_type_all_values) {
  for (int t = 0; t < API_TYPE_COUNT; t++) {
    const char *name = api_type_name((ApiType)t);
    ASSERT_NOT_NULL(name);
    ASSERT(strlen(name) > 0);
    ApiType parsed = api_type_from_name(name);
    ASSERT_EQ_INT(t, (int)parsed);
  }
  PASS();
}

TEST(edge_api_type_invalid_names) {
  const char *invalid[] = {"foo", "bar", "baz", "unknown", "test", NULL};
  for (int i = 0; invalid[i] != NULL; i++) {
    ApiType t = api_type_from_name(invalid[i]);
    ASSERT_EQ_INT(API_TYPE_APHRODITE, (int)t);
  }
  PASS();
}

void run_edge_case_generated_tests(void) {
  TEST_SUITE("Edge Case Generated Tests");
  RUN_TEST(edge_utf8_sequence_len_0);
  RUN_TEST(edge_utf8_sequence_len_1);
  RUN_TEST(edge_utf8_sequence_len_2);
  RUN_TEST(edge_utf8_sequence_len_3);
  RUN_TEST(edge_utf8_sequence_len_4);
  RUN_TEST(edge_utf8_sequence_len_5);
  RUN_TEST(edge_utf8_sequence_len_6);
  RUN_TEST(edge_utf8_sequence_len_7);
  RUN_TEST(edge_utf8_sequence_len_8);
  RUN_TEST(edge_utf8_sequence_len_9);
  RUN_TEST(edge_utf8_sequence_len_10);
  RUN_TEST(edge_utf8_sequence_len_11);
  RUN_TEST(edge_utf8_sequence_len_12);
  RUN_TEST(edge_utf8_sequence_len_13);
  RUN_TEST(edge_utf8_sequence_len_14);
  RUN_TEST(edge_utf8_sequence_len_15);
  RUN_TEST(edge_utf8_sequence_len_16);
  RUN_TEST(edge_utf8_sequence_len_17);
  RUN_TEST(edge_utf8_sequence_len_18);
  RUN_TEST(edge_utf8_sequence_len_19);
  RUN_TEST(edge_utf8_sequence_len_20);
  RUN_TEST(edge_utf8_sequence_len_21);
  RUN_TEST(edge_utf8_sequence_len_22);
  RUN_TEST(edge_utf8_sequence_len_23);
  RUN_TEST(edge_utf8_sequence_len_24);
  RUN_TEST(edge_utf8_sequence_len_25);
  RUN_TEST(edge_utf8_sequence_len_26);
  RUN_TEST(edge_utf8_sequence_len_27);
  RUN_TEST(edge_utf8_sequence_len_28);
  RUN_TEST(edge_utf8_sequence_len_29);
  RUN_TEST(edge_utf8_sequence_len_30);
  RUN_TEST(edge_utf8_sequence_len_31);
  RUN_TEST(edge_utf8_sequence_len_32);
  RUN_TEST(edge_utf8_sequence_len_33);
  RUN_TEST(edge_utf8_sequence_len_34);
  RUN_TEST(edge_utf8_sequence_len_35);
  RUN_TEST(edge_utf8_sequence_len_36);
  RUN_TEST(edge_utf8_sequence_len_37);
  RUN_TEST(edge_utf8_sequence_len_38);
  RUN_TEST(edge_utf8_sequence_len_39);
  RUN_TEST(edge_utf8_sequence_len_40);
  RUN_TEST(edge_utf8_sequence_len_41);
  RUN_TEST(edge_utf8_sequence_len_42);
  RUN_TEST(edge_utf8_sequence_len_43);
  RUN_TEST(edge_utf8_sequence_len_44);
  RUN_TEST(edge_utf8_sequence_len_45);
  RUN_TEST(edge_utf8_sequence_len_46);
  RUN_TEST(edge_utf8_sequence_len_47);
  RUN_TEST(edge_utf8_sequence_len_48);
  RUN_TEST(edge_utf8_sequence_len_49);
  RUN_TEST(edge_utf8_sequence_len_50);
  RUN_TEST(edge_utf8_sequence_len_51);
  RUN_TEST(edge_utf8_sequence_len_52);
  RUN_TEST(edge_utf8_sequence_len_53);
  RUN_TEST(edge_utf8_sequence_len_54);
  RUN_TEST(edge_utf8_sequence_len_55);
  RUN_TEST(edge_utf8_sequence_len_56);
  RUN_TEST(edge_utf8_sequence_len_57);
  RUN_TEST(edge_utf8_sequence_len_58);
  RUN_TEST(edge_utf8_sequence_len_59);
  RUN_TEST(edge_utf8_sequence_len_60);
  RUN_TEST(edge_utf8_sequence_len_61);
  RUN_TEST(edge_utf8_sequence_len_62);
  RUN_TEST(edge_utf8_sequence_len_63);
  RUN_TEST(edge_utf8_sequence_len_64);
  RUN_TEST(edge_utf8_leading_byte_0xC0);
  RUN_TEST(edge_utf8_leading_byte_0xC1);
  RUN_TEST(edge_utf8_leading_byte_0xC2);
  RUN_TEST(edge_utf8_leading_byte_0xC3);
  RUN_TEST(edge_utf8_leading_byte_0xC4);
  RUN_TEST(edge_utf8_leading_byte_0xC5);
  RUN_TEST(edge_utf8_leading_byte_0xC6);
  RUN_TEST(edge_utf8_leading_byte_0xC7);
  RUN_TEST(edge_utf8_leading_byte_0xC8);
  RUN_TEST(edge_utf8_leading_byte_0xC9);
  RUN_TEST(edge_utf8_leading_byte_0xCA);
  RUN_TEST(edge_utf8_leading_byte_0xCB);
  RUN_TEST(edge_utf8_leading_byte_0xCC);
  RUN_TEST(edge_utf8_leading_byte_0xCD);
  RUN_TEST(edge_utf8_leading_byte_0xCE);
  RUN_TEST(edge_utf8_leading_byte_0xCF);
  RUN_TEST(edge_utf8_leading_byte_0xD0);
  RUN_TEST(edge_utf8_leading_byte_0xD1);
  RUN_TEST(edge_utf8_leading_byte_0xD2);
  RUN_TEST(edge_utf8_leading_byte_0xD3);
  RUN_TEST(edge_utf8_leading_byte_0xD4);
  RUN_TEST(edge_utf8_leading_byte_0xD5);
  RUN_TEST(edge_utf8_leading_byte_0xD6);
  RUN_TEST(edge_utf8_leading_byte_0xD7);
  RUN_TEST(edge_utf8_leading_byte_0xD8);
  RUN_TEST(edge_utf8_leading_byte_0xD9);
  RUN_TEST(edge_utf8_leading_byte_0xDA);
  RUN_TEST(edge_utf8_leading_byte_0xDB);
  RUN_TEST(edge_utf8_leading_byte_0xDC);
  RUN_TEST(edge_utf8_leading_byte_0xDD);
  RUN_TEST(edge_utf8_leading_byte_0xDE);
  RUN_TEST(edge_utf8_leading_byte_0xDF);
  RUN_TEST(edge_utf8_leading_byte_0xE0);
  RUN_TEST(edge_utf8_leading_byte_0xE1);
  RUN_TEST(edge_utf8_leading_byte_0xE2);
  RUN_TEST(edge_utf8_leading_byte_0xE3);
  RUN_TEST(edge_utf8_leading_byte_0xE4);
  RUN_TEST(edge_utf8_leading_byte_0xE5);
  RUN_TEST(edge_utf8_leading_byte_0xE6);
  RUN_TEST(edge_utf8_leading_byte_0xE7);
  RUN_TEST(edge_utf8_leading_byte_0xE8);
  RUN_TEST(edge_utf8_leading_byte_0xE9);
  RUN_TEST(edge_utf8_leading_byte_0xEA);
  RUN_TEST(edge_utf8_leading_byte_0xEB);
  RUN_TEST(edge_utf8_leading_byte_0xEC);
  RUN_TEST(edge_utf8_leading_byte_0xED);
  RUN_TEST(edge_utf8_leading_byte_0xEE);
  RUN_TEST(edge_utf8_leading_byte_0xEF);
  RUN_TEST(edge_utf8_leading_byte_0xF0);
  RUN_TEST(edge_utf8_leading_byte_0xF1);
  RUN_TEST(edge_utf8_leading_byte_0xF2);
  RUN_TEST(edge_utf8_leading_byte_0xF3);
  RUN_TEST(edge_utf8_leading_byte_0xF4);
  RUN_TEST(edge_utf8_leading_byte_0xF5);
  RUN_TEST(edge_utf8_leading_byte_0xF6);
  RUN_TEST(edge_utf8_leading_byte_0xF7);
  RUN_TEST(edge_utf8_leading_byte_0xF8);
  RUN_TEST(edge_utf8_leading_byte_0xF9);
  RUN_TEST(edge_utf8_leading_byte_0xFA);
  RUN_TEST(edge_utf8_leading_byte_0xFB);
  RUN_TEST(edge_utf8_leading_byte_0xFC);
  RUN_TEST(edge_utf8_leading_byte_0xFD);
  RUN_TEST(edge_utf8_leading_byte_0xFE);
  RUN_TEST(edge_utf8_leading_byte_0xFF);
  RUN_TEST(edge_history_100_messages);
  RUN_TEST(edge_history_swipe_stress);
  RUN_TEST(edge_history_interleaved_ops);
  RUN_TEST(edge_config_max_models);
  RUN_TEST(edge_config_set_active_boundary);
  RUN_TEST(edge_pretokenize_unicode_mix);
  RUN_TEST(edge_pretokenize_special_chars);
  RUN_TEST(edge_pretokenize_spaces_1);
  RUN_TEST(edge_pretokenize_digits_1);
  RUN_TEST(edge_pretokenize_spaces_11);
  RUN_TEST(edge_pretokenize_digits_11);
  RUN_TEST(edge_pretokenize_spaces_21);
  RUN_TEST(edge_pretokenize_digits_21);
  RUN_TEST(edge_pretokenize_spaces_31);
  RUN_TEST(edge_pretokenize_digits_31);
  RUN_TEST(edge_pretokenize_spaces_41);
  RUN_TEST(edge_pretokenize_digits_41);
  RUN_TEST(edge_pretokenize_spaces_51);
  RUN_TEST(edge_pretokenize_digits_51);
  RUN_TEST(edge_pretokenize_spaces_61);
  RUN_TEST(edge_pretokenize_digits_61);
  RUN_TEST(edge_pretokenize_spaces_71);
  RUN_TEST(edge_pretokenize_digits_71);
  RUN_TEST(edge_pretokenize_spaces_81);
  RUN_TEST(edge_pretokenize_digits_81);
  RUN_TEST(edge_pretokenize_spaces_91);
  RUN_TEST(edge_pretokenize_digits_91);
  RUN_TEST(edge_api_type_all_values);
  RUN_TEST(edge_api_type_invalid_names);
}

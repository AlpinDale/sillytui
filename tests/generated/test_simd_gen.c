#include "test_framework.h"
#include "tokenizer/simd.h"
#include <stdlib.h>
#include <string.h>

TEST(simd_hash_len_0) {
  uint8_t buf[1];
  for (int i = 0; i < 0; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 0);
  uint64_t h2 = simd_hash_bytes(buf, 0);
  ASSERT(h1 == h2);
  PASS();
}

TEST(simd_hash_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 1);
  uint64_t h2 = simd_hash_bytes(buf, 1);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 1);
  if (1 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 2);
  uint64_t h2 = simd_hash_bytes(buf, 2);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 2);
  if (2 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 3);
  uint64_t h2 = simd_hash_bytes(buf, 3);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 3);
  if (3 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 4);
  uint64_t h2 = simd_hash_bytes(buf, 4);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 4);
  if (4 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 5);
  uint64_t h2 = simd_hash_bytes(buf, 5);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 5);
  if (5 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 6);
  uint64_t h2 = simd_hash_bytes(buf, 6);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 6);
  if (6 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 7);
  uint64_t h2 = simd_hash_bytes(buf, 7);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 7);
  if (7 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 8);
  uint64_t h2 = simd_hash_bytes(buf, 8);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 8);
  if (8 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 9);
  uint64_t h2 = simd_hash_bytes(buf, 9);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 9);
  if (9 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 10);
  uint64_t h2 = simd_hash_bytes(buf, 10);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 10);
  if (10 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 11);
  uint64_t h2 = simd_hash_bytes(buf, 11);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 11);
  if (11 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 12);
  uint64_t h2 = simd_hash_bytes(buf, 12);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 12);
  if (12 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 13);
  uint64_t h2 = simd_hash_bytes(buf, 13);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 13);
  if (13 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 14);
  uint64_t h2 = simd_hash_bytes(buf, 14);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 14);
  if (14 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 15);
  uint64_t h2 = simd_hash_bytes(buf, 15);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 15);
  if (15 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 16);
  uint64_t h2 = simd_hash_bytes(buf, 16);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 16);
  if (16 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 17);
  uint64_t h2 = simd_hash_bytes(buf, 17);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 17);
  if (17 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 18);
  uint64_t h2 = simd_hash_bytes(buf, 18);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 18);
  if (18 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 19);
  uint64_t h2 = simd_hash_bytes(buf, 19);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 19);
  if (19 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 20);
  uint64_t h2 = simd_hash_bytes(buf, 20);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 20);
  if (20 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 21);
  uint64_t h2 = simd_hash_bytes(buf, 21);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 21);
  if (21 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 22);
  uint64_t h2 = simd_hash_bytes(buf, 22);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 22);
  if (22 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 23);
  uint64_t h2 = simd_hash_bytes(buf, 23);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 23);
  if (23 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 24);
  uint64_t h2 = simd_hash_bytes(buf, 24);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 24);
  if (24 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 25);
  uint64_t h2 = simd_hash_bytes(buf, 25);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 25);
  if (25 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 26);
  uint64_t h2 = simd_hash_bytes(buf, 26);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 26);
  if (26 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 27);
  uint64_t h2 = simd_hash_bytes(buf, 27);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 27);
  if (27 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 28);
  uint64_t h2 = simd_hash_bytes(buf, 28);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 28);
  if (28 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 29);
  uint64_t h2 = simd_hash_bytes(buf, 29);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 29);
  if (29 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 30);
  uint64_t h2 = simd_hash_bytes(buf, 30);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 30);
  if (30 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 31);
  uint64_t h2 = simd_hash_bytes(buf, 31);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 31);
  if (31 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 32);
  uint64_t h2 = simd_hash_bytes(buf, 32);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 32);
  if (32 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 33);
  uint64_t h2 = simd_hash_bytes(buf, 33);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 33);
  if (33 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 34);
  uint64_t h2 = simd_hash_bytes(buf, 34);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 34);
  if (34 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 35);
  uint64_t h2 = simd_hash_bytes(buf, 35);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 35);
  if (35 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 36);
  uint64_t h2 = simd_hash_bytes(buf, 36);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 36);
  if (36 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 37);
  uint64_t h2 = simd_hash_bytes(buf, 37);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 37);
  if (37 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 38);
  uint64_t h2 = simd_hash_bytes(buf, 38);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 38);
  if (38 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 39);
  uint64_t h2 = simd_hash_bytes(buf, 39);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 39);
  if (39 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 40);
  uint64_t h2 = simd_hash_bytes(buf, 40);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 40);
  if (40 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 41);
  uint64_t h2 = simd_hash_bytes(buf, 41);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 41);
  if (41 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 42);
  uint64_t h2 = simd_hash_bytes(buf, 42);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 42);
  if (42 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 43);
  uint64_t h2 = simd_hash_bytes(buf, 43);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 43);
  if (43 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 44);
  uint64_t h2 = simd_hash_bytes(buf, 44);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 44);
  if (44 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 45);
  uint64_t h2 = simd_hash_bytes(buf, 45);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 45);
  if (45 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 46);
  uint64_t h2 = simd_hash_bytes(buf, 46);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 46);
  if (46 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 47);
  uint64_t h2 = simd_hash_bytes(buf, 47);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 47);
  if (47 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 48);
  uint64_t h2 = simd_hash_bytes(buf, 48);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 48);
  if (48 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 49);
  uint64_t h2 = simd_hash_bytes(buf, 49);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 49);
  if (49 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 50);
  uint64_t h2 = simd_hash_bytes(buf, 50);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 50);
  if (50 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 51);
  uint64_t h2 = simd_hash_bytes(buf, 51);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 51);
  if (51 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 52);
  uint64_t h2 = simd_hash_bytes(buf, 52);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 52);
  if (52 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 53);
  uint64_t h2 = simd_hash_bytes(buf, 53);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 53);
  if (53 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 54);
  uint64_t h2 = simd_hash_bytes(buf, 54);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 54);
  if (54 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 55);
  uint64_t h2 = simd_hash_bytes(buf, 55);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 55);
  if (55 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 56);
  uint64_t h2 = simd_hash_bytes(buf, 56);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 56);
  if (56 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 57);
  uint64_t h2 = simd_hash_bytes(buf, 57);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 57);
  if (57 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 58);
  uint64_t h2 = simd_hash_bytes(buf, 58);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 58);
  if (58 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 59);
  uint64_t h2 = simd_hash_bytes(buf, 59);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 59);
  if (59 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 60);
  uint64_t h2 = simd_hash_bytes(buf, 60);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 60);
  if (60 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 61);
  uint64_t h2 = simd_hash_bytes(buf, 61);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 61);
  if (61 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 62);
  uint64_t h2 = simd_hash_bytes(buf, 62);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 62);
  if (62 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 63);
  uint64_t h2 = simd_hash_bytes(buf, 63);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 63);
  if (63 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 64);
  uint64_t h2 = simd_hash_bytes(buf, 64);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 64);
  if (64 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_65) {
  uint8_t buf[65];
  for (int i = 0; i < 65; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 65);
  uint64_t h2 = simd_hash_bytes(buf, 65);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 65);
  if (65 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_66) {
  uint8_t buf[66];
  for (int i = 0; i < 66; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 66);
  uint64_t h2 = simd_hash_bytes(buf, 66);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 66);
  if (66 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_67) {
  uint8_t buf[67];
  for (int i = 0; i < 67; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 67);
  uint64_t h2 = simd_hash_bytes(buf, 67);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 67);
  if (67 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_68) {
  uint8_t buf[68];
  for (int i = 0; i < 68; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 68);
  uint64_t h2 = simd_hash_bytes(buf, 68);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 68);
  if (68 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_69) {
  uint8_t buf[69];
  for (int i = 0; i < 69; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 69);
  uint64_t h2 = simd_hash_bytes(buf, 69);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 69);
  if (69 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_70) {
  uint8_t buf[70];
  for (int i = 0; i < 70; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 70);
  uint64_t h2 = simd_hash_bytes(buf, 70);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 70);
  if (70 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_71) {
  uint8_t buf[71];
  for (int i = 0; i < 71; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 71);
  uint64_t h2 = simd_hash_bytes(buf, 71);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 71);
  if (71 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_72) {
  uint8_t buf[72];
  for (int i = 0; i < 72; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 72);
  uint64_t h2 = simd_hash_bytes(buf, 72);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 72);
  if (72 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_73) {
  uint8_t buf[73];
  for (int i = 0; i < 73; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 73);
  uint64_t h2 = simd_hash_bytes(buf, 73);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 73);
  if (73 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_74) {
  uint8_t buf[74];
  for (int i = 0; i < 74; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 74);
  uint64_t h2 = simd_hash_bytes(buf, 74);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 74);
  if (74 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_75) {
  uint8_t buf[75];
  for (int i = 0; i < 75; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 75);
  uint64_t h2 = simd_hash_bytes(buf, 75);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 75);
  if (75 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_76) {
  uint8_t buf[76];
  for (int i = 0; i < 76; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 76);
  uint64_t h2 = simd_hash_bytes(buf, 76);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 76);
  if (76 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_77) {
  uint8_t buf[77];
  for (int i = 0; i < 77; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 77);
  uint64_t h2 = simd_hash_bytes(buf, 77);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 77);
  if (77 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_78) {
  uint8_t buf[78];
  for (int i = 0; i < 78; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 78);
  uint64_t h2 = simd_hash_bytes(buf, 78);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 78);
  if (78 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_79) {
  uint8_t buf[79];
  for (int i = 0; i < 79; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 79);
  uint64_t h2 = simd_hash_bytes(buf, 79);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 79);
  if (79 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_80) {
  uint8_t buf[80];
  for (int i = 0; i < 80; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 80);
  uint64_t h2 = simd_hash_bytes(buf, 80);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 80);
  if (80 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_81) {
  uint8_t buf[81];
  for (int i = 0; i < 81; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 81);
  uint64_t h2 = simd_hash_bytes(buf, 81);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 81);
  if (81 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_82) {
  uint8_t buf[82];
  for (int i = 0; i < 82; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 82);
  uint64_t h2 = simd_hash_bytes(buf, 82);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 82);
  if (82 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_83) {
  uint8_t buf[83];
  for (int i = 0; i < 83; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 83);
  uint64_t h2 = simd_hash_bytes(buf, 83);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 83);
  if (83 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_84) {
  uint8_t buf[84];
  for (int i = 0; i < 84; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 84);
  uint64_t h2 = simd_hash_bytes(buf, 84);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 84);
  if (84 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_85) {
  uint8_t buf[85];
  for (int i = 0; i < 85; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 85);
  uint64_t h2 = simd_hash_bytes(buf, 85);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 85);
  if (85 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_86) {
  uint8_t buf[86];
  for (int i = 0; i < 86; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 86);
  uint64_t h2 = simd_hash_bytes(buf, 86);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 86);
  if (86 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_87) {
  uint8_t buf[87];
  for (int i = 0; i < 87; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 87);
  uint64_t h2 = simd_hash_bytes(buf, 87);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 87);
  if (87 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_88) {
  uint8_t buf[88];
  for (int i = 0; i < 88; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 88);
  uint64_t h2 = simd_hash_bytes(buf, 88);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 88);
  if (88 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_89) {
  uint8_t buf[89];
  for (int i = 0; i < 89; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 89);
  uint64_t h2 = simd_hash_bytes(buf, 89);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 89);
  if (89 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_90) {
  uint8_t buf[90];
  for (int i = 0; i < 90; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 90);
  uint64_t h2 = simd_hash_bytes(buf, 90);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 90);
  if (90 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_91) {
  uint8_t buf[91];
  for (int i = 0; i < 91; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 91);
  uint64_t h2 = simd_hash_bytes(buf, 91);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 91);
  if (91 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_92) {
  uint8_t buf[92];
  for (int i = 0; i < 92; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 92);
  uint64_t h2 = simd_hash_bytes(buf, 92);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 92);
  if (92 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_93) {
  uint8_t buf[93];
  for (int i = 0; i < 93; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 93);
  uint64_t h2 = simd_hash_bytes(buf, 93);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 93);
  if (93 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_94) {
  uint8_t buf[94];
  for (int i = 0; i < 94; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 94);
  uint64_t h2 = simd_hash_bytes(buf, 94);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 94);
  if (94 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_95) {
  uint8_t buf[95];
  for (int i = 0; i < 95; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 95);
  uint64_t h2 = simd_hash_bytes(buf, 95);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 95);
  if (95 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_96) {
  uint8_t buf[96];
  for (int i = 0; i < 96; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 96);
  uint64_t h2 = simd_hash_bytes(buf, 96);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 96);
  if (96 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_97) {
  uint8_t buf[97];
  for (int i = 0; i < 97; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 97);
  uint64_t h2 = simd_hash_bytes(buf, 97);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 97);
  if (97 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_98) {
  uint8_t buf[98];
  for (int i = 0; i < 98; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 98);
  uint64_t h2 = simd_hash_bytes(buf, 98);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 98);
  if (98 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_99) {
  uint8_t buf[99];
  for (int i = 0; i < 99; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 99);
  uint64_t h2 = simd_hash_bytes(buf, 99);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 99);
  if (99 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_100) {
  uint8_t buf[100];
  for (int i = 0; i < 100; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 100);
  uint64_t h2 = simd_hash_bytes(buf, 100);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 100);
  if (100 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_101) {
  uint8_t buf[101];
  for (int i = 0; i < 101; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 101);
  uint64_t h2 = simd_hash_bytes(buf, 101);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 101);
  if (101 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_102) {
  uint8_t buf[102];
  for (int i = 0; i < 102; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 102);
  uint64_t h2 = simd_hash_bytes(buf, 102);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 102);
  if (102 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_103) {
  uint8_t buf[103];
  for (int i = 0; i < 103; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 103);
  uint64_t h2 = simd_hash_bytes(buf, 103);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 103);
  if (103 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_104) {
  uint8_t buf[104];
  for (int i = 0; i < 104; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 104);
  uint64_t h2 = simd_hash_bytes(buf, 104);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 104);
  if (104 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_105) {
  uint8_t buf[105];
  for (int i = 0; i < 105; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 105);
  uint64_t h2 = simd_hash_bytes(buf, 105);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 105);
  if (105 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_106) {
  uint8_t buf[106];
  for (int i = 0; i < 106; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 106);
  uint64_t h2 = simd_hash_bytes(buf, 106);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 106);
  if (106 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_107) {
  uint8_t buf[107];
  for (int i = 0; i < 107; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 107);
  uint64_t h2 = simd_hash_bytes(buf, 107);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 107);
  if (107 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_108) {
  uint8_t buf[108];
  for (int i = 0; i < 108; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 108);
  uint64_t h2 = simd_hash_bytes(buf, 108);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 108);
  if (108 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_109) {
  uint8_t buf[109];
  for (int i = 0; i < 109; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 109);
  uint64_t h2 = simd_hash_bytes(buf, 109);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 109);
  if (109 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_110) {
  uint8_t buf[110];
  for (int i = 0; i < 110; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 110);
  uint64_t h2 = simd_hash_bytes(buf, 110);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 110);
  if (110 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_111) {
  uint8_t buf[111];
  for (int i = 0; i < 111; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 111);
  uint64_t h2 = simd_hash_bytes(buf, 111);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 111);
  if (111 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_112) {
  uint8_t buf[112];
  for (int i = 0; i < 112; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 112);
  uint64_t h2 = simd_hash_bytes(buf, 112);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 112);
  if (112 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_113) {
  uint8_t buf[113];
  for (int i = 0; i < 113; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 113);
  uint64_t h2 = simd_hash_bytes(buf, 113);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 113);
  if (113 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_114) {
  uint8_t buf[114];
  for (int i = 0; i < 114; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 114);
  uint64_t h2 = simd_hash_bytes(buf, 114);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 114);
  if (114 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_115) {
  uint8_t buf[115];
  for (int i = 0; i < 115; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 115);
  uint64_t h2 = simd_hash_bytes(buf, 115);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 115);
  if (115 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_116) {
  uint8_t buf[116];
  for (int i = 0; i < 116; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 116);
  uint64_t h2 = simd_hash_bytes(buf, 116);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 116);
  if (116 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_117) {
  uint8_t buf[117];
  for (int i = 0; i < 117; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 117);
  uint64_t h2 = simd_hash_bytes(buf, 117);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 117);
  if (117 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_118) {
  uint8_t buf[118];
  for (int i = 0; i < 118; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 118);
  uint64_t h2 = simd_hash_bytes(buf, 118);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 118);
  if (118 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_119) {
  uint8_t buf[119];
  for (int i = 0; i < 119; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 119);
  uint64_t h2 = simd_hash_bytes(buf, 119);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 119);
  if (119 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_120) {
  uint8_t buf[120];
  for (int i = 0; i < 120; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 120);
  uint64_t h2 = simd_hash_bytes(buf, 120);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 120);
  if (120 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_121) {
  uint8_t buf[121];
  for (int i = 0; i < 121; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 121);
  uint64_t h2 = simd_hash_bytes(buf, 121);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 121);
  if (121 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_122) {
  uint8_t buf[122];
  for (int i = 0; i < 122; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 122);
  uint64_t h2 = simd_hash_bytes(buf, 122);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 122);
  if (122 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_123) {
  uint8_t buf[123];
  for (int i = 0; i < 123; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 123);
  uint64_t h2 = simd_hash_bytes(buf, 123);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 123);
  if (123 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_124) {
  uint8_t buf[124];
  for (int i = 0; i < 124; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 124);
  uint64_t h2 = simd_hash_bytes(buf, 124);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 124);
  if (124 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_125) {
  uint8_t buf[125];
  for (int i = 0; i < 125; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 125);
  uint64_t h2 = simd_hash_bytes(buf, 125);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 125);
  if (125 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_126) {
  uint8_t buf[126];
  for (int i = 0; i < 126; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 126);
  uint64_t h2 = simd_hash_bytes(buf, 126);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 126);
  if (126 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_127) {
  uint8_t buf[127];
  for (int i = 0; i < 127; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 127);
  uint64_t h2 = simd_hash_bytes(buf, 127);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 127);
  if (127 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_hash_len_128) {
  uint8_t buf[128];
  for (int i = 0; i < 128; i++)
    buf[i] = (uint8_t)(i * 7);
  uint64_t h1 = simd_hash_bytes(buf, 128);
  uint64_t h2 = simd_hash_bytes(buf, 128);
  ASSERT(h1 == h2);
  buf[0] ^= 1;
  uint64_t h3 = simd_hash_bytes(buf, 128);
  if (128 > 0)
    ASSERT(h1 != h3);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 1);
  ASSERT_EQ_SIZE(1, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 1);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = 'a' + (i % 26);
  buf[1 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 1);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 2);
  ASSERT_EQ_SIZE(2, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 2);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = 'a' + (i % 26);
  buf[2 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 2);
  ASSERT_EQ_SIZE(1, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 3);
  ASSERT_EQ_SIZE(3, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 3);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = 'a' + (i % 26);
  buf[3 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 3);
  ASSERT_EQ_SIZE(2, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 4);
  ASSERT_EQ_SIZE(4, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 4);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = 'a' + (i % 26);
  buf[4 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 4);
  ASSERT_EQ_SIZE(3, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 5);
  ASSERT_EQ_SIZE(5, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 5);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = 'a' + (i % 26);
  buf[5 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 5);
  ASSERT_EQ_SIZE(4, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 6);
  ASSERT_EQ_SIZE(6, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 6);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = 'a' + (i % 26);
  buf[6 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 6);
  ASSERT_EQ_SIZE(5, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 7);
  ASSERT_EQ_SIZE(7, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 7);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = 'a' + (i % 26);
  buf[7 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 7);
  ASSERT_EQ_SIZE(6, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 8);
  ASSERT_EQ_SIZE(8, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 8);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = 'a' + (i % 26);
  buf[8 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 8);
  ASSERT_EQ_SIZE(7, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 9);
  ASSERT_EQ_SIZE(9, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 9);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = 'a' + (i % 26);
  buf[9 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 9);
  ASSERT_EQ_SIZE(8, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 10);
  ASSERT_EQ_SIZE(10, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 10);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = 'a' + (i % 26);
  buf[10 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 10);
  ASSERT_EQ_SIZE(9, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 11);
  ASSERT_EQ_SIZE(11, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 11);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = 'a' + (i % 26);
  buf[11 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 11);
  ASSERT_EQ_SIZE(10, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 12);
  ASSERT_EQ_SIZE(12, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 12);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = 'a' + (i % 26);
  buf[12 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 12);
  ASSERT_EQ_SIZE(11, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 13);
  ASSERT_EQ_SIZE(13, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 13);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = 'a' + (i % 26);
  buf[13 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 13);
  ASSERT_EQ_SIZE(12, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 14);
  ASSERT_EQ_SIZE(14, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 14);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = 'a' + (i % 26);
  buf[14 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 14);
  ASSERT_EQ_SIZE(13, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 15);
  ASSERT_EQ_SIZE(15, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 15);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = 'a' + (i % 26);
  buf[15 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 15);
  ASSERT_EQ_SIZE(14, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 16);
  ASSERT_EQ_SIZE(16, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 16);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = 'a' + (i % 26);
  buf[16 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 16);
  ASSERT_EQ_SIZE(15, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 17);
  ASSERT_EQ_SIZE(17, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 17);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = 'a' + (i % 26);
  buf[17 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 17);
  ASSERT_EQ_SIZE(16, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 18);
  ASSERT_EQ_SIZE(18, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 18);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = 'a' + (i % 26);
  buf[18 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 18);
  ASSERT_EQ_SIZE(17, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 19);
  ASSERT_EQ_SIZE(19, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 19);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = 'a' + (i % 26);
  buf[19 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 19);
  ASSERT_EQ_SIZE(18, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 20);
  ASSERT_EQ_SIZE(20, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 20);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = 'a' + (i % 26);
  buf[20 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 20);
  ASSERT_EQ_SIZE(19, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 21);
  ASSERT_EQ_SIZE(21, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 21);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = 'a' + (i % 26);
  buf[21 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 21);
  ASSERT_EQ_SIZE(20, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 22);
  ASSERT_EQ_SIZE(22, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 22);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = 'a' + (i % 26);
  buf[22 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 22);
  ASSERT_EQ_SIZE(21, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 23);
  ASSERT_EQ_SIZE(23, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 23);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = 'a' + (i % 26);
  buf[23 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 23);
  ASSERT_EQ_SIZE(22, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 24);
  ASSERT_EQ_SIZE(24, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 24);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = 'a' + (i % 26);
  buf[24 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 24);
  ASSERT_EQ_SIZE(23, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 25);
  ASSERT_EQ_SIZE(25, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 25);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = 'a' + (i % 26);
  buf[25 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 25);
  ASSERT_EQ_SIZE(24, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 26);
  ASSERT_EQ_SIZE(26, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 26);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = 'a' + (i % 26);
  buf[26 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 26);
  ASSERT_EQ_SIZE(25, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 27);
  ASSERT_EQ_SIZE(27, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 27);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = 'a' + (i % 26);
  buf[27 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 27);
  ASSERT_EQ_SIZE(26, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 28);
  ASSERT_EQ_SIZE(28, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 28);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = 'a' + (i % 26);
  buf[28 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 28);
  ASSERT_EQ_SIZE(27, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 29);
  ASSERT_EQ_SIZE(29, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 29);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = 'a' + (i % 26);
  buf[29 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 29);
  ASSERT_EQ_SIZE(28, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 30);
  ASSERT_EQ_SIZE(30, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 30);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = 'a' + (i % 26);
  buf[30 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 30);
  ASSERT_EQ_SIZE(29, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 31);
  ASSERT_EQ_SIZE(31, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 31);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = 'a' + (i % 26);
  buf[31 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 31);
  ASSERT_EQ_SIZE(30, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 32);
  ASSERT_EQ_SIZE(32, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 32);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = 'a' + (i % 26);
  buf[32 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 32);
  ASSERT_EQ_SIZE(31, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 33);
  ASSERT_EQ_SIZE(33, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 33);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = 'a' + (i % 26);
  buf[33 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 33);
  ASSERT_EQ_SIZE(32, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 34);
  ASSERT_EQ_SIZE(34, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 34);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = 'a' + (i % 26);
  buf[34 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 34);
  ASSERT_EQ_SIZE(33, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 35);
  ASSERT_EQ_SIZE(35, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 35);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = 'a' + (i % 26);
  buf[35 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 35);
  ASSERT_EQ_SIZE(34, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 36);
  ASSERT_EQ_SIZE(36, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 36);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = 'a' + (i % 26);
  buf[36 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 36);
  ASSERT_EQ_SIZE(35, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 37);
  ASSERT_EQ_SIZE(37, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 37);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = 'a' + (i % 26);
  buf[37 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 37);
  ASSERT_EQ_SIZE(36, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 38);
  ASSERT_EQ_SIZE(38, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 38);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = 'a' + (i % 26);
  buf[38 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 38);
  ASSERT_EQ_SIZE(37, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 39);
  ASSERT_EQ_SIZE(39, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 39);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = 'a' + (i % 26);
  buf[39 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 39);
  ASSERT_EQ_SIZE(38, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 40);
  ASSERT_EQ_SIZE(40, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 40);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = 'a' + (i % 26);
  buf[40 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 40);
  ASSERT_EQ_SIZE(39, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 41);
  ASSERT_EQ_SIZE(41, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 41);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = 'a' + (i % 26);
  buf[41 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 41);
  ASSERT_EQ_SIZE(40, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 42);
  ASSERT_EQ_SIZE(42, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 42);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = 'a' + (i % 26);
  buf[42 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 42);
  ASSERT_EQ_SIZE(41, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 43);
  ASSERT_EQ_SIZE(43, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 43);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = 'a' + (i % 26);
  buf[43 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 43);
  ASSERT_EQ_SIZE(42, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 44);
  ASSERT_EQ_SIZE(44, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 44);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = 'a' + (i % 26);
  buf[44 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 44);
  ASSERT_EQ_SIZE(43, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 45);
  ASSERT_EQ_SIZE(45, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 45);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = 'a' + (i % 26);
  buf[45 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 45);
  ASSERT_EQ_SIZE(44, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 46);
  ASSERT_EQ_SIZE(46, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 46);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = 'a' + (i % 26);
  buf[46 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 46);
  ASSERT_EQ_SIZE(45, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 47);
  ASSERT_EQ_SIZE(47, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 47);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = 'a' + (i % 26);
  buf[47 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 47);
  ASSERT_EQ_SIZE(46, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 48);
  ASSERT_EQ_SIZE(48, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 48);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = 'a' + (i % 26);
  buf[48 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 48);
  ASSERT_EQ_SIZE(47, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 49);
  ASSERT_EQ_SIZE(49, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 49);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = 'a' + (i % 26);
  buf[49 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 49);
  ASSERT_EQ_SIZE(48, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 50);
  ASSERT_EQ_SIZE(50, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 50);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = 'a' + (i % 26);
  buf[50 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 50);
  ASSERT_EQ_SIZE(49, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 51);
  ASSERT_EQ_SIZE(51, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 51);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = 'a' + (i % 26);
  buf[51 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 51);
  ASSERT_EQ_SIZE(50, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 52);
  ASSERT_EQ_SIZE(52, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 52);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = 'a' + (i % 26);
  buf[52 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 52);
  ASSERT_EQ_SIZE(51, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 53);
  ASSERT_EQ_SIZE(53, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 53);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = 'a' + (i % 26);
  buf[53 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 53);
  ASSERT_EQ_SIZE(52, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 54);
  ASSERT_EQ_SIZE(54, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 54);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = 'a' + (i % 26);
  buf[54 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 54);
  ASSERT_EQ_SIZE(53, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 55);
  ASSERT_EQ_SIZE(55, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 55);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = 'a' + (i % 26);
  buf[55 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 55);
  ASSERT_EQ_SIZE(54, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 56);
  ASSERT_EQ_SIZE(56, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 56);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = 'a' + (i % 26);
  buf[56 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 56);
  ASSERT_EQ_SIZE(55, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 57);
  ASSERT_EQ_SIZE(57, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 57);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = 'a' + (i % 26);
  buf[57 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 57);
  ASSERT_EQ_SIZE(56, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 58);
  ASSERT_EQ_SIZE(58, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 58);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = 'a' + (i % 26);
  buf[58 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 58);
  ASSERT_EQ_SIZE(57, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 59);
  ASSERT_EQ_SIZE(59, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 59);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = 'a' + (i % 26);
  buf[59 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 59);
  ASSERT_EQ_SIZE(58, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 60);
  ASSERT_EQ_SIZE(60, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 60);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = 'a' + (i % 26);
  buf[60 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 60);
  ASSERT_EQ_SIZE(59, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 61);
  ASSERT_EQ_SIZE(61, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 61);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = 'a' + (i % 26);
  buf[61 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 61);
  ASSERT_EQ_SIZE(60, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 62);
  ASSERT_EQ_SIZE(62, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 62);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = 'a' + (i % 26);
  buf[62 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 62);
  ASSERT_EQ_SIZE(61, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 63);
  ASSERT_EQ_SIZE(63, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 63);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = 'a' + (i % 26);
  buf[63 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 63);
  ASSERT_EQ_SIZE(62, pos);
  PASS();
}

TEST(simd_find_non_ascii_all_ascii_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = 'a' + (i % 26);
  size_t pos = simd_find_non_ascii(buf, 64);
  ASSERT_EQ_SIZE(64, pos);
  PASS();
}

TEST(simd_find_non_ascii_first_non_ascii_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = 'a' + (i % 26);
  buf[0] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 64);
  ASSERT_EQ_SIZE(0, pos);
  PASS();
}

TEST(simd_find_non_ascii_last_non_ascii_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = 'a' + (i % 26);
  buf[64 - 1] = 0x80;
  size_t pos = simd_find_non_ascii(buf, 64);
  ASSERT_EQ_SIZE(63, pos);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_0) {
  uint8_t buf[1];
  for (int i = 0; i < 0; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 0);
  ASSERT_EQ_SIZE(0, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 1);
  ASSERT_EQ_SIZE(1, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 2);
  ASSERT_EQ_SIZE(2, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 3);
  ASSERT_EQ_SIZE(3, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 4);
  ASSERT_EQ_SIZE(4, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 5);
  ASSERT_EQ_SIZE(5, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 6);
  ASSERT_EQ_SIZE(6, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 7);
  ASSERT_EQ_SIZE(7, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 8);
  ASSERT_EQ_SIZE(8, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 9);
  ASSERT_EQ_SIZE(9, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 10);
  ASSERT_EQ_SIZE(10, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 11);
  ASSERT_EQ_SIZE(11, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 12);
  ASSERT_EQ_SIZE(12, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 13);
  ASSERT_EQ_SIZE(13, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 14);
  ASSERT_EQ_SIZE(14, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 15);
  ASSERT_EQ_SIZE(15, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 16);
  ASSERT_EQ_SIZE(16, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 17);
  ASSERT_EQ_SIZE(17, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 18);
  ASSERT_EQ_SIZE(18, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 19);
  ASSERT_EQ_SIZE(19, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 20);
  ASSERT_EQ_SIZE(20, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 21);
  ASSERT_EQ_SIZE(21, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 22);
  ASSERT_EQ_SIZE(22, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 23);
  ASSERT_EQ_SIZE(23, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 24);
  ASSERT_EQ_SIZE(24, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 25);
  ASSERT_EQ_SIZE(25, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 26);
  ASSERT_EQ_SIZE(26, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 27);
  ASSERT_EQ_SIZE(27, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 28);
  ASSERT_EQ_SIZE(28, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 29);
  ASSERT_EQ_SIZE(29, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 30);
  ASSERT_EQ_SIZE(30, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 31);
  ASSERT_EQ_SIZE(31, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 32);
  ASSERT_EQ_SIZE(32, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 33);
  ASSERT_EQ_SIZE(33, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 34);
  ASSERT_EQ_SIZE(34, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 35);
  ASSERT_EQ_SIZE(35, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 36);
  ASSERT_EQ_SIZE(36, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 37);
  ASSERT_EQ_SIZE(37, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 38);
  ASSERT_EQ_SIZE(38, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 39);
  ASSERT_EQ_SIZE(39, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 40);
  ASSERT_EQ_SIZE(40, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 41);
  ASSERT_EQ_SIZE(41, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 42);
  ASSERT_EQ_SIZE(42, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 43);
  ASSERT_EQ_SIZE(43, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 44);
  ASSERT_EQ_SIZE(44, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 45);
  ASSERT_EQ_SIZE(45, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 46);
  ASSERT_EQ_SIZE(46, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 47);
  ASSERT_EQ_SIZE(47, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 48);
  ASSERT_EQ_SIZE(48, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 49);
  ASSERT_EQ_SIZE(49, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 50);
  ASSERT_EQ_SIZE(50, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 51);
  ASSERT_EQ_SIZE(51, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 52);
  ASSERT_EQ_SIZE(52, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 53);
  ASSERT_EQ_SIZE(53, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 54);
  ASSERT_EQ_SIZE(54, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 55);
  ASSERT_EQ_SIZE(55, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 56);
  ASSERT_EQ_SIZE(56, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 57);
  ASSERT_EQ_SIZE(57, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 58);
  ASSERT_EQ_SIZE(58, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 59);
  ASSERT_EQ_SIZE(59, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 60);
  ASSERT_EQ_SIZE(60, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 61);
  ASSERT_EQ_SIZE(61, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 62);
  ASSERT_EQ_SIZE(62, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 63);
  ASSERT_EQ_SIZE(63, count);
  PASS();
}

TEST(simd_count_utf8_chars_ascii_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = 'a' + (i % 26);
  size_t count = simd_count_utf8_chars(buf, 64);
  ASSERT_EQ_SIZE(64, count);
  PASS();
}

TEST(simd_match_ascii_letters_len_0) {
  uint8_t buf[1];
  for (int i = 0; i < 0; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 0);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_0) {
  uint8_t buf[1];
  for (int i = 0; i < 0; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 0);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 1);
  ASSERT_EQ_SIZE(1, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_1) {
  uint8_t buf[1];
  for (int i = 0; i < 1; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 1);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 2);
  ASSERT_EQ_SIZE(2, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_2) {
  uint8_t buf[2];
  for (int i = 0; i < 2; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 2);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 3);
  ASSERT_EQ_SIZE(3, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_3) {
  uint8_t buf[3];
  for (int i = 0; i < 3; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 3);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 4);
  ASSERT_EQ_SIZE(4, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_4) {
  uint8_t buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 4);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 5);
  ASSERT_EQ_SIZE(5, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_5) {
  uint8_t buf[5];
  for (int i = 0; i < 5; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 5);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 6);
  ASSERT_EQ_SIZE(6, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_6) {
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 6);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 7);
  ASSERT_EQ_SIZE(7, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_7) {
  uint8_t buf[7];
  for (int i = 0; i < 7; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 7);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 8);
  ASSERT_EQ_SIZE(8, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_8) {
  uint8_t buf[8];
  for (int i = 0; i < 8; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 8);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 9);
  ASSERT_EQ_SIZE(9, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_9) {
  uint8_t buf[9];
  for (int i = 0; i < 9; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 9);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 10);
  ASSERT_EQ_SIZE(10, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_10) {
  uint8_t buf[10];
  for (int i = 0; i < 10; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 10);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 11);
  ASSERT_EQ_SIZE(11, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_11) {
  uint8_t buf[11];
  for (int i = 0; i < 11; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 11);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 12);
  ASSERT_EQ_SIZE(12, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_12) {
  uint8_t buf[12];
  for (int i = 0; i < 12; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 12);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 13);
  ASSERT_EQ_SIZE(13, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_13) {
  uint8_t buf[13];
  for (int i = 0; i < 13; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 13);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 14);
  ASSERT_EQ_SIZE(14, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_14) {
  uint8_t buf[14];
  for (int i = 0; i < 14; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 14);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 15);
  ASSERT_EQ_SIZE(15, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_15) {
  uint8_t buf[15];
  for (int i = 0; i < 15; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 15);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 16);
  ASSERT_EQ_SIZE(16, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_16) {
  uint8_t buf[16];
  for (int i = 0; i < 16; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 16);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 17);
  ASSERT_EQ_SIZE(17, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_17) {
  uint8_t buf[17];
  for (int i = 0; i < 17; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 17);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 18);
  ASSERT_EQ_SIZE(18, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_18) {
  uint8_t buf[18];
  for (int i = 0; i < 18; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 18);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 19);
  ASSERT_EQ_SIZE(19, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_19) {
  uint8_t buf[19];
  for (int i = 0; i < 19; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 19);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 20);
  ASSERT_EQ_SIZE(20, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_20) {
  uint8_t buf[20];
  for (int i = 0; i < 20; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 20);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 21);
  ASSERT_EQ_SIZE(21, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_21) {
  uint8_t buf[21];
  for (int i = 0; i < 21; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 21);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 22);
  ASSERT_EQ_SIZE(22, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_22) {
  uint8_t buf[22];
  for (int i = 0; i < 22; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 22);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 23);
  ASSERT_EQ_SIZE(23, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_23) {
  uint8_t buf[23];
  for (int i = 0; i < 23; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 23);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 24);
  ASSERT_EQ_SIZE(24, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_24) {
  uint8_t buf[24];
  for (int i = 0; i < 24; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 24);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 25);
  ASSERT_EQ_SIZE(25, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_25) {
  uint8_t buf[25];
  for (int i = 0; i < 25; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 25);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 26);
  ASSERT_EQ_SIZE(26, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_26) {
  uint8_t buf[26];
  for (int i = 0; i < 26; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 26);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 27);
  ASSERT_EQ_SIZE(27, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_27) {
  uint8_t buf[27];
  for (int i = 0; i < 27; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 27);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 28);
  ASSERT_EQ_SIZE(28, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_28) {
  uint8_t buf[28];
  for (int i = 0; i < 28; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 28);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 29);
  ASSERT_EQ_SIZE(29, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_29) {
  uint8_t buf[29];
  for (int i = 0; i < 29; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 29);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 30);
  ASSERT_EQ_SIZE(30, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_30) {
  uint8_t buf[30];
  for (int i = 0; i < 30; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 30);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 31);
  ASSERT_EQ_SIZE(31, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_31) {
  uint8_t buf[31];
  for (int i = 0; i < 31; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 31);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 32);
  ASSERT_EQ_SIZE(32, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_32) {
  uint8_t buf[32];
  for (int i = 0; i < 32; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 32);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 33);
  ASSERT_EQ_SIZE(33, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_33) {
  uint8_t buf[33];
  for (int i = 0; i < 33; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 33);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 34);
  ASSERT_EQ_SIZE(34, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_34) {
  uint8_t buf[34];
  for (int i = 0; i < 34; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 34);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 35);
  ASSERT_EQ_SIZE(35, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_35) {
  uint8_t buf[35];
  for (int i = 0; i < 35; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 35);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 36);
  ASSERT_EQ_SIZE(36, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_36) {
  uint8_t buf[36];
  for (int i = 0; i < 36; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 36);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 37);
  ASSERT_EQ_SIZE(37, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_37) {
  uint8_t buf[37];
  for (int i = 0; i < 37; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 37);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 38);
  ASSERT_EQ_SIZE(38, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_38) {
  uint8_t buf[38];
  for (int i = 0; i < 38; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 38);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 39);
  ASSERT_EQ_SIZE(39, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_39) {
  uint8_t buf[39];
  for (int i = 0; i < 39; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 39);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 40);
  ASSERT_EQ_SIZE(40, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_40) {
  uint8_t buf[40];
  for (int i = 0; i < 40; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 40);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 41);
  ASSERT_EQ_SIZE(41, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_41) {
  uint8_t buf[41];
  for (int i = 0; i < 41; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 41);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 42);
  ASSERT_EQ_SIZE(42, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_42) {
  uint8_t buf[42];
  for (int i = 0; i < 42; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 42);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 43);
  ASSERT_EQ_SIZE(43, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_43) {
  uint8_t buf[43];
  for (int i = 0; i < 43; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 43);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 44);
  ASSERT_EQ_SIZE(44, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_44) {
  uint8_t buf[44];
  for (int i = 0; i < 44; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 44);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 45);
  ASSERT_EQ_SIZE(45, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_45) {
  uint8_t buf[45];
  for (int i = 0; i < 45; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 45);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 46);
  ASSERT_EQ_SIZE(46, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_46) {
  uint8_t buf[46];
  for (int i = 0; i < 46; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 46);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 47);
  ASSERT_EQ_SIZE(47, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_47) {
  uint8_t buf[47];
  for (int i = 0; i < 47; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 47);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 48);
  ASSERT_EQ_SIZE(48, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_48) {
  uint8_t buf[48];
  for (int i = 0; i < 48; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 48);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 49);
  ASSERT_EQ_SIZE(49, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_49) {
  uint8_t buf[49];
  for (int i = 0; i < 49; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 49);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 50);
  ASSERT_EQ_SIZE(50, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_50) {
  uint8_t buf[50];
  for (int i = 0; i < 50; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 50);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 51);
  ASSERT_EQ_SIZE(51, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_51) {
  uint8_t buf[51];
  for (int i = 0; i < 51; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 51);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 52);
  ASSERT_EQ_SIZE(52, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_52) {
  uint8_t buf[52];
  for (int i = 0; i < 52; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 52);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 53);
  ASSERT_EQ_SIZE(53, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_53) {
  uint8_t buf[53];
  for (int i = 0; i < 53; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 53);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 54);
  ASSERT_EQ_SIZE(54, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_54) {
  uint8_t buf[54];
  for (int i = 0; i < 54; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 54);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 55);
  ASSERT_EQ_SIZE(55, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_55) {
  uint8_t buf[55];
  for (int i = 0; i < 55; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 55);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 56);
  ASSERT_EQ_SIZE(56, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_56) {
  uint8_t buf[56];
  for (int i = 0; i < 56; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 56);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 57);
  ASSERT_EQ_SIZE(57, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_57) {
  uint8_t buf[57];
  for (int i = 0; i < 57; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 57);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 58);
  ASSERT_EQ_SIZE(58, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_58) {
  uint8_t buf[58];
  for (int i = 0; i < 58; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 58);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 59);
  ASSERT_EQ_SIZE(59, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_59) {
  uint8_t buf[59];
  for (int i = 0; i < 59; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 59);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 60);
  ASSERT_EQ_SIZE(60, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_60) {
  uint8_t buf[60];
  for (int i = 0; i < 60; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 60);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 61);
  ASSERT_EQ_SIZE(61, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_61) {
  uint8_t buf[61];
  for (int i = 0; i < 61; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 61);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 62);
  ASSERT_EQ_SIZE(62, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_62) {
  uint8_t buf[62];
  for (int i = 0; i < 62; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 62);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 63);
  ASSERT_EQ_SIZE(63, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_63) {
  uint8_t buf[63];
  for (int i = 0; i < 63; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 63);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_match_ascii_letters_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = 'A' + (i % 26);
  size_t matched = simd_match_ascii_letters(buf, 64);
  ASSERT_EQ_SIZE(64, matched);
  PASS();
}

TEST(simd_match_ascii_letters_none_len_64) {
  uint8_t buf[64];
  for (int i = 0; i < 64; i++)
    buf[i] = '0' + (i % 10);
  size_t matched = simd_match_ascii_letters(buf, 64);
  ASSERT_EQ_SIZE(0, matched);
  PASS();
}

TEST(simd_is_all_ascii_exhaustive) {
  uint8_t buf[256];
  for (int i = 0; i < 128; i++) {
    buf[0] = (uint8_t)i;
    ASSERT_TRUE(simd_is_all_ascii(buf, 1));
  }
  for (int i = 128; i < 256; i++) {
    buf[0] = (uint8_t)i;
    ASSERT_FALSE(simd_is_all_ascii(buf, 1));
  }
  PASS();
}

TEST(simd_argmin_u32_exhaustive) {
  uint32_t arr[16];
  uint32_t out_min;
  for (int min_pos = 0; min_pos < 16; min_pos++) {
    for (int i = 0; i < 16; i++)
      arr[i] = 100;
    arr[min_pos] = 1;
    size_t result = simd_argmin_u32(arr, 16, &out_min);
    ASSERT_EQ_SIZE((size_t)min_pos, result);
    ASSERT_EQ_INT(1, (int)out_min);
  }
  PASS();
}

void run_simd_generated_tests(void) {
  TEST_SUITE("SIMD Generated Tests");
  RUN_TEST(simd_hash_len_0);
  RUN_TEST(simd_hash_len_1);
  RUN_TEST(simd_hash_len_2);
  RUN_TEST(simd_hash_len_3);
  RUN_TEST(simd_hash_len_4);
  RUN_TEST(simd_hash_len_5);
  RUN_TEST(simd_hash_len_6);
  RUN_TEST(simd_hash_len_7);
  RUN_TEST(simd_hash_len_8);
  RUN_TEST(simd_hash_len_9);
  RUN_TEST(simd_hash_len_10);
  RUN_TEST(simd_hash_len_11);
  RUN_TEST(simd_hash_len_12);
  RUN_TEST(simd_hash_len_13);
  RUN_TEST(simd_hash_len_14);
  RUN_TEST(simd_hash_len_15);
  RUN_TEST(simd_hash_len_16);
  RUN_TEST(simd_hash_len_17);
  RUN_TEST(simd_hash_len_18);
  RUN_TEST(simd_hash_len_19);
  RUN_TEST(simd_hash_len_20);
  RUN_TEST(simd_hash_len_21);
  RUN_TEST(simd_hash_len_22);
  RUN_TEST(simd_hash_len_23);
  RUN_TEST(simd_hash_len_24);
  RUN_TEST(simd_hash_len_25);
  RUN_TEST(simd_hash_len_26);
  RUN_TEST(simd_hash_len_27);
  RUN_TEST(simd_hash_len_28);
  RUN_TEST(simd_hash_len_29);
  RUN_TEST(simd_hash_len_30);
  RUN_TEST(simd_hash_len_31);
  RUN_TEST(simd_hash_len_32);
  RUN_TEST(simd_hash_len_33);
  RUN_TEST(simd_hash_len_34);
  RUN_TEST(simd_hash_len_35);
  RUN_TEST(simd_hash_len_36);
  RUN_TEST(simd_hash_len_37);
  RUN_TEST(simd_hash_len_38);
  RUN_TEST(simd_hash_len_39);
  RUN_TEST(simd_hash_len_40);
  RUN_TEST(simd_hash_len_41);
  RUN_TEST(simd_hash_len_42);
  RUN_TEST(simd_hash_len_43);
  RUN_TEST(simd_hash_len_44);
  RUN_TEST(simd_hash_len_45);
  RUN_TEST(simd_hash_len_46);
  RUN_TEST(simd_hash_len_47);
  RUN_TEST(simd_hash_len_48);
  RUN_TEST(simd_hash_len_49);
  RUN_TEST(simd_hash_len_50);
  RUN_TEST(simd_hash_len_51);
  RUN_TEST(simd_hash_len_52);
  RUN_TEST(simd_hash_len_53);
  RUN_TEST(simd_hash_len_54);
  RUN_TEST(simd_hash_len_55);
  RUN_TEST(simd_hash_len_56);
  RUN_TEST(simd_hash_len_57);
  RUN_TEST(simd_hash_len_58);
  RUN_TEST(simd_hash_len_59);
  RUN_TEST(simd_hash_len_60);
  RUN_TEST(simd_hash_len_61);
  RUN_TEST(simd_hash_len_62);
  RUN_TEST(simd_hash_len_63);
  RUN_TEST(simd_hash_len_64);
  RUN_TEST(simd_hash_len_65);
  RUN_TEST(simd_hash_len_66);
  RUN_TEST(simd_hash_len_67);
  RUN_TEST(simd_hash_len_68);
  RUN_TEST(simd_hash_len_69);
  RUN_TEST(simd_hash_len_70);
  RUN_TEST(simd_hash_len_71);
  RUN_TEST(simd_hash_len_72);
  RUN_TEST(simd_hash_len_73);
  RUN_TEST(simd_hash_len_74);
  RUN_TEST(simd_hash_len_75);
  RUN_TEST(simd_hash_len_76);
  RUN_TEST(simd_hash_len_77);
  RUN_TEST(simd_hash_len_78);
  RUN_TEST(simd_hash_len_79);
  RUN_TEST(simd_hash_len_80);
  RUN_TEST(simd_hash_len_81);
  RUN_TEST(simd_hash_len_82);
  RUN_TEST(simd_hash_len_83);
  RUN_TEST(simd_hash_len_84);
  RUN_TEST(simd_hash_len_85);
  RUN_TEST(simd_hash_len_86);
  RUN_TEST(simd_hash_len_87);
  RUN_TEST(simd_hash_len_88);
  RUN_TEST(simd_hash_len_89);
  RUN_TEST(simd_hash_len_90);
  RUN_TEST(simd_hash_len_91);
  RUN_TEST(simd_hash_len_92);
  RUN_TEST(simd_hash_len_93);
  RUN_TEST(simd_hash_len_94);
  RUN_TEST(simd_hash_len_95);
  RUN_TEST(simd_hash_len_96);
  RUN_TEST(simd_hash_len_97);
  RUN_TEST(simd_hash_len_98);
  RUN_TEST(simd_hash_len_99);
  RUN_TEST(simd_hash_len_100);
  RUN_TEST(simd_hash_len_101);
  RUN_TEST(simd_hash_len_102);
  RUN_TEST(simd_hash_len_103);
  RUN_TEST(simd_hash_len_104);
  RUN_TEST(simd_hash_len_105);
  RUN_TEST(simd_hash_len_106);
  RUN_TEST(simd_hash_len_107);
  RUN_TEST(simd_hash_len_108);
  RUN_TEST(simd_hash_len_109);
  RUN_TEST(simd_hash_len_110);
  RUN_TEST(simd_hash_len_111);
  RUN_TEST(simd_hash_len_112);
  RUN_TEST(simd_hash_len_113);
  RUN_TEST(simd_hash_len_114);
  RUN_TEST(simd_hash_len_115);
  RUN_TEST(simd_hash_len_116);
  RUN_TEST(simd_hash_len_117);
  RUN_TEST(simd_hash_len_118);
  RUN_TEST(simd_hash_len_119);
  RUN_TEST(simd_hash_len_120);
  RUN_TEST(simd_hash_len_121);
  RUN_TEST(simd_hash_len_122);
  RUN_TEST(simd_hash_len_123);
  RUN_TEST(simd_hash_len_124);
  RUN_TEST(simd_hash_len_125);
  RUN_TEST(simd_hash_len_126);
  RUN_TEST(simd_hash_len_127);
  RUN_TEST(simd_hash_len_128);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_1);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_1);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_1);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_2);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_2);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_2);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_3);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_3);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_3);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_4);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_4);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_4);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_5);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_5);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_5);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_6);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_6);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_6);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_7);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_7);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_7);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_8);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_8);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_8);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_9);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_9);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_9);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_10);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_10);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_10);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_11);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_11);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_11);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_12);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_12);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_12);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_13);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_13);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_13);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_14);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_14);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_14);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_15);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_15);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_15);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_16);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_16);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_16);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_17);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_17);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_17);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_18);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_18);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_18);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_19);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_19);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_19);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_20);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_20);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_20);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_21);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_21);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_21);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_22);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_22);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_22);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_23);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_23);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_23);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_24);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_24);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_24);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_25);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_25);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_25);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_26);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_26);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_26);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_27);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_27);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_27);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_28);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_28);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_28);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_29);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_29);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_29);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_30);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_30);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_30);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_31);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_31);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_31);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_32);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_32);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_32);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_33);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_33);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_33);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_34);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_34);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_34);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_35);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_35);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_35);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_36);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_36);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_36);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_37);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_37);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_37);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_38);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_38);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_38);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_39);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_39);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_39);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_40);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_40);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_40);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_41);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_41);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_41);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_42);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_42);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_42);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_43);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_43);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_43);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_44);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_44);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_44);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_45);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_45);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_45);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_46);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_46);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_46);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_47);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_47);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_47);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_48);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_48);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_48);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_49);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_49);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_49);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_50);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_50);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_50);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_51);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_51);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_51);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_52);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_52);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_52);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_53);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_53);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_53);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_54);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_54);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_54);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_55);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_55);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_55);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_56);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_56);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_56);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_57);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_57);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_57);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_58);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_58);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_58);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_59);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_59);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_59);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_60);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_60);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_60);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_61);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_61);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_61);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_62);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_62);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_62);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_63);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_63);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_63);
  RUN_TEST(simd_find_non_ascii_all_ascii_len_64);
  RUN_TEST(simd_find_non_ascii_first_non_ascii_len_64);
  RUN_TEST(simd_find_non_ascii_last_non_ascii_len_64);
  RUN_TEST(simd_count_utf8_chars_ascii_len_0);
  RUN_TEST(simd_count_utf8_chars_ascii_len_1);
  RUN_TEST(simd_count_utf8_chars_ascii_len_2);
  RUN_TEST(simd_count_utf8_chars_ascii_len_3);
  RUN_TEST(simd_count_utf8_chars_ascii_len_4);
  RUN_TEST(simd_count_utf8_chars_ascii_len_5);
  RUN_TEST(simd_count_utf8_chars_ascii_len_6);
  RUN_TEST(simd_count_utf8_chars_ascii_len_7);
  RUN_TEST(simd_count_utf8_chars_ascii_len_8);
  RUN_TEST(simd_count_utf8_chars_ascii_len_9);
  RUN_TEST(simd_count_utf8_chars_ascii_len_10);
  RUN_TEST(simd_count_utf8_chars_ascii_len_11);
  RUN_TEST(simd_count_utf8_chars_ascii_len_12);
  RUN_TEST(simd_count_utf8_chars_ascii_len_13);
  RUN_TEST(simd_count_utf8_chars_ascii_len_14);
  RUN_TEST(simd_count_utf8_chars_ascii_len_15);
  RUN_TEST(simd_count_utf8_chars_ascii_len_16);
  RUN_TEST(simd_count_utf8_chars_ascii_len_17);
  RUN_TEST(simd_count_utf8_chars_ascii_len_18);
  RUN_TEST(simd_count_utf8_chars_ascii_len_19);
  RUN_TEST(simd_count_utf8_chars_ascii_len_20);
  RUN_TEST(simd_count_utf8_chars_ascii_len_21);
  RUN_TEST(simd_count_utf8_chars_ascii_len_22);
  RUN_TEST(simd_count_utf8_chars_ascii_len_23);
  RUN_TEST(simd_count_utf8_chars_ascii_len_24);
  RUN_TEST(simd_count_utf8_chars_ascii_len_25);
  RUN_TEST(simd_count_utf8_chars_ascii_len_26);
  RUN_TEST(simd_count_utf8_chars_ascii_len_27);
  RUN_TEST(simd_count_utf8_chars_ascii_len_28);
  RUN_TEST(simd_count_utf8_chars_ascii_len_29);
  RUN_TEST(simd_count_utf8_chars_ascii_len_30);
  RUN_TEST(simd_count_utf8_chars_ascii_len_31);
  RUN_TEST(simd_count_utf8_chars_ascii_len_32);
  RUN_TEST(simd_count_utf8_chars_ascii_len_33);
  RUN_TEST(simd_count_utf8_chars_ascii_len_34);
  RUN_TEST(simd_count_utf8_chars_ascii_len_35);
  RUN_TEST(simd_count_utf8_chars_ascii_len_36);
  RUN_TEST(simd_count_utf8_chars_ascii_len_37);
  RUN_TEST(simd_count_utf8_chars_ascii_len_38);
  RUN_TEST(simd_count_utf8_chars_ascii_len_39);
  RUN_TEST(simd_count_utf8_chars_ascii_len_40);
  RUN_TEST(simd_count_utf8_chars_ascii_len_41);
  RUN_TEST(simd_count_utf8_chars_ascii_len_42);
  RUN_TEST(simd_count_utf8_chars_ascii_len_43);
  RUN_TEST(simd_count_utf8_chars_ascii_len_44);
  RUN_TEST(simd_count_utf8_chars_ascii_len_45);
  RUN_TEST(simd_count_utf8_chars_ascii_len_46);
  RUN_TEST(simd_count_utf8_chars_ascii_len_47);
  RUN_TEST(simd_count_utf8_chars_ascii_len_48);
  RUN_TEST(simd_count_utf8_chars_ascii_len_49);
  RUN_TEST(simd_count_utf8_chars_ascii_len_50);
  RUN_TEST(simd_count_utf8_chars_ascii_len_51);
  RUN_TEST(simd_count_utf8_chars_ascii_len_52);
  RUN_TEST(simd_count_utf8_chars_ascii_len_53);
  RUN_TEST(simd_count_utf8_chars_ascii_len_54);
  RUN_TEST(simd_count_utf8_chars_ascii_len_55);
  RUN_TEST(simd_count_utf8_chars_ascii_len_56);
  RUN_TEST(simd_count_utf8_chars_ascii_len_57);
  RUN_TEST(simd_count_utf8_chars_ascii_len_58);
  RUN_TEST(simd_count_utf8_chars_ascii_len_59);
  RUN_TEST(simd_count_utf8_chars_ascii_len_60);
  RUN_TEST(simd_count_utf8_chars_ascii_len_61);
  RUN_TEST(simd_count_utf8_chars_ascii_len_62);
  RUN_TEST(simd_count_utf8_chars_ascii_len_63);
  RUN_TEST(simd_count_utf8_chars_ascii_len_64);
  RUN_TEST(simd_match_ascii_letters_len_0);
  RUN_TEST(simd_match_ascii_letters_none_len_0);
  RUN_TEST(simd_match_ascii_letters_len_1);
  RUN_TEST(simd_match_ascii_letters_none_len_1);
  RUN_TEST(simd_match_ascii_letters_len_2);
  RUN_TEST(simd_match_ascii_letters_none_len_2);
  RUN_TEST(simd_match_ascii_letters_len_3);
  RUN_TEST(simd_match_ascii_letters_none_len_3);
  RUN_TEST(simd_match_ascii_letters_len_4);
  RUN_TEST(simd_match_ascii_letters_none_len_4);
  RUN_TEST(simd_match_ascii_letters_len_5);
  RUN_TEST(simd_match_ascii_letters_none_len_5);
  RUN_TEST(simd_match_ascii_letters_len_6);
  RUN_TEST(simd_match_ascii_letters_none_len_6);
  RUN_TEST(simd_match_ascii_letters_len_7);
  RUN_TEST(simd_match_ascii_letters_none_len_7);
  RUN_TEST(simd_match_ascii_letters_len_8);
  RUN_TEST(simd_match_ascii_letters_none_len_8);
  RUN_TEST(simd_match_ascii_letters_len_9);
  RUN_TEST(simd_match_ascii_letters_none_len_9);
  RUN_TEST(simd_match_ascii_letters_len_10);
  RUN_TEST(simd_match_ascii_letters_none_len_10);
  RUN_TEST(simd_match_ascii_letters_len_11);
  RUN_TEST(simd_match_ascii_letters_none_len_11);
  RUN_TEST(simd_match_ascii_letters_len_12);
  RUN_TEST(simd_match_ascii_letters_none_len_12);
  RUN_TEST(simd_match_ascii_letters_len_13);
  RUN_TEST(simd_match_ascii_letters_none_len_13);
  RUN_TEST(simd_match_ascii_letters_len_14);
  RUN_TEST(simd_match_ascii_letters_none_len_14);
  RUN_TEST(simd_match_ascii_letters_len_15);
  RUN_TEST(simd_match_ascii_letters_none_len_15);
  RUN_TEST(simd_match_ascii_letters_len_16);
  RUN_TEST(simd_match_ascii_letters_none_len_16);
  RUN_TEST(simd_match_ascii_letters_len_17);
  RUN_TEST(simd_match_ascii_letters_none_len_17);
  RUN_TEST(simd_match_ascii_letters_len_18);
  RUN_TEST(simd_match_ascii_letters_none_len_18);
  RUN_TEST(simd_match_ascii_letters_len_19);
  RUN_TEST(simd_match_ascii_letters_none_len_19);
  RUN_TEST(simd_match_ascii_letters_len_20);
  RUN_TEST(simd_match_ascii_letters_none_len_20);
  RUN_TEST(simd_match_ascii_letters_len_21);
  RUN_TEST(simd_match_ascii_letters_none_len_21);
  RUN_TEST(simd_match_ascii_letters_len_22);
  RUN_TEST(simd_match_ascii_letters_none_len_22);
  RUN_TEST(simd_match_ascii_letters_len_23);
  RUN_TEST(simd_match_ascii_letters_none_len_23);
  RUN_TEST(simd_match_ascii_letters_len_24);
  RUN_TEST(simd_match_ascii_letters_none_len_24);
  RUN_TEST(simd_match_ascii_letters_len_25);
  RUN_TEST(simd_match_ascii_letters_none_len_25);
  RUN_TEST(simd_match_ascii_letters_len_26);
  RUN_TEST(simd_match_ascii_letters_none_len_26);
  RUN_TEST(simd_match_ascii_letters_len_27);
  RUN_TEST(simd_match_ascii_letters_none_len_27);
  RUN_TEST(simd_match_ascii_letters_len_28);
  RUN_TEST(simd_match_ascii_letters_none_len_28);
  RUN_TEST(simd_match_ascii_letters_len_29);
  RUN_TEST(simd_match_ascii_letters_none_len_29);
  RUN_TEST(simd_match_ascii_letters_len_30);
  RUN_TEST(simd_match_ascii_letters_none_len_30);
  RUN_TEST(simd_match_ascii_letters_len_31);
  RUN_TEST(simd_match_ascii_letters_none_len_31);
  RUN_TEST(simd_match_ascii_letters_len_32);
  RUN_TEST(simd_match_ascii_letters_none_len_32);
  RUN_TEST(simd_match_ascii_letters_len_33);
  RUN_TEST(simd_match_ascii_letters_none_len_33);
  RUN_TEST(simd_match_ascii_letters_len_34);
  RUN_TEST(simd_match_ascii_letters_none_len_34);
  RUN_TEST(simd_match_ascii_letters_len_35);
  RUN_TEST(simd_match_ascii_letters_none_len_35);
  RUN_TEST(simd_match_ascii_letters_len_36);
  RUN_TEST(simd_match_ascii_letters_none_len_36);
  RUN_TEST(simd_match_ascii_letters_len_37);
  RUN_TEST(simd_match_ascii_letters_none_len_37);
  RUN_TEST(simd_match_ascii_letters_len_38);
  RUN_TEST(simd_match_ascii_letters_none_len_38);
  RUN_TEST(simd_match_ascii_letters_len_39);
  RUN_TEST(simd_match_ascii_letters_none_len_39);
  RUN_TEST(simd_match_ascii_letters_len_40);
  RUN_TEST(simd_match_ascii_letters_none_len_40);
  RUN_TEST(simd_match_ascii_letters_len_41);
  RUN_TEST(simd_match_ascii_letters_none_len_41);
  RUN_TEST(simd_match_ascii_letters_len_42);
  RUN_TEST(simd_match_ascii_letters_none_len_42);
  RUN_TEST(simd_match_ascii_letters_len_43);
  RUN_TEST(simd_match_ascii_letters_none_len_43);
  RUN_TEST(simd_match_ascii_letters_len_44);
  RUN_TEST(simd_match_ascii_letters_none_len_44);
  RUN_TEST(simd_match_ascii_letters_len_45);
  RUN_TEST(simd_match_ascii_letters_none_len_45);
  RUN_TEST(simd_match_ascii_letters_len_46);
  RUN_TEST(simd_match_ascii_letters_none_len_46);
  RUN_TEST(simd_match_ascii_letters_len_47);
  RUN_TEST(simd_match_ascii_letters_none_len_47);
  RUN_TEST(simd_match_ascii_letters_len_48);
  RUN_TEST(simd_match_ascii_letters_none_len_48);
  RUN_TEST(simd_match_ascii_letters_len_49);
  RUN_TEST(simd_match_ascii_letters_none_len_49);
  RUN_TEST(simd_match_ascii_letters_len_50);
  RUN_TEST(simd_match_ascii_letters_none_len_50);
  RUN_TEST(simd_match_ascii_letters_len_51);
  RUN_TEST(simd_match_ascii_letters_none_len_51);
  RUN_TEST(simd_match_ascii_letters_len_52);
  RUN_TEST(simd_match_ascii_letters_none_len_52);
  RUN_TEST(simd_match_ascii_letters_len_53);
  RUN_TEST(simd_match_ascii_letters_none_len_53);
  RUN_TEST(simd_match_ascii_letters_len_54);
  RUN_TEST(simd_match_ascii_letters_none_len_54);
  RUN_TEST(simd_match_ascii_letters_len_55);
  RUN_TEST(simd_match_ascii_letters_none_len_55);
  RUN_TEST(simd_match_ascii_letters_len_56);
  RUN_TEST(simd_match_ascii_letters_none_len_56);
  RUN_TEST(simd_match_ascii_letters_len_57);
  RUN_TEST(simd_match_ascii_letters_none_len_57);
  RUN_TEST(simd_match_ascii_letters_len_58);
  RUN_TEST(simd_match_ascii_letters_none_len_58);
  RUN_TEST(simd_match_ascii_letters_len_59);
  RUN_TEST(simd_match_ascii_letters_none_len_59);
  RUN_TEST(simd_match_ascii_letters_len_60);
  RUN_TEST(simd_match_ascii_letters_none_len_60);
  RUN_TEST(simd_match_ascii_letters_len_61);
  RUN_TEST(simd_match_ascii_letters_none_len_61);
  RUN_TEST(simd_match_ascii_letters_len_62);
  RUN_TEST(simd_match_ascii_letters_none_len_62);
  RUN_TEST(simd_match_ascii_letters_len_63);
  RUN_TEST(simd_match_ascii_letters_none_len_63);
  RUN_TEST(simd_match_ascii_letters_len_64);
  RUN_TEST(simd_match_ascii_letters_none_len_64);
  RUN_TEST(simd_is_all_ascii_exhaustive);
  RUN_TEST(simd_argmin_u32_exhaustive);
}

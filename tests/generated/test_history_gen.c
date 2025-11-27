#include "chat/history.h"
#include "test_framework.h"
#include <stdlib.h>
#include <string.h>

TEST(history_add_1_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 1; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(1, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_2_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 2; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(2, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_3_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 3; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(3, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_4_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 4; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(4, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_5_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 5; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(5, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_6_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 6; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(6, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_7_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 7; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(7, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_8_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 8; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(8, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_9_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 9; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(9, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_10_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 10; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(10, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_11_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 11; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(11, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_12_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 12; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(12, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_13_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 13; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(13, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_14_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 14; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(14, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_15_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 15; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(15, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_16_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 16; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(16, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_17_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 17; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(17, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_18_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 18; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(18, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_19_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 19; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(19, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_20_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 20; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(20, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_21_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 21; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(21, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_22_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 22; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(22, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_23_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 23; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(23, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_24_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 24; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(24, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_25_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 25; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(25, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_26_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 26; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(26, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_27_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 27; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(27, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_28_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 28; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(28, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_29_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 29; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(29, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_30_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 30; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(30, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_31_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 31; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(31, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_32_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 32; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(32, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_33_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 33; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(33, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_34_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 34; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(34, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_35_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 35; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(35, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_36_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 36; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(36, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_37_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 37; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(37, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_38_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 38; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(38, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_39_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 39; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(39, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_40_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 40; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(40, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_41_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 41; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(41, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_42_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 42; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(42, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_43_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 43; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(43, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_44_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 44; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(44, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_45_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 45; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(45, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_46_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 46; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(46, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_47_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 47; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(47, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_48_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 48; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(48, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_49_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 49; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(49, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_50_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 50; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(50, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_51_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 51; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(51, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_52_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 52; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(52, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_53_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 53; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(53, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_54_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 54; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(54, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_55_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 55; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(55, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_56_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 56; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(56, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_57_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 57; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(57, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_58_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 58; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(58, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_59_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 59; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(59, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_60_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 60; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(60, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_61_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 61; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(61, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_62_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 62; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(62, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_63_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 63; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(63, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_64_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 64; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(64, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_65_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 65; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(65, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_66_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 66; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(66, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_67_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 67; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(67, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_68_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 68; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(68, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_69_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 69; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(69, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_70_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 70; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(70, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_71_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 71; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(71, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_72_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 72; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(72, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_73_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 73; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(73, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_74_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 74; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(74, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_75_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 75; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(75, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_76_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 76; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(76, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_77_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 77; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(77, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_78_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 78; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(78, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_79_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 79; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(79, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_80_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 80; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(80, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_81_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 81; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(81, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_82_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 82; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(82, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_83_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 83; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(83, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_84_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 84; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(84, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_85_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 85; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(85, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_86_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 86; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(86, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_87_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 87; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(87, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_88_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 88; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(88, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_89_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 89; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(89, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_90_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 90; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(90, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_91_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 91; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(91, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_92_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 92; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(92, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_93_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 93; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(93, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_94_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 94; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(94, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_95_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 95; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(95, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_96_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 96; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(96, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_97_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 97; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(97, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_98_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 98; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(98, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_99_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 99; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(99, h.count);
  history_free(&h);
  PASS();
}

TEST(history_add_100_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 100; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    size_t idx = history_add(&h, buf);
    ASSERT_EQ_SIZE((size_t)i, idx);
  }
  ASSERT_EQ_SIZE(100, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_1_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 1; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(1, h.count);
  for (size_t i = 0; i < 1; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(1 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_2_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 2; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(2, h.count);
  for (size_t i = 0; i < 2; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(2 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_3_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 3; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(3, h.count);
  for (size_t i = 0; i < 3; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(3 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_4_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 4; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(4, h.count);
  for (size_t i = 0; i < 4; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(4 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_5_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 5; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(5, h.count);
  for (size_t i = 0; i < 5; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(5 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_6_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 6; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(6, h.count);
  for (size_t i = 0; i < 6; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(6 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_7_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 7; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(7, h.count);
  for (size_t i = 0; i < 7; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(7 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_8_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 8; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(8, h.count);
  for (size_t i = 0; i < 8; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(8 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_9_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 9; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(9, h.count);
  for (size_t i = 0; i < 9; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(9 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_10_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 10; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(10, h.count);
  for (size_t i = 0; i < 10; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(10 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_11_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 11; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(11, h.count);
  for (size_t i = 0; i < 11; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(11 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_12_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 12; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(12, h.count);
  for (size_t i = 0; i < 12; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(12 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_13_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 13; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(13, h.count);
  for (size_t i = 0; i < 13; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(13 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_14_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 14; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(14, h.count);
  for (size_t i = 0; i < 14; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(14 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_15_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 15; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(15, h.count);
  for (size_t i = 0; i < 15; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(15 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_16_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 16; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(16, h.count);
  for (size_t i = 0; i < 16; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(16 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_17_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 17; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(17, h.count);
  for (size_t i = 0; i < 17; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(17 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_18_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 18; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(18, h.count);
  for (size_t i = 0; i < 18; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(18 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_19_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 19; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(19, h.count);
  for (size_t i = 0; i < 19; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(19 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_delete_from_20_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 20; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Message %d", i);
    history_add(&h, buf);
  }
  ASSERT_EQ_SIZE(20, h.count);
  for (size_t i = 0; i < 20; i++) {
    ASSERT_TRUE(history_delete(&h, 0));
    ASSERT_EQ_SIZE(20 - i - 1, h.count);
  }
  ASSERT_EQ_SIZE(0, h.count);
  history_free(&h);
  PASS();
}

TEST(history_swipes_1_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 1; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(2, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 1; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_2_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 2; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(3, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 2; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_3_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 3; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(4, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 3; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_4_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 4; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(5, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 4; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_5_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 5; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(6, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 5; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_6_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 6; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(7, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 6; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_7_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 7; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(8, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 7; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_8_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 8; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(9, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 8; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_9_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 9; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(10, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 9; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_10_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 10; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(11, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 10; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_11_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 11; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(12, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 11; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_12_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 12; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(13, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 12; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_13_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 13; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(14, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 13; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_14_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 14; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(15, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 14; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_15_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 15; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(16, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 15; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_swipes_16_per_message) {
  ChatHistory h;
  history_init(&h);
  size_t idx = history_add(&h, "Original");
  for (int s = 0; s < 16; s++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Swipe %d", s);
    history_add_swipe(&h, idx, buf);
  }
  ASSERT_EQ_SIZE(17, history_get_swipe_count(&h, idx));
  for (int s = 0; s <= 16; s++) {
    ASSERT_TRUE(history_set_active_swipe(&h, idx, s));
    ASSERT_EQ_SIZE((size_t)s, history_get_active_swipe(&h, idx));
  }
  history_free(&h);
  PASS();
}

TEST(history_update_all_messages) {
  ChatHistory h;
  history_init(&h);
  for (int i = 0; i < 50; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Original %d", i);
    history_add(&h, buf);
  }
  for (int i = 0; i < 50; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Updated %d", i);
    history_update(&h, (size_t)i, buf);
  }
  for (int i = 0; i < 50; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Updated %d", i);
    const char *msg = history_get(&h, (size_t)i);
    ASSERT_NOT_NULL(msg);
    ASSERT_EQ_INT(0, strcmp(msg, buf));
  }
  history_free(&h);
  PASS();
}

TEST(history_mixed_operations) {
  ChatHistory h;
  history_init(&h);
  for (int iter = 0; iter < 100; iter++) {
    size_t idx = history_add(&h, "New");
    history_update(&h, idx, "Updated");
    if (idx > 0) {
      history_add_swipe(&h, 0, "Swipe");
    }
    if (h.count > 5) {
      history_delete(&h, h.count / 2);
    }
  }
  history_free(&h);
  PASS();
}

void run_history_generated_tests(void) {
  TEST_SUITE("History Generated Tests");
  RUN_TEST(history_add_1_messages);
  RUN_TEST(history_add_2_messages);
  RUN_TEST(history_add_3_messages);
  RUN_TEST(history_add_4_messages);
  RUN_TEST(history_add_5_messages);
  RUN_TEST(history_add_6_messages);
  RUN_TEST(history_add_7_messages);
  RUN_TEST(history_add_8_messages);
  RUN_TEST(history_add_9_messages);
  RUN_TEST(history_add_10_messages);
  RUN_TEST(history_add_11_messages);
  RUN_TEST(history_add_12_messages);
  RUN_TEST(history_add_13_messages);
  RUN_TEST(history_add_14_messages);
  RUN_TEST(history_add_15_messages);
  RUN_TEST(history_add_16_messages);
  RUN_TEST(history_add_17_messages);
  RUN_TEST(history_add_18_messages);
  RUN_TEST(history_add_19_messages);
  RUN_TEST(history_add_20_messages);
  RUN_TEST(history_add_21_messages);
  RUN_TEST(history_add_22_messages);
  RUN_TEST(history_add_23_messages);
  RUN_TEST(history_add_24_messages);
  RUN_TEST(history_add_25_messages);
  RUN_TEST(history_add_26_messages);
  RUN_TEST(history_add_27_messages);
  RUN_TEST(history_add_28_messages);
  RUN_TEST(history_add_29_messages);
  RUN_TEST(history_add_30_messages);
  RUN_TEST(history_add_31_messages);
  RUN_TEST(history_add_32_messages);
  RUN_TEST(history_add_33_messages);
  RUN_TEST(history_add_34_messages);
  RUN_TEST(history_add_35_messages);
  RUN_TEST(history_add_36_messages);
  RUN_TEST(history_add_37_messages);
  RUN_TEST(history_add_38_messages);
  RUN_TEST(history_add_39_messages);
  RUN_TEST(history_add_40_messages);
  RUN_TEST(history_add_41_messages);
  RUN_TEST(history_add_42_messages);
  RUN_TEST(history_add_43_messages);
  RUN_TEST(history_add_44_messages);
  RUN_TEST(history_add_45_messages);
  RUN_TEST(history_add_46_messages);
  RUN_TEST(history_add_47_messages);
  RUN_TEST(history_add_48_messages);
  RUN_TEST(history_add_49_messages);
  RUN_TEST(history_add_50_messages);
  RUN_TEST(history_add_51_messages);
  RUN_TEST(history_add_52_messages);
  RUN_TEST(history_add_53_messages);
  RUN_TEST(history_add_54_messages);
  RUN_TEST(history_add_55_messages);
  RUN_TEST(history_add_56_messages);
  RUN_TEST(history_add_57_messages);
  RUN_TEST(history_add_58_messages);
  RUN_TEST(history_add_59_messages);
  RUN_TEST(history_add_60_messages);
  RUN_TEST(history_add_61_messages);
  RUN_TEST(history_add_62_messages);
  RUN_TEST(history_add_63_messages);
  RUN_TEST(history_add_64_messages);
  RUN_TEST(history_add_65_messages);
  RUN_TEST(history_add_66_messages);
  RUN_TEST(history_add_67_messages);
  RUN_TEST(history_add_68_messages);
  RUN_TEST(history_add_69_messages);
  RUN_TEST(history_add_70_messages);
  RUN_TEST(history_add_71_messages);
  RUN_TEST(history_add_72_messages);
  RUN_TEST(history_add_73_messages);
  RUN_TEST(history_add_74_messages);
  RUN_TEST(history_add_75_messages);
  RUN_TEST(history_add_76_messages);
  RUN_TEST(history_add_77_messages);
  RUN_TEST(history_add_78_messages);
  RUN_TEST(history_add_79_messages);
  RUN_TEST(history_add_80_messages);
  RUN_TEST(history_add_81_messages);
  RUN_TEST(history_add_82_messages);
  RUN_TEST(history_add_83_messages);
  RUN_TEST(history_add_84_messages);
  RUN_TEST(history_add_85_messages);
  RUN_TEST(history_add_86_messages);
  RUN_TEST(history_add_87_messages);
  RUN_TEST(history_add_88_messages);
  RUN_TEST(history_add_89_messages);
  RUN_TEST(history_add_90_messages);
  RUN_TEST(history_add_91_messages);
  RUN_TEST(history_add_92_messages);
  RUN_TEST(history_add_93_messages);
  RUN_TEST(history_add_94_messages);
  RUN_TEST(history_add_95_messages);
  RUN_TEST(history_add_96_messages);
  RUN_TEST(history_add_97_messages);
  RUN_TEST(history_add_98_messages);
  RUN_TEST(history_add_99_messages);
  RUN_TEST(history_add_100_messages);
  RUN_TEST(history_delete_from_1_messages);
  RUN_TEST(history_delete_from_2_messages);
  RUN_TEST(history_delete_from_3_messages);
  RUN_TEST(history_delete_from_4_messages);
  RUN_TEST(history_delete_from_5_messages);
  RUN_TEST(history_delete_from_6_messages);
  RUN_TEST(history_delete_from_7_messages);
  RUN_TEST(history_delete_from_8_messages);
  RUN_TEST(history_delete_from_9_messages);
  RUN_TEST(history_delete_from_10_messages);
  RUN_TEST(history_delete_from_11_messages);
  RUN_TEST(history_delete_from_12_messages);
  RUN_TEST(history_delete_from_13_messages);
  RUN_TEST(history_delete_from_14_messages);
  RUN_TEST(history_delete_from_15_messages);
  RUN_TEST(history_delete_from_16_messages);
  RUN_TEST(history_delete_from_17_messages);
  RUN_TEST(history_delete_from_18_messages);
  RUN_TEST(history_delete_from_19_messages);
  RUN_TEST(history_delete_from_20_messages);
  RUN_TEST(history_swipes_1_per_message);
  RUN_TEST(history_swipes_2_per_message);
  RUN_TEST(history_swipes_3_per_message);
  RUN_TEST(history_swipes_4_per_message);
  RUN_TEST(history_swipes_5_per_message);
  RUN_TEST(history_swipes_6_per_message);
  RUN_TEST(history_swipes_7_per_message);
  RUN_TEST(history_swipes_8_per_message);
  RUN_TEST(history_swipes_9_per_message);
  RUN_TEST(history_swipes_10_per_message);
  RUN_TEST(history_swipes_11_per_message);
  RUN_TEST(history_swipes_12_per_message);
  RUN_TEST(history_swipes_13_per_message);
  RUN_TEST(history_swipes_14_per_message);
  RUN_TEST(history_swipes_15_per_message);
  RUN_TEST(history_swipes_16_per_message);
  RUN_TEST(history_update_all_messages);
  RUN_TEST(history_mixed_operations);
}

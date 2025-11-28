#include "test_framework.h"
#include "tokenizer/tiktoken.h"
#include <stdlib.h>
#include <string.h>

TEST(pretokenize_0_empty_string) {
  SpanList spans = {0};
  const char *input = "";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_1_single_ascii_letter) {
  SpanList spans = {0};
  const char *input = "a";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_2_single_uppercase_letter) {
  SpanList spans = {0};
  const char *input = "A";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_3_single_lowercase_z) {
  SpanList spans = {0};
  const char *input = "z";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_4_single_digit) {
  SpanList spans = {0};
  const char *input = "0";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_5_single_digit_9) {
  SpanList spans = {0};
  const char *input = "9";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_6_single_space) {
  SpanList spans = {0};
  const char *input = " ";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_7_single_tab) {
  SpanList spans = {0};
  const char *input = "\t";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_8_single_newline) {
  SpanList spans = {0};
  const char *input = "\n";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_9_single_carriage_return) {
  SpanList spans = {0};
  const char *input = "\r";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_10_single_period) {
  SpanList spans = {0};
  const char *input = ".";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_11_single_comma) {
  SpanList spans = {0};
  const char *input = ",";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_12_single_exclamation) {
  SpanList spans = {0};
  const char *input = "!";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_13_single_question) {
  SpanList spans = {0};
  const char *input = "?";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_14_single_at_sign) {
  SpanList spans = {0};
  const char *input = "@";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_15_single_hash) {
  SpanList spans = {0};
  const char *input = "#";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_16_single_dollar) {
  SpanList spans = {0};
  const char *input = "$";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_17_single_percent) {
  SpanList spans = {0};
  const char *input = "%";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_18_simple_word) {
  SpanList spans = {0};
  const char *input = "hello";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_19_uppercase_word) {
  SpanList spans = {0};
  const char *input = "HELLO";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_20_mixed_case_word) {
  SpanList spans = {0};
  const char *input = "Hello";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_21_word_with_trailing_numbers) {
  SpanList spans = {0};
  const char *input = "world123";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_22_numbers_then_word) {
  SpanList spans = {0};
  const char *input = "123abc";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_23_5_digits) {
  SpanList spans = {0};
  const char *input = "12345";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_24_6_digits) {
  SpanList spans = {0};
  const char *input = "123456";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_25_7_digits) {
  SpanList spans = {0};
  const char *input = "1234567";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_26_9_digits) {
  SpanList spans = {0};
  const char *input = "123456789";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_27_two_words) {
  SpanList spans = {0};
  const char *input = "hello world";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_28_two_words_double_space) {
  SpanList spans = {0};
  const char *input = "hello  world";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_29_two_words_triple_space) {
  SpanList spans = {0};
  const char *input = "hello   world";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_30_leading_spaces) {
  SpanList spans = {0};
  const char *input = "  hello";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_31_trailing_spaces) {
  SpanList spans = {0};
  const char *input = "hello  ";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_32_leading_and_trailing_spaces) {
  SpanList spans = {0};
  const char *input = "  hello  ";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_33_contraction_s) {
  SpanList spans = {0};
  const char *input = "'s";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_34_contraction_t) {
  SpanList spans = {0};
  const char *input = "'t";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_35_contraction_m) {
  SpanList spans = {0};
  const char *input = "'m";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_36_contraction_d) {
  SpanList spans = {0};
  const char *input = "'d";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_37_contraction_ll) {
  SpanList spans = {0};
  const char *input = "'ll";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_38_contraction_ve) {
  SpanList spans = {0};
  const char *input = "'ve";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_39_contraction_re) {
  SpanList spans = {0};
  const char *input = "'re";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_40_uppercase_contraction_S) {
  SpanList spans = {0};
  const char *input = "'S";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_41_uppercase_contraction_LL) {
  SpanList spans = {0};
  const char *input = "'LL";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_42_word_with_contraction) {
  SpanList spans = {0};
  const char *input = "it's";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_43_word_with_contraction_t) {
  SpanList spans = {0};
  const char *input = "don't";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_44_I_am_contraction) {
  SpanList spans = {0};
  const char *input = "I'm";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_45_we_will_contraction) {
  SpanList spans = {0};
  const char *input = "we'll";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_46_they_have_contraction) {
  SpanList spans = {0};
  const char *input = "they've";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_47_you_are_contraction) {
  SpanList spans = {0};
  const char *input = "you're";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_48_word_then_period) {
  SpanList spans = {0};
  const char *input = "test.";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_49_word_then_comma) {
  SpanList spans = {0};
  const char *input = "test,";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_50_word_then_exclamation) {
  SpanList spans = {0};
  const char *input = "test!";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_51_word_then_question) {
  SpanList spans = {0};
  const char *input = "test?";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_52_word_then_ellipsis) {
  SpanList spans = {0};
  const char *input = "test...";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_53_word_then_multiple_questions) {
  SpanList spans = {0};
  const char *input = "test???";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_54_word_in_parens) {
  SpanList spans = {0};
  const char *input = "(test)";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_55_word_in_brackets) {
  SpanList spans = {0};
  const char *input = "[test]";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_56_word_in_braces) {
  SpanList spans = {0};
  const char *input = "{test}";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_57_word_in_angle_brackets) {
  SpanList spans = {0};
  const char *input = "<test>";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_58_dollar_amount) {
  SpanList spans = {0};
  const char *input = "$100";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_59_4_digit_dollar_amount) {
  SpanList spans = {0};
  const char *input = "$1000";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_60_dollar_with_comma) {
  SpanList spans = {0};
  const char *input = "$1,000";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_61_percentage) {
  SpanList spans = {0};
  const char *input = "100%";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_62_decimal_number) {
  SpanList spans = {0};
  const char *input = "3.14";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_63_fraction) {
  SpanList spans = {0};
  const char *input = "1/2";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_64_simple_math) {
  SpanList spans = {0};
  const char *input = "1+1";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_65_multiplication) {
  SpanList spans = {0};
  const char *input = "2*3";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_66_subtraction) {
  SpanList spans = {0};
  const char *input = "10-5";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_67_division) {
  SpanList spans = {0};
  const char *input = "10/2";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_68_space_then_plus) {
  SpanList spans = {0};
  const char *input = " +";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_69_space_then_minus) {
  SpanList spans = {0};
  const char *input = " -";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_70_space_then_asterisk) {
  SpanList spans = {0};
  const char *input = " *";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_71_space_then_slash) {
  SpanList spans = {0};
  const char *input = " /";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_72_space_then_at) {
  SpanList spans = {0};
  const char *input = " @";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_73_space_then_hash) {
  SpanList spans = {0};
  const char *input = " #";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_74_two_letters_with_newline) {
  SpanList spans = {0};
  const char *input = "a\nb";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_75_two_letters_with_double_newline) {
  SpanList spans = {0};
  const char *input = "a\n\nb";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_76_two_lines) {
  SpanList spans = {0};
  const char *input = "line1\nline2";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_77_crlf_line_ending) {
  SpanList spans = {0};
  const char *input = "line1\r\nline2";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 6);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_78_email_like) {
  SpanList spans = {0};
  const char *input = "test@example.com";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_79_url_like) {
  SpanList spans = {0};
  const char *input = "http://test";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_80_unix_path) {
  SpanList spans = {0};
  const char *input = "path/to/file";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_81_windows_path) {
  SpanList spans = {0};
  const char *input = "C:\\path\\file";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 4);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_82_hello_world_punctuated) {
  SpanList spans = {0};
  const char *input = "Hello, World!";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 4);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_83_pangram_start) {
  SpanList spans = {0};
  const char *input = "The quick brown fox";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 4);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_84_math_expression) {
  SpanList spans = {0};
  const char *input = "123 + 456 = 579";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 7);
  ASSERT(spans.count <= 9);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_85_function_call) {
  SpanList spans = {0};
  const char *input = "foo(bar)";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 2);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_86_function_with_args) {
  SpanList spans = {0};
  const char *input = "foo(a, b, c)";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 4);
  ASSERT(spans.count <= 8);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_87_assignment) {
  SpanList spans = {0};
  const char *input = "x = y + z;";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 6);
  ASSERT(spans.count <= 8);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_88_if_statement) {
  SpanList spans = {0};
  const char *input = "if (x) { y }";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 7);
  ASSERT(spans.count <= 9);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_89_single_cjk) {
  SpanList spans = {0};
  const char *input = "\xE6" "\x97" "\xA5";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_90_two_cjk) {
  SpanList spans = {0};
  const char *input = "\xE6" "\x97" "\xA5" "\xE6" "\x9C" "\xAC";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_91_three_cjk) {
  SpanList spans = {0};
  const char *input = "\xE6" "\x97" "\xA5" "\xE6" "\x9C" "\xAC" "\xE8" "\xAA" "\x9E";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_92_hiragana) {
  SpanList spans = {0};
  const char *input = "\xE3" "\x81" "\x93" "\xE3" "\x82" "\x93" "\xE3" "\x81" "\xAB" "\xE3" "\x81" "\xA1" "\xE3" "\x81" "\xAF";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_93_katakana) {
  SpanList spans = {0};
  const char *input = "\xE3" "\x82" "\xAB" "\xE3" "\x82" "\xBF" "\xE3" "\x82" "\xAB" "\xE3" "\x83" "\x8A";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_94_hangul) {
  SpanList spans = {0};
  const char *input = "\xED" "\x95" "\x9C" "\xEA" "\xB8" "\x80";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_95_cyrillic) {
  SpanList spans = {0};
  const char *input = "\xD0" "\x9F" "\xD1" "\x80" "\xD0" "\xB8" "\xD0" "\xB2" "\xD0" "\xB5" "\xD1" "\x82";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 1);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_96_arabic) {
  SpanList spans = {0};
  const char *input = "\xD9" "\x85" "\xD8" "\xB1" "\xD8" "\xAD" "\xD8" "\xA8" "\xD8" "\xA7";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_97_hebrew) {
  SpanList spans = {0};
  const char *input = "\xD7" "\xA9" "\xD7" "\x9C" "\xD7" "\x95" "\xD7" "\x9D";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_98_mixed_ascii_cjk) {
  SpanList spans = {0};
  const char *input = "Hello\xE6" "\x97" "\xA5" "\xE6" "\x9C" "\xAC";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_99_cjk_then_ascii) {
  SpanList spans = {0};
  const char *input = "\xE6" "\x97" "\xA5" "\xE6" "\x9C" "\xACHello";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 3);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_100_ascii_cjk_ascii) {
  SpanList spans = {0};
  const char *input = "test\xE6" "\x97" "\xA5" "\xE6" "\x9C" "\xACtest";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_101_mixed_whitespace_only) {
  SpanList spans = {0};
  const char *input = "   \t\t\n\r   ";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_102_letters_with_mixed_whitespace) {
  SpanList spans = {0};
  const char *input = "a   \t\t\n\r   b";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 3);
  ASSERT(spans.count <= 5);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_103_overlong_2_byte) {
  SpanList spans = {0};
  const char *input = "a\xC0" "\x80" "b";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 4);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_104_BOM_like_invalid) {
  SpanList spans = {0};
  const char *input = "\xFF" "\xFE";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 1);
  ASSERT(spans.count <= 2);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_105_full_pangram) {
  SpanList spans = {0};
  const char *input = "The quick brown fox jumps over the lazy dog";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 9);
  ASSERT(spans.count <= 9);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_106_another_pangram) {
  SpanList spans = {0};
  const char *input = "Pack my box with five dozen liquor jugs";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 8);
  ASSERT(spans.count <= 8);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

TEST(pretokenize_107_third_pangram) {
  SpanList spans = {0};
  const char *input = "How vexingly quick daft zebras jump!";
  int result = pretokenize_cl100k(input, &spans);
  ASSERT(result >= 0);
  ASSERT(spans.count >= 6);
  ASSERT(spans.count <= 7);
  for (size_t j = 0; j < spans.count; j++) {
    ASSERT(spans.spans[j].start < spans.spans[j].end);
    ASSERT(spans.spans[j].end <= strlen(input));
  }
  size_t covered = 0;
  for (size_t j = 0; j < spans.count; j++) {
    covered += spans.spans[j].end - spans.spans[j].start;
  }
  ASSERT(covered == strlen(input));
  free(spans.spans);
  PASS();
}

void run_pretokenize_generated_tests(void) {
  TEST_SUITE("Pre-tokenize Generated Tests");
  RUN_TEST(pretokenize_0_empty_string);
  RUN_TEST(pretokenize_1_single_ascii_letter);
  RUN_TEST(pretokenize_2_single_uppercase_letter);
  RUN_TEST(pretokenize_3_single_lowercase_z);
  RUN_TEST(pretokenize_4_single_digit);
  RUN_TEST(pretokenize_5_single_digit_9);
  RUN_TEST(pretokenize_6_single_space);
  RUN_TEST(pretokenize_7_single_tab);
  RUN_TEST(pretokenize_8_single_newline);
  RUN_TEST(pretokenize_9_single_carriage_return);
  RUN_TEST(pretokenize_10_single_period);
  RUN_TEST(pretokenize_11_single_comma);
  RUN_TEST(pretokenize_12_single_exclamation);
  RUN_TEST(pretokenize_13_single_question);
  RUN_TEST(pretokenize_14_single_at_sign);
  RUN_TEST(pretokenize_15_single_hash);
  RUN_TEST(pretokenize_16_single_dollar);
  RUN_TEST(pretokenize_17_single_percent);
  RUN_TEST(pretokenize_18_simple_word);
  RUN_TEST(pretokenize_19_uppercase_word);
  RUN_TEST(pretokenize_20_mixed_case_word);
  RUN_TEST(pretokenize_21_word_with_trailing_numbers);
  RUN_TEST(pretokenize_22_numbers_then_word);
  RUN_TEST(pretokenize_23_5_digits);
  RUN_TEST(pretokenize_24_6_digits);
  RUN_TEST(pretokenize_25_7_digits);
  RUN_TEST(pretokenize_26_9_digits);
  RUN_TEST(pretokenize_27_two_words);
  RUN_TEST(pretokenize_28_two_words_double_space);
  RUN_TEST(pretokenize_29_two_words_triple_space);
  RUN_TEST(pretokenize_30_leading_spaces);
  RUN_TEST(pretokenize_31_trailing_spaces);
  RUN_TEST(pretokenize_32_leading_and_trailing_spaces);
  RUN_TEST(pretokenize_33_contraction_s);
  RUN_TEST(pretokenize_34_contraction_t);
  RUN_TEST(pretokenize_35_contraction_m);
  RUN_TEST(pretokenize_36_contraction_d);
  RUN_TEST(pretokenize_37_contraction_ll);
  RUN_TEST(pretokenize_38_contraction_ve);
  RUN_TEST(pretokenize_39_contraction_re);
  RUN_TEST(pretokenize_40_uppercase_contraction_S);
  RUN_TEST(pretokenize_41_uppercase_contraction_LL);
  RUN_TEST(pretokenize_42_word_with_contraction);
  RUN_TEST(pretokenize_43_word_with_contraction_t);
  RUN_TEST(pretokenize_44_I_am_contraction);
  RUN_TEST(pretokenize_45_we_will_contraction);
  RUN_TEST(pretokenize_46_they_have_contraction);
  RUN_TEST(pretokenize_47_you_are_contraction);
  RUN_TEST(pretokenize_48_word_then_period);
  RUN_TEST(pretokenize_49_word_then_comma);
  RUN_TEST(pretokenize_50_word_then_exclamation);
  RUN_TEST(pretokenize_51_word_then_question);
  RUN_TEST(pretokenize_52_word_then_ellipsis);
  RUN_TEST(pretokenize_53_word_then_multiple_questions);
  RUN_TEST(pretokenize_54_word_in_parens);
  RUN_TEST(pretokenize_55_word_in_brackets);
  RUN_TEST(pretokenize_56_word_in_braces);
  RUN_TEST(pretokenize_57_word_in_angle_brackets);
  RUN_TEST(pretokenize_58_dollar_amount);
  RUN_TEST(pretokenize_59_4_digit_dollar_amount);
  RUN_TEST(pretokenize_60_dollar_with_comma);
  RUN_TEST(pretokenize_61_percentage);
  RUN_TEST(pretokenize_62_decimal_number);
  RUN_TEST(pretokenize_63_fraction);
  RUN_TEST(pretokenize_64_simple_math);
  RUN_TEST(pretokenize_65_multiplication);
  RUN_TEST(pretokenize_66_subtraction);
  RUN_TEST(pretokenize_67_division);
  RUN_TEST(pretokenize_68_space_then_plus);
  RUN_TEST(pretokenize_69_space_then_minus);
  RUN_TEST(pretokenize_70_space_then_asterisk);
  RUN_TEST(pretokenize_71_space_then_slash);
  RUN_TEST(pretokenize_72_space_then_at);
  RUN_TEST(pretokenize_73_space_then_hash);
  RUN_TEST(pretokenize_74_two_letters_with_newline);
  RUN_TEST(pretokenize_75_two_letters_with_double_newline);
  RUN_TEST(pretokenize_76_two_lines);
  RUN_TEST(pretokenize_77_crlf_line_ending);
  RUN_TEST(pretokenize_78_email_like);
  RUN_TEST(pretokenize_79_url_like);
  RUN_TEST(pretokenize_80_unix_path);
  RUN_TEST(pretokenize_81_windows_path);
  RUN_TEST(pretokenize_82_hello_world_punctuated);
  RUN_TEST(pretokenize_83_pangram_start);
  RUN_TEST(pretokenize_84_math_expression);
  RUN_TEST(pretokenize_85_function_call);
  RUN_TEST(pretokenize_86_function_with_args);
  RUN_TEST(pretokenize_87_assignment);
  RUN_TEST(pretokenize_88_if_statement);
  RUN_TEST(pretokenize_89_single_cjk);
  RUN_TEST(pretokenize_90_two_cjk);
  RUN_TEST(pretokenize_91_three_cjk);
  RUN_TEST(pretokenize_92_hiragana);
  RUN_TEST(pretokenize_93_katakana);
  RUN_TEST(pretokenize_94_hangul);
  RUN_TEST(pretokenize_95_cyrillic);
  RUN_TEST(pretokenize_96_arabic);
  RUN_TEST(pretokenize_97_hebrew);
  RUN_TEST(pretokenize_98_mixed_ascii_cjk);
  RUN_TEST(pretokenize_99_cjk_then_ascii);
  RUN_TEST(pretokenize_100_ascii_cjk_ascii);
  RUN_TEST(pretokenize_101_mixed_whitespace_only);
  RUN_TEST(pretokenize_102_letters_with_mixed_whitespace);
  RUN_TEST(pretokenize_103_overlong_2_byte);
  RUN_TEST(pretokenize_104_BOM_like_invalid);
  RUN_TEST(pretokenize_105_full_pangram);
  RUN_TEST(pretokenize_106_another_pangram);
  RUN_TEST(pretokenize_107_third_pangram);
}

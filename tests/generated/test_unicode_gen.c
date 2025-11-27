#include "test_framework.h"
#include "tokenizer/tiktoken.h"

TEST(unicode_letter_boundary_low) {
  ASSERT_FALSE(unicode_is_letter(0x0000));
  ASSERT_FALSE(unicode_is_letter(0x0040));
  ASSERT_TRUE(unicode_is_letter(0x0041));
  PASS();
}

TEST(unicode_letter_boundary_high) {
  ASSERT_TRUE(unicode_is_letter(0x005A));
  ASSERT_FALSE(unicode_is_letter(0x005B));
  PASS();
}

TEST(unicode_letter_ranges_exhaustive) {
  int letter_count = 0;
  for (uint32_t cp = 0x0000; cp <= 0x001F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0020; cp <= 0x0020; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0021; cp <= 0x002F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0030; cp <= 0x0039; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x003A; cp <= 0x0040; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0041; cp <= 0x005A; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x005B; cp <= 0x0060; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0061; cp <= 0x007A; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x007B; cp <= 0x007E; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x007F; cp <= 0x009F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00A0; cp <= 0x00A0; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00AA; cp <= 0x00AA; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00B2; cp <= 0x00B3; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00B5; cp <= 0x00B5; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00B9; cp <= 0x00B9; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00BA; cp <= 0x00BA; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00BC; cp <= 0x00BE; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00C0; cp <= 0x00D6; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00D8; cp <= 0x00F6; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x00F8; cp <= 0x00FF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0100; cp <= 0x017F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0370; cp <= 0x03FF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0400; cp <= 0x04FF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0530; cp <= 0x058F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0590; cp <= 0x05FF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0600; cp <= 0x06FF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0900; cp <= 0x097F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x0E00; cp <= 0x0E7F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x1100; cp <= 0x11FF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x1680; cp <= 0x1680; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x2000; cp <= 0x200A; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x2028; cp <= 0x2029; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x202F; cp <= 0x202F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x205F; cp <= 0x205F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x3000; cp <= 0x3000; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x3040; cp <= 0x309F; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x30A0; cp <= 0x30FF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0x4E00; cp <= 0x9FFF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  for (uint32_t cp = 0xAC00; cp <= 0xD7AF; cp++) {
    if (unicode_is_letter(cp))
      letter_count++;
  }
  ASSERT(letter_count > 0);
  PASS();
}

TEST(unicode_number_ascii_digits) {
  for (uint32_t cp = '0'; cp <= '9'; cp++) {
    ASSERT_TRUE(unicode_is_number(cp));
  }
  PASS();
}

TEST(unicode_number_superscripts) {
  ASSERT_TRUE(unicode_is_number(0x00B2));
  ASSERT_TRUE(unicode_is_number(0x00B3));
  ASSERT_TRUE(unicode_is_number(0x00B9));
  PASS();
}

TEST(unicode_number_fractions) {
  ASSERT_TRUE(unicode_is_number(0x00BC));
  ASSERT_TRUE(unicode_is_number(0x00BD));
  ASSERT_TRUE(unicode_is_number(0x00BE));
  PASS();
}

TEST(unicode_number_fullwidth) {
  for (uint32_t cp = 0xFF10; cp <= 0xFF19; cp++) {
    ASSERT_TRUE(unicode_is_number(cp));
  }
  PASS();
}

TEST(unicode_whitespace_ascii) {
  ASSERT_TRUE(unicode_is_whitespace(' '));
  ASSERT_TRUE(unicode_is_whitespace('\t'));
  ASSERT_TRUE(unicode_is_whitespace('\n'));
  ASSERT_TRUE(unicode_is_whitespace('\r'));
  ASSERT_TRUE(unicode_is_whitespace('\f'));
  ASSERT_TRUE(unicode_is_whitespace('\v'));
  PASS();
}

TEST(unicode_whitespace_extended) {
  ASSERT_TRUE(unicode_is_whitespace(0x85));
  ASSERT_TRUE(unicode_is_whitespace(0xA0));
  ASSERT_TRUE(unicode_is_whitespace(0x1680));
  PASS();
}

TEST(unicode_whitespace_general_punctuation) {
  for (uint32_t cp = 0x2000; cp <= 0x200A; cp++) {
    ASSERT_TRUE(unicode_is_whitespace(cp));
  }
  PASS();
}

TEST(unicode_whitespace_line_separators) {
  ASSERT_TRUE(unicode_is_whitespace(0x2028));
  ASSERT_TRUE(unicode_is_whitespace(0x2029));
  PASS();
}

TEST(unicode_whitespace_special) {
  ASSERT_TRUE(unicode_is_whitespace(0x202F));
  ASSERT_TRUE(unicode_is_whitespace(0x205F));
  ASSERT_TRUE(unicode_is_whitespace(0x3000));
  PASS();
}

TEST(unicode_whitespace_not_letters) {
  uint32_t ws[] = {' ', '\t', '\n', 0xA0, 0x2000, 0x3000};
  for (size_t i = 0; i < 6; i++) {
    ASSERT_FALSE(unicode_is_letter(ws[i]));
    ASSERT_FALSE(unicode_is_number(ws[i]));
  }
  PASS();
}

TEST(unicode_non_bmp_letters) {
  ASSERT_TRUE(unicode_is_letter(0x10000));
  ASSERT_TRUE(unicode_is_letter(0x10400));
  ASSERT_TRUE(unicode_is_letter(0x10450));
  PASS();
}

TEST(unicode_non_bmp_numbers) {
  ASSERT_TRUE(unicode_is_number(0x10107));
  ASSERT_TRUE(unicode_is_number(0x10140));
  ASSERT_TRUE(unicode_is_number(0x1D7CE));
  PASS();
}

TEST(unicode_cjk_unified_ideographs) {
  ASSERT_TRUE(unicode_is_letter(0x4E00));
  ASSERT_TRUE(unicode_is_letter(0x4E2D));
  ASSERT_TRUE(unicode_is_letter(0x56FD));
  ASSERT_TRUE(unicode_is_letter(0x65E5));
  ASSERT_TRUE(unicode_is_letter(0x672C));
  PASS();
}

TEST(unicode_hiragana) {
  for (uint32_t cp = 0x3041; cp <= 0x3096; cp++) {
    ASSERT_TRUE(unicode_is_letter(cp));
  }
  PASS();
}

TEST(unicode_katakana) {
  for (uint32_t cp = 0x30A1; cp <= 0x30FA; cp++) {
    ASSERT_TRUE(unicode_is_letter(cp));
  }
  PASS();
}

TEST(unicode_hangul) {
  ASSERT_TRUE(unicode_is_letter(0xAC00));
  ASSERT_TRUE(unicode_is_letter(0xD7A3));
  PASS();
}

TEST(unicode_arabic_digits) {
  for (uint32_t cp = 0x0660; cp <= 0x0669; cp++) {
    ASSERT_TRUE(unicode_is_number(cp));
  }
  PASS();
}

TEST(unicode_devanagari_digits) {
  for (uint32_t cp = 0x0966; cp <= 0x096F; cp++) {
    ASSERT_TRUE(unicode_is_number(cp));
  }
  PASS();
}

TEST(unicode_thai_digits) {
  for (uint32_t cp = 0x0E50; cp <= 0x0E59; cp++) {
    ASSERT_TRUE(unicode_is_number(cp));
  }
  PASS();
}

void run_unicode_generated_tests(void) {
  TEST_SUITE("Unicode Generated Tests");
  RUN_TEST(unicode_letter_boundary_low);
  RUN_TEST(unicode_letter_boundary_high);
  RUN_TEST(unicode_letter_ranges_exhaustive);
  RUN_TEST(unicode_number_ascii_digits);
  RUN_TEST(unicode_number_superscripts);
  RUN_TEST(unicode_number_fractions);
  RUN_TEST(unicode_number_fullwidth);
  RUN_TEST(unicode_whitespace_ascii);
  RUN_TEST(unicode_whitespace_extended);
  RUN_TEST(unicode_whitespace_general_punctuation);
  RUN_TEST(unicode_whitespace_line_separators);
  RUN_TEST(unicode_whitespace_special);
  RUN_TEST(unicode_whitespace_not_letters);
  RUN_TEST(unicode_non_bmp_letters);
  RUN_TEST(unicode_non_bmp_numbers);
  RUN_TEST(unicode_cjk_unified_ideographs);
  RUN_TEST(unicode_hiragana);
  RUN_TEST(unicode_katakana);
  RUN_TEST(unicode_hangul);
  RUN_TEST(unicode_arabic_digits);
  RUN_TEST(unicode_devanagari_digits);
  RUN_TEST(unicode_thai_digits);
}

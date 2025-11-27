#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint32_t start;
  uint32_t end;
  const char *category;
} UnicodeRange;

static const UnicodeRange RANGES[] = {
    {0x0000, 0x001F, "control"},
    {0x0020, 0x0020, "space"},
    {0x0021, 0x002F, "punctuation"},
    {0x0030, 0x0039, "digit"},
    {0x003A, 0x0040, "punctuation"},
    {0x0041, 0x005A, "uppercase"},
    {0x005B, 0x0060, "punctuation"},
    {0x0061, 0x007A, "lowercase"},
    {0x007B, 0x007E, "punctuation"},
    {0x007F, 0x009F, "control"},
    {0x00A0, 0x00A0, "whitespace_nbsp"},
    {0x00AA, 0x00AA, "letter"},
    {0x00B2, 0x00B3, "number"},
    {0x00B5, 0x00B5, "letter"},
    {0x00B9, 0x00B9, "number"},
    {0x00BA, 0x00BA, "letter"},
    {0x00BC, 0x00BE, "number"},
    {0x00C0, 0x00D6, "letter"},
    {0x00D8, 0x00F6, "letter"},
    {0x00F8, 0x00FF, "letter"},
    {0x0100, 0x017F, "letter_extended"},
    {0x0370, 0x03FF, "greek"},
    {0x0400, 0x04FF, "cyrillic"},
    {0x0530, 0x058F, "armenian"},
    {0x0590, 0x05FF, "hebrew"},
    {0x0600, 0x06FF, "arabic"},
    {0x0900, 0x097F, "devanagari"},
    {0x0E00, 0x0E7F, "thai"},
    {0x1100, 0x11FF, "hangul_jamo"},
    {0x1680, 0x1680, "whitespace_ogham"},
    {0x2000, 0x200A, "whitespace_general"},
    {0x2028, 0x2029, "whitespace_linesep"},
    {0x202F, 0x202F, "whitespace_narrow"},
    {0x205F, 0x205F, "whitespace_medium"},
    {0x3000, 0x3000, "whitespace_ideographic"},
    {0x3040, 0x309F, "hiragana"},
    {0x30A0, 0x30FF, "katakana"},
    {0x4E00, 0x9FFF, "cjk"},
    {0xAC00, 0xD7AF, "hangul"},
    {0x10000, 0x1007F, "linear_b"},
    {0x1F600, 0x1F64F, "emoji"},
};
#define RANGE_COUNT (sizeof(RANGES) / sizeof(RANGES[0]))

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"tokenizer/tiktoken.h\"\n\n");

  printf("TEST(unicode_letter_boundary_low) {\n");
  printf("  ASSERT_FALSE(unicode_is_letter(0x0000));\n");
  printf("  ASSERT_FALSE(unicode_is_letter(0x0040));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x0041));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_letter_boundary_high) {\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x005A));\n");
  printf("  ASSERT_FALSE(unicode_is_letter(0x005B));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_letter_ranges_exhaustive) {\n");
  printf("  int letter_count = 0;\n");
  for (size_t i = 0; i < RANGE_COUNT; i++) {
    if (RANGES[i].start > 0xFFFF)
      continue;
    printf("  for (uint32_t cp = 0x%04X; cp <= 0x%04X; cp++) {\n",
           RANGES[i].start, RANGES[i].end);
    printf("    if (unicode_is_letter(cp)) letter_count++;\n");
    printf("  }\n");
  }
  printf("  ASSERT(letter_count > 0);\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_number_ascii_digits) {\n");
  printf("  for (uint32_t cp = '0'; cp <= '9'; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_number(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_number_superscripts) {\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x00B2));\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x00B3));\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x00B9));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_number_fractions) {\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x00BC));\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x00BD));\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x00BE));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_number_fullwidth) {\n");
  printf("  for (uint32_t cp = 0xFF10; cp <= 0xFF19; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_number(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_whitespace_ascii) {\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(' '));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace('\\t'));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace('\\n'));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace('\\r'));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace('\\f'));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace('\\v'));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_whitespace_extended) {\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0x85));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0xA0));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0x1680));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_whitespace_general_punctuation) {\n");
  printf("  for (uint32_t cp = 0x2000; cp <= 0x200A; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_whitespace(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_whitespace_line_separators) {\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0x2028));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0x2029));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_whitespace_special) {\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0x202F));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0x205F));\n");
  printf("  ASSERT_TRUE(unicode_is_whitespace(0x3000));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_whitespace_not_letters) {\n");
  printf("  uint32_t ws[] = {' ', '\\t', '\\n', 0xA0, 0x2000, 0x3000};\n");
  printf("  for (size_t i = 0; i < 6; i++) {\n");
  printf("    ASSERT_FALSE(unicode_is_letter(ws[i]));\n");
  printf("    ASSERT_FALSE(unicode_is_number(ws[i]));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_non_bmp_letters) {\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x10000));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x10400));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x10450));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_non_bmp_numbers) {\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x10107));\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x10140));\n");
  printf("  ASSERT_TRUE(unicode_is_number(0x1D7CE));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_cjk_unified_ideographs) {\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x4E00));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x4E2D));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x56FD));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x65E5));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0x672C));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_hiragana) {\n");
  printf("  for (uint32_t cp = 0x3041; cp <= 0x3096; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_letter(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_katakana) {\n");
  printf("  for (uint32_t cp = 0x30A1; cp <= 0x30FA; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_letter(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_hangul) {\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0xAC00));\n");
  printf("  ASSERT_TRUE(unicode_is_letter(0xD7A3));\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_arabic_digits) {\n");
  printf("  for (uint32_t cp = 0x0660; cp <= 0x0669; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_number(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_devanagari_digits) {\n");
  printf("  for (uint32_t cp = 0x0966; cp <= 0x096F; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_number(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("TEST(unicode_thai_digits) {\n");
  printf("  for (uint32_t cp = 0x0E50; cp <= 0x0E59; cp++) {\n");
  printf("    ASSERT_TRUE(unicode_is_number(cp));\n");
  printf("  }\n");
  printf("  PASS();\n");
  printf("}\n\n");

  printf("void run_unicode_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"Unicode Generated Tests\");\n");
  printf("  RUN_TEST(unicode_letter_boundary_low);\n");
  printf("  RUN_TEST(unicode_letter_boundary_high);\n");
  printf("  RUN_TEST(unicode_letter_ranges_exhaustive);\n");
  printf("  RUN_TEST(unicode_number_ascii_digits);\n");
  printf("  RUN_TEST(unicode_number_superscripts);\n");
  printf("  RUN_TEST(unicode_number_fractions);\n");
  printf("  RUN_TEST(unicode_number_fullwidth);\n");
  printf("  RUN_TEST(unicode_whitespace_ascii);\n");
  printf("  RUN_TEST(unicode_whitespace_extended);\n");
  printf("  RUN_TEST(unicode_whitespace_general_punctuation);\n");
  printf("  RUN_TEST(unicode_whitespace_line_separators);\n");
  printf("  RUN_TEST(unicode_whitespace_special);\n");
  printf("  RUN_TEST(unicode_whitespace_not_letters);\n");
  printf("  RUN_TEST(unicode_non_bmp_letters);\n");
  printf("  RUN_TEST(unicode_non_bmp_numbers);\n");
  printf("  RUN_TEST(unicode_cjk_unified_ideographs);\n");
  printf("  RUN_TEST(unicode_hiragana);\n");
  printf("  RUN_TEST(unicode_katakana);\n");
  printf("  RUN_TEST(unicode_hangul);\n");
  printf("  RUN_TEST(unicode_arabic_digits);\n");
  printf("  RUN_TEST(unicode_devanagari_digits);\n");
  printf("  RUN_TEST(unicode_thai_digits);\n");
  printf("}\n");

  return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *input;
  const char *description;
  int min_spans;
  int max_spans;
} TestCase;

static const TestCase CASES[] = {
    {"", "empty string", 0, 0},
    {"a", "single ascii letter", 1, 1},
    {"A", "single uppercase letter", 1, 1},
    {"z", "single lowercase z", 1, 1},
    {"0", "single digit", 1, 1},
    {"9", "single digit 9", 1, 1},
    {" ", "single space", 1, 1},
    {"\t", "single tab", 1, 1},
    {"\n", "single newline", 1, 1},
    {"\r", "single carriage return", 1, 1},
    {".", "single period", 1, 1},
    {",", "single comma", 1, 1},
    {"!", "single exclamation", 1, 1},
    {"?", "single question", 1, 1},
    {"@", "single at sign", 1, 1},
    {"#", "single hash", 1, 1},
    {"$", "single dollar", 1, 1},
    {"%", "single percent", 1, 1},

    {"hello", "simple word", 1, 1},
    {"HELLO", "uppercase word", 1, 1},
    {"Hello", "mixed case word", 1, 1},
    {"world123", "word with trailing numbers", 1, 2},
    {"123abc", "numbers then word", 2, 3},
    {"12345", "5 digits", 2, 2},
    {"123456", "6 digits", 2, 3},
    {"1234567", "7 digits", 3, 3},
    {"123456789", "9 digits", 3, 4},

    {"hello world", "two words", 2, 2},
    {"hello  world", "two words double space", 3, 3},
    {"hello   world", "two words triple space", 2, 4},
    {"  hello", "leading spaces", 2, 2},
    {"hello  ", "trailing spaces", 1, 2},
    {"  hello  ", "leading and trailing spaces", 2, 3},

    {"'s", "contraction s", 1, 1},
    {"'t", "contraction t", 1, 1},
    {"'m", "contraction m", 1, 1},
    {"'d", "contraction d", 1, 1},
    {"'ll", "contraction ll", 1, 1},
    {"'ve", "contraction ve", 1, 1},
    {"'re", "contraction re", 1, 1},
    {"'S", "uppercase contraction S", 1, 1},
    {"'LL", "uppercase contraction LL", 1, 1},
    {"it's", "word with contraction", 2, 2},
    {"don't", "word with contraction t", 2, 2},
    {"I'm", "I am contraction", 2, 2},
    {"we'll", "we will contraction", 2, 2},
    {"they've", "they have contraction", 2, 2},
    {"you're", "you are contraction", 2, 2},

    {"test.", "word then period", 2, 2},
    {"test,", "word then comma", 2, 2},
    {"test!", "word then exclamation", 2, 2},
    {"test?", "word then question", 2, 2},
    {"test...", "word then ellipsis", 2, 4},
    {"test???", "word then multiple questions", 2, 4},
    {"(test)", "word in parens", 2, 3},
    {"[test]", "word in brackets", 2, 3},
    {"{test}", "word in braces", 2, 3},
    {"<test>", "word in angle brackets", 2, 3},

    {"$100", "dollar amount", 2, 2},
    {"$1000", "4 digit dollar amount", 2, 3},
    {"$1,000", "dollar with comma", 3, 4},
    {"100%", "percentage", 2, 2},
    {"3.14", "decimal number", 3, 3},
    {"1/2", "fraction", 3, 3},
    {"1+1", "simple math", 3, 3},
    {"2*3", "multiplication", 3, 3},
    {"10-5", "subtraction", 3, 3},
    {"10/2", "division", 3, 3},

    {" +", "space then plus", 1, 2},
    {" -", "space then minus", 1, 2},
    {" *", "space then asterisk", 1, 2},
    {" /", "space then slash", 1, 2},
    {" @", "space then at", 1, 2},
    {" #", "space then hash", 1, 2},

    {"a\nb", "two letters with newline", 3, 3},
    {"a\n\nb", "two letters with double newline", 2, 4},
    {"line1\nline2", "two lines", 2, 5},
    {"line1\r\nline2", "crlf line ending", 2, 6},

    {"test@example.com", "email-like", 2, 5},
    {"http://test", "url-like", 2, 5},
    {"path/to/file", "unix path", 2, 5},
    {"C:\\path\\file", "windows path", 4, 5},

    {"Hello, World!", "hello world punctuated", 4, 4},
    {"The quick brown fox", "pangram start", 4, 4},
    {"123 + 456 = 579", "math expression", 7, 9},
    {"foo(bar)", "function call", 2, 4},
    {"foo(a, b, c)", "function with args", 4, 8},
    {"x = y + z;", "assignment", 6, 8},
    {"if (x) { y }", "if statement", 7, 9},

    {"日", "single cjk", 1, 1},
    {"日本", "two cjk", 1, 2},
    {"日本語", "three cjk", 1, 3},
    {"こんにちは", "hiragana", 1, 5},
    {"カタカナ", "katakana", 1, 4},
    {"한글", "hangul", 1, 2},
    {"Привет", "cyrillic", 1, 1},
    {"مرحبا", "arabic", 1, 5},
    {"שלום", "hebrew", 1, 4},

    {"Hello日本", "mixed ascii cjk", 1, 3},
    {"日本Hello", "cjk then ascii", 1, 3},
    {"test日本test", "ascii cjk ascii", 1, 5},

    {"   \t\t\n\r   ", "mixed whitespace only", 1, 4},
    {"a   \t\t\n\r   b", "letters with mixed whitespace", 3, 5},

    {"a\xC0"
     "\x80"
     "b",
     "overlong 2-byte", 1, 4},
    {"\xFF"
     "\xFE",
     "BOM-like invalid", 1, 2},

    {"The quick brown fox jumps over the lazy dog", "full pangram", 9, 9},
    {"Pack my box with five dozen liquor jugs", "another pangram", 8, 8},
    {"How vexingly quick daft zebras jump!", "third pangram", 6, 7},
};
#define CASE_COUNT (sizeof(CASES) / sizeof(CASES[0]))

int main(void) {
  printf("#include \"test_framework.h\"\n");
  printf("#include \"tokenizer/tiktoken.h\"\n");
  printf("#include <stdlib.h>\n");
  printf("#include <string.h>\n\n");

  for (size_t i = 0; i < CASE_COUNT; i++) {
    char test_name[128];
    snprintf(test_name, sizeof(test_name), "pretokenize_%zu_%s", i,
             CASES[i].description);
    for (char *p = test_name; *p; p++) {
      if (*p == ' ' || *p == '-' || *p == '/' || *p == '\\' || *p == '(' ||
          *p == ')' || *p == '[' || *p == ']' || *p == '{' || *p == '}' ||
          *p == '<' || *p == '>' || *p == ',' || *p == '.')
        *p = '_';
    }

    printf("TEST(%s) {\n", test_name);
    printf("  SpanList spans = {0};\n");
    printf("  const char *input = \"");

    int last_was_hex = 0;
    for (const char *p = CASES[i].input; *p; p++) {
      unsigned char c = (unsigned char)*p;
      unsigned char next = (unsigned char)*(p + 1);
      int next_is_hex_char = (next >= '0' && next <= '9') ||
                             (next >= 'a' && next <= 'f') ||
                             (next >= 'A' && next <= 'F');

      if (last_was_hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                           (c >= 'A' && c <= 'F'))) {
        printf("\" \"");
      }

      if (c == '"') {
        printf("\\\"");
        last_was_hex = 0;
      } else if (c == '\\') {
        printf("\\\\");
        last_was_hex = 0;
      } else if (c == '\n') {
        printf("\\n");
        last_was_hex = 0;
      } else if (c == '\r') {
        printf("\\r");
        last_was_hex = 0;
      } else if (c == '\t') {
        printf("\\t");
        last_was_hex = 0;
      } else if (c < 32 || c >= 127) {
        printf("\\x%02X", c);
        last_was_hex = next_is_hex_char || (next >= 128);
        if (last_was_hex && *(p + 1)) {
          printf("\" \"");
          last_was_hex = 0;
        }
      } else {
        printf("%c", c);
        last_was_hex = 0;
      }
    }
    printf("\";\n");

    printf("  int result = pretokenize_cl100k(input, &spans);\n");
    printf("  ASSERT(result >= 0);\n");
    if (CASES[i].min_spans > 0) {
      printf("  ASSERT(spans.count >= %d);\n", CASES[i].min_spans);
    }
    if (CASES[i].max_spans > 0) {
      printf("  ASSERT(spans.count <= %d);\n", CASES[i].max_spans);
    }
    printf("  for (size_t j = 0; j < spans.count; j++) {\n");
    printf("    ASSERT(spans.spans[j].start < spans.spans[j].end);\n");
    printf("    ASSERT(spans.spans[j].end <= strlen(input));\n");
    printf("  }\n");
    printf("  size_t covered = 0;\n");
    printf("  for (size_t j = 0; j < spans.count; j++) {\n");
    printf("    covered += spans.spans[j].end - spans.spans[j].start;\n");
    printf("  }\n");
    printf("  ASSERT(covered == strlen(input));\n");
    printf("  free(spans.spans);\n");
    printf("  PASS();\n");
    printf("}\n\n");
  }

  printf("void run_pretokenize_generated_tests(void) {\n");
  printf("  TEST_SUITE(\"Pre-tokenize Generated Tests\");\n");
  for (size_t i = 0; i < CASE_COUNT; i++) {
    char test_name[128];
    snprintf(test_name, sizeof(test_name), "pretokenize_%zu_%s", i,
             CASES[i].description);
    for (char *p = test_name; *p; p++) {
      if (*p == ' ' || *p == '-' || *p == '/' || *p == '\\' || *p == '(' ||
          *p == ')' || *p == '[' || *p == ']' || *p == '{' || *p == '}' ||
          *p == '<' || *p == '>' || *p == ',' || *p == '.')
        *p = '_';
    }
    printf("  RUN_TEST(%s);\n", test_name);
  }
  printf("}\n");

  return 0;
}

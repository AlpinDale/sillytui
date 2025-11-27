#include "property_tests.h"
#include "test_framework.h"
#include "tokenizer/simd.h"
#include "tokenizer/tiktoken.h"
#include <stdlib.h>
#include <string.h>

TEST(property_utf8_encode_decode_roundtrip) {
  RngState rng;
  rng_init(&rng, 42);

  for (int i = 0; i < 10000; i++) {
    uint32_t cp;
    uint32_t r = rng_range(&rng, 100);
    if (r < 50) {
      cp = rng_range(&rng, 0x80);
    } else if (r < 70) {
      cp = 0x80 + rng_range(&rng, 0x780);
    } else if (r < 90) {
      cp = 0x800 + rng_range(&rng, 0xF800);
      if (cp >= 0xD800 && cp <= 0xDFFF) {
        cp = 0x4E00;
      }
    } else {
      cp = 0x10000 + rng_range(&rng, 0xFFFFF);
      if (cp > 0x10FFFF)
        cp = 0x10000;
    }

    uint8_t buf[4];
    int enc_len = utf8_encode(cp, buf);
    ASSERT(enc_len >= 1 && enc_len <= 4);

    uint32_t decoded;
    int dec_len = utf8_decode(buf, (size_t)enc_len, &decoded);
    ASSERT_EQ_INT(enc_len, dec_len);
    ASSERT_EQ_INT((int)cp, (int)decoded);
  }
  PASS();
}

TEST(property_unicode_letter_implies_not_number_whitespace) {
  RngState rng;
  rng_init(&rng, 12345);

  for (int i = 0; i < 5000; i++) {
    uint32_t cp = rng_range(&rng, 0x20000);

    if (unicode_is_letter(cp)) {
      ASSERT_FALSE(unicode_is_number(cp));
    }
    if (unicode_is_number(cp)) {
      ASSERT_FALSE(unicode_is_letter(cp));
    }
    if (unicode_is_whitespace(cp)) {
      ASSERT_FALSE(unicode_is_letter(cp));
      ASSERT_FALSE(unicode_is_number(cp));
    }
  }
  PASS();
}

TEST(property_pretokenize_covers_all_input) {
  RngState rng;
  rng_init(&rng, 99999);

  for (int i = 0; i < 1000; i++) {
    char buf[256];
    size_t len = gen_string(&rng, buf, 255);

    SpanList spans = {0};
    int result = pretokenize_cl100k(buf, &spans);
    ASSERT(result >= 0);

    size_t covered = 0;
    for (size_t j = 0; j < spans.count; j++) {
      ASSERT(spans.spans[j].start <= spans.spans[j].end);
      ASSERT(spans.spans[j].end <= len);
      covered += spans.spans[j].end - spans.spans[j].start;
    }
    ASSERT_EQ_SIZE(len, covered);

    free(spans.spans);
  }
  PASS();
}

TEST(property_pretokenize_spans_are_contiguous) {
  RngState rng;
  rng_init(&rng, 77777);

  for (int i = 0; i < 500; i++) {
    char buf[128];
    gen_string(&rng, buf, 127);

    SpanList spans = {0};
    int result = pretokenize_cl100k(buf, &spans);
    ASSERT(result >= 0);

    if (spans.count > 1) {
      for (size_t j = 0; j < spans.count - 1; j++) {
        ASSERT_EQ_SIZE(spans.spans[j].end, spans.spans[j + 1].start);
      }
    }

    free(spans.spans);
  }
  PASS();
}

TEST(property_pretokenize_utf8_input) {
  RngState rng;
  rng_init(&rng, 55555);

  for (int i = 0; i < 500; i++) {
    char buf[256];
    size_t len = gen_utf8_string(&rng, buf, 255);

    SpanList spans = {0};
    int result = pretokenize_cl100k(buf, &spans);
    ASSERT(result >= 0);

    size_t covered = 0;
    for (size_t j = 0; j < spans.count; j++) {
      covered += spans.spans[j].end - spans.spans[j].start;
    }
    ASSERT_EQ_SIZE(len, covered);

    free(spans.spans);
  }
  PASS();
}

TEST(property_hash_deterministic) {
  RngState rng;
  rng_init(&rng, 33333);

  for (int i = 0; i < 1000; i++) {
    uint8_t buf[64];
    size_t len = gen_bytes(&rng, buf, 63);
    if (len == 0)
      continue;

    uint64_t hash1 = simd_hash_bytes(buf, len);
    uint64_t hash2 = simd_hash_bytes(buf, len);
    ASSERT(hash1 == hash2);
  }
  PASS();
}

TEST(property_hash_different_for_different_inputs) {
  RngState rng;
  rng_init(&rng, 11111);

  int collisions = 0;
  for (int i = 0; i < 1000; i++) {
    uint8_t buf1[32], buf2[32];
    size_t len1 = gen_bytes(&rng, buf1, 31);
    size_t len2 = gen_bytes(&rng, buf2, 31);
    if (len1 == 0)
      len1 = 1;
    if (len2 == 0)
      len2 = 1;
    buf1[len1 - 1] = (uint8_t)i;

    uint64_t hash1 = simd_hash_bytes(buf1, len1);
    uint64_t hash2 = simd_hash_bytes(buf2, len2);
    if (len1 != len2 || memcmp(buf1, buf2, len1) != 0) {
      if (hash1 == hash2)
        collisions++;
    }
  }
  ASSERT(collisions < 10);
  PASS();
}

TEST(property_base64_known_values) {
  const char *encoded = "SGVsbG8gV29ybGQ=";
  uint8_t decoded[64];
  size_t dec_len = simd_base64_decode(encoded, strlen(encoded), decoded, 64);
  ASSERT_EQ_SIZE(11, dec_len);
  ASSERT(memcmp(decoded, "Hello World", 11) == 0);

  const char *enc2 = "VGVzdA==";
  dec_len = simd_base64_decode(enc2, strlen(enc2), decoded, 64);
  ASSERT_EQ_SIZE(4, dec_len);
  ASSERT(memcmp(decoded, "Test", 4) == 0);

  const char *enc3 = "YWJj";
  dec_len = simd_base64_decode(enc3, strlen(enc3), decoded, 64);
  ASSERT_EQ_SIZE(3, dec_len);
  ASSERT(memcmp(decoded, "abc", 3) == 0);

  PASS();
}

void run_property_tests(void) {
  TEST_SUITE("Property-Based Tests");
  RUN_TEST(property_utf8_encode_decode_roundtrip);
  RUN_TEST(property_unicode_letter_implies_not_number_whitespace);
  RUN_TEST(property_pretokenize_covers_all_input);
  RUN_TEST(property_pretokenize_spans_are_contiguous);
  RUN_TEST(property_pretokenize_utf8_input);
  RUN_TEST(property_hash_deterministic);
  RUN_TEST(property_hash_different_for_different_inputs);
  RUN_TEST(property_base64_known_values);
}

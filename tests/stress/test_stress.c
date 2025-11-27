#include "../test_framework.h"
#include "chat/history.h"
#include "core/config.h"
#include "tokenizer/gpt2bpe.h"
#include "tokenizer/simd.h"
#include "tokenizer/tiktoken.h"
#include <time.h>

#define STRESS_ITERATIONS 10000
#define STRESS_LARGE_ITERATIONS 100000

TEST(stress_history_rapid_add_delete) {
  ChatHistory h;
  history_init(&h);

  for (int i = 0; i < STRESS_ITERATIONS; i++) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Message %d", i);
    history_add(&h, msg);

    if (i > 0 && i % 10 == 0) {
      history_delete(&h, 0);
    }
  }

  ASSERT(h.count > 0);
  history_free(&h);
  PASS();
}

TEST(stress_history_swipe_operations) {
  ChatHistory h;
  history_init(&h);

  for (int i = 0; i < 100; i++) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Message %d", i);
    history_add(&h, msg);

    for (int j = 0; j < 50; j++) {
      char swipe[64];
      snprintf(swipe, sizeof(swipe), "Swipe %d-%d", i, j);
      history_add_swipe(&h, i, swipe);
    }

    for (int j = 0; j < 50; j++) {
      history_set_active_swipe(&h, i, j % (j + 1));
    }
  }

  history_free(&h);
  PASS();
}

TEST(stress_config_add_remove) {
  for (int iter = 0; iter < 100; iter++) {
    ModelsFile mf = {0};
    mf.active_index = -1;

    for (int i = 0; i < MAX_MODELS / 2; i++) {
      ModelConfig m = {0};
      snprintf(m.name, sizeof(m.name), "Model %d", i);
      config_add_model(&mf, &m);
    }

    while (mf.count > 0) {
      config_remove_model(&mf, mf.count / 2);
    }

    ASSERT_EQ_SIZE(0, mf.count);
  }

  PASS();
}

TEST(stress_tokenizer_encode_decode) {
  simd_init();
  Tokenizer tok;
  tokenizer_init(&tok);

  if (!tokenizer_load_tiktoken(&tok,
                               "tokenizers/openai/cl100k_base.tiktoken")) {
    tokenizer_free(&tok);
    printf("(skipped - vocab not found) ");
    PASS();
  }

  const char *texts[] = {
      "Hello, world!",
      "The quick brown fox jumps over the lazy dog.",
      "こんにちは世界",
      "Hello 😀🎉 World!",
      "1234567890",
      "   multiple   spaces   ",
      "\n\t\r special whitespace",
      "MixedCASE_and_symbols!@#$%",
  };

  for (int iter = 0; iter < 1000; iter++) {
    for (size_t i = 0; i < sizeof(texts) / sizeof(texts[0]); i++) {
      uint32_t tokens[256];
      int count = tokenizer_encode(&tok, texts[i], tokens, 256);
      ASSERT(count >= 0);

      if (count > 0) {
        char *decoded = tokenizer_decode(&tok, tokens, count);
        ASSERT_NOT_NULL(decoded);
        ASSERT_EQ_STR(texts[i], decoded);
        free(decoded);
      }
    }
  }

  tokenizer_free(&tok);
  PASS();
}

TEST(stress_gpt2bpe_encode_decode) {
  GPT2BPETokenizer tok;
  gpt2_init(&tok);

  if (!gpt2_load(&tok, "tokenizers/llama3/vocab.json",
                 "tokenizers/llama3/merges.txt")) {
    gpt2_free(&tok);
    printf("(skipped - vocab not found) ");
    PASS();
  }

  const char *texts[] = {
      "Hello, world!",
      "The quick brown fox.",
      "日本語テスト",
      "Numbers: 12345",
  };

  for (int iter = 0; iter < 1000; iter++) {
    for (size_t i = 0; i < sizeof(texts) / sizeof(texts[0]); i++) {
      uint32_t tokens[256];
      int count = gpt2_encode(&tok, texts[i], tokens, 256);
      ASSERT(count >= 0);

      if (count > 0) {
        char *decoded = gpt2_decode(&tok, tokens, count);
        ASSERT_NOT_NULL(decoded);
        free(decoded);
      }
    }
  }

  gpt2_free(&tok);
  PASS();
}

TEST(stress_memory_churn) {
  for (int iter = 0; iter < 1000; iter++) {
    ChatHistory h;
    history_init(&h);

    for (int i = 0; i < 100; i++) {
      char msg[256];
      memset(msg, 'x', 255);
      msg[255] = '\0';
      history_add(&h, msg);
    }

    history_free(&h);
  }

  PASS();
}

TEST(stress_concurrent_like_access) {
  ChatHistory h;
  history_init(&h);

  for (int i = 0; i < 1000; i++) {
    history_add(&h, "Message");
  }

  for (int iter = 0; iter < 10000; iter++) {
    int op = iter % 5;
    size_t idx = iter % h.count;

    switch (op) {
    case 0:
      history_get(&h, idx);
      break;
    case 1:
      history_update(&h, idx, "Updated");
      break;
    case 2:
      history_get_swipe_count(&h, idx);
      break;
    case 3:
      history_get_active_swipe(&h, idx);
      break;
    case 4:
      history_set_token_count(&h, idx, 0, iter);
      break;
    }
  }

  history_free(&h);
  PASS();
}

TEST(stress_hash_collisions) {
  simd_init();

  char buf[256];
  for (int i = 0; i < STRESS_LARGE_ITERATIONS; i++) {
    snprintf(buf, sizeof(buf), "test_string_%d_with_some_padding", i);
    uint64_t hash = simd_hash_bytes((const uint8_t *)buf, strlen(buf));
    (void)hash;
  }

  PASS();
}

TEST(stress_utf8_processing) {
  simd_init();

  const char *unicode_samples[] = {
      "ASCII only text here",
      "Émoji: 😀🎉🔥💯",
      "日本語テキスト",
      "Ελληνικά",
      "العربية",
      "Mixed: Hello 世界 🌍",
  };

  for (int iter = 0; iter < 10000; iter++) {
    for (size_t i = 0; i < sizeof(unicode_samples) / sizeof(unicode_samples[0]);
         i++) {
      const char *s = unicode_samples[i];
      size_t len = strlen(s);

      simd_is_all_ascii((const uint8_t *)s, len);
      simd_count_utf8_chars((const uint8_t *)s, len);
      simd_find_non_ascii((const uint8_t *)s, len);
    }
  }

  PASS();
}

void run_stress_tests(void);
void run_stress_tests(void) {
  TEST_SUITE("Stress Tests");
  RUN_TEST(stress_history_rapid_add_delete);
  RUN_TEST(stress_history_swipe_operations);
  RUN_TEST(stress_config_add_remove);
  RUN_TEST(stress_tokenizer_encode_decode);
  RUN_TEST(stress_gpt2bpe_encode_decode);
  RUN_TEST(stress_memory_churn);
  RUN_TEST(stress_concurrent_like_access);
  RUN_TEST(stress_hash_collisions);
  RUN_TEST(stress_utf8_processing);
}

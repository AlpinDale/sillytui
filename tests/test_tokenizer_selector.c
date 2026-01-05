#include "inference/tokenizer/selector.h"
#include "test_framework.h"
#include <stdlib.h>
#include <string.h>

TEST(tokenizer_selector_init) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  ASSERT_EQ_INT(TOKENIZER_API, ct.selection);
  ASSERT(!ct.loaded);
  ASSERT(ct.instance == NULL);
  PASS();
}

TEST(tokenizer_selector_init_null) {
  chat_tokenizer_init(NULL);
  PASS();
}

TEST(tokenizer_selector_free_null) {
  chat_tokenizer_free(NULL);
  PASS();
}

TEST(tokenizer_selector_free_unloaded) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_set_api) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  ASSERT(chat_tokenizer_set(&ct, TOKENIZER_API));
  ASSERT_EQ_INT(TOKENIZER_API, ct.selection);
  ASSERT(ct.loaded);
  ASSERT(chat_tokenizer_is_api(&ct));
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_is_api) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  ASSERT(chat_tokenizer_is_api(&ct));
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_is_api_null) {
  ASSERT(!chat_tokenizer_is_api(NULL));
  PASS();
}

TEST(tokenizer_selector_count_null) {
  ASSERT_EQ_INT(-1, chat_tokenizer_count(NULL, "test"));
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  ASSERT_EQ_INT(-1, chat_tokenizer_count(&ct, NULL));
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_count_api_returns_negative) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  chat_tokenizer_set(&ct, TOKENIZER_API);
  ASSERT_EQ_INT(-1, chat_tokenizer_count(&ct, "test"));
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selection_name_all) {
  ASSERT_EQ_STR("api", tokenizer_selection_name(TOKENIZER_API));
  ASSERT_EQ_STR("openai", tokenizer_selection_name(TOKENIZER_OPENAI_LEGACY));
  ASSERT_EQ_STR("openai-o200k", tokenizer_selection_name(TOKENIZER_OPENAI_NEW));
  ASSERT_EQ_STR("deepseek", tokenizer_selection_name(TOKENIZER_DEEPSEEK));
  ASSERT_EQ_STR("glm4", tokenizer_selection_name(TOKENIZER_GLM4));
  ASSERT_EQ_STR("llama3", tokenizer_selection_name(TOKENIZER_LLAMA3));
  ASSERT_EQ_STR("qwen3", tokenizer_selection_name(TOKENIZER_QWEN3));
  PASS();
}

TEST(tokenizer_selection_name_invalid) {
  ASSERT_EQ_STR("unknown", tokenizer_selection_name(TOKENIZER_COUNT));
  ASSERT_EQ_STR("unknown", tokenizer_selection_name(999));
  PASS();
}

TEST(tokenizer_selection_description_all) {
  ASSERT(strlen(tokenizer_selection_description(TOKENIZER_API)) > 0);
  ASSERT(strlen(tokenizer_selection_description(TOKENIZER_OPENAI_LEGACY)) > 0);
  ASSERT(strlen(tokenizer_selection_description(TOKENIZER_OPENAI_NEW)) > 0);
  ASSERT(strlen(tokenizer_selection_description(TOKENIZER_DEEPSEEK)) > 0);
  ASSERT(strlen(tokenizer_selection_description(TOKENIZER_GLM4)) > 0);
  ASSERT(strlen(tokenizer_selection_description(TOKENIZER_LLAMA3)) > 0);
  ASSERT(strlen(tokenizer_selection_description(TOKENIZER_QWEN3)) > 0);
  PASS();
}

TEST(tokenizer_selection_from_name_valid) {
  ASSERT_EQ_INT(TOKENIZER_API, tokenizer_selection_from_name("api"));
  ASSERT_EQ_INT(TOKENIZER_OPENAI_LEGACY,
                tokenizer_selection_from_name("openai"));
  ASSERT_EQ_INT(TOKENIZER_OPENAI_NEW,
                tokenizer_selection_from_name("openai-o200k"));
  ASSERT_EQ_INT(TOKENIZER_DEEPSEEK, tokenizer_selection_from_name("deepseek"));
  ASSERT_EQ_INT(TOKENIZER_GLM4, tokenizer_selection_from_name("glm4"));
  ASSERT_EQ_INT(TOKENIZER_LLAMA3, tokenizer_selection_from_name("llama3"));
  ASSERT_EQ_INT(TOKENIZER_QWEN3, tokenizer_selection_from_name("qwen3"));
  PASS();
}

TEST(tokenizer_selection_from_name_invalid) {
  ASSERT_EQ_INT(TOKENIZER_API, tokenizer_selection_from_name(NULL));
  ASSERT_EQ_INT(TOKENIZER_API, tokenizer_selection_from_name(""));
  ASSERT_EQ_INT(TOKENIZER_API, tokenizer_selection_from_name("nonexistent"));
  ASSERT_EQ_INT(TOKENIZER_API, tokenizer_selection_from_name("OPENAI"));
  PASS();
}

TEST(tokenizer_selector_set_invalid) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  ASSERT(!chat_tokenizer_set(&ct, TOKENIZER_COUNT));
  ASSERT(!chat_tokenizer_set(&ct, 999));
  ASSERT(!chat_tokenizer_set(NULL, TOKENIZER_API));
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_double_set) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  ASSERT(chat_tokenizer_set(&ct, TOKENIZER_API));
  ASSERT_EQ_INT(TOKENIZER_API, ct.selection);
  ASSERT(chat_tokenizer_set(&ct, TOKENIZER_API));
  ASSERT_EQ_INT(TOKENIZER_API, ct.selection);
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_load_openai) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    ASSERT(!chat_tokenizer_is_api(&ct));
    ASSERT(ct.loaded);
    ASSERT(ct.instance != NULL);
    int count = chat_tokenizer_count(&ct, "Hello, world!");
    ASSERT(count > 0);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_load_llama3) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_LLAMA3);
  if (loaded) {
    ASSERT(!chat_tokenizer_is_api(&ct));
    ASSERT(ct.loaded);
    int count = chat_tokenizer_count(&ct, "Hello, world!");
    ASSERT(count > 0);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_selector_switch_tokenizers) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  chat_tokenizer_set(&ct, TOKENIZER_API);
  ASSERT(chat_tokenizer_is_api(&ct));
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    ASSERT(!chat_tokenizer_is_api(&ct));
    ASSERT_EQ_INT(TOKENIZER_OPENAI_LEGACY, ct.selection);
  }
  chat_tokenizer_set(&ct, TOKENIZER_API);
  ASSERT(chat_tokenizer_is_api(&ct));
  ASSERT_EQ_INT(TOKENIZER_API, ct.selection);
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_count_empty_string) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    int count = chat_tokenizer_count(&ct, "");
    ASSERT_EQ_INT(0, count);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_count_simple_text) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    int count = chat_tokenizer_count(&ct, "Hello world");
    ASSERT(count > 0);
    ASSERT(count < 10);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_count_multiline) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    int count = chat_tokenizer_count(&ct, "Hello\nWorld\nTest");
    ASSERT(count > 0);
    int single = chat_tokenizer_count(&ct, "Hello");
    ASSERT(count >= single);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_count_special_chars) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    int count = chat_tokenizer_count(&ct, "Hello! @#$% 123");
    ASSERT(count > 0);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(token_result_init_free) {
  TokenResult tr;
  token_result_init(&tr);
  ASSERT(tr.ids == NULL);
  ASSERT(tr.offsets == NULL);
  ASSERT_EQ_INT(0, (int)tr.count);
  ASSERT_EQ_INT(0, (int)tr.cap);
  token_result_free(&tr);
  token_result_init(NULL);
  token_result_free(NULL);
  PASS();
}

TEST(tokenizer_encode_null) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  TokenResult tr;
  token_result_init(&tr);
  ASSERT_EQ_INT(-1, chat_tokenizer_encode(NULL, "test", &tr));
  ASSERT_EQ_INT(-1, chat_tokenizer_encode(&ct, NULL, &tr));
  ASSERT_EQ_INT(-1, chat_tokenizer_encode(&ct, "test", NULL));
  token_result_free(&tr);
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_encode_api_returns_negative) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  chat_tokenizer_set(&ct, TOKENIZER_API);
  TokenResult tr;
  token_result_init(&tr);
  ASSERT_EQ_INT(-1, chat_tokenizer_encode(&ct, "test", &tr));
  token_result_free(&tr);
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_encode_with_offsets) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    TokenResult tr;
    token_result_init(&tr);
    int count = chat_tokenizer_encode(&ct, "Hello world", &tr);
    ASSERT(count > 0);
    ASSERT_EQ_INT((int)tr.count, count);
    ASSERT(tr.ids != NULL);
    ASSERT(tr.offsets != NULL);
    ASSERT_EQ_INT(0, (int)tr.offsets[0]);
    ASSERT(tr.offsets[count] > 0);
    token_result_free(&tr);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_encode_offsets_monotonic) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    TokenResult tr;
    token_result_init(&tr);
    int count = chat_tokenizer_encode(&ct, "The quick brown fox", &tr);
    ASSERT(count > 0);
    for (int i = 0; i < count; i++) {
      ASSERT(tr.offsets[i] < tr.offsets[i + 1]);
    }
    token_result_free(&tr);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

TEST(tokenizer_encode_reuses_buffer) {
  ChatTokenizer ct;
  chat_tokenizer_init(&ct);
  bool loaded = chat_tokenizer_set(&ct, TOKENIZER_OPENAI_LEGACY);
  if (loaded) {
    TokenResult tr;
    token_result_init(&tr);
    chat_tokenizer_encode(&ct, "first", &tr);
    uint32_t *first_ids = tr.ids;
    chat_tokenizer_encode(&ct, "second", &tr);
    ASSERT(tr.ids == first_ids);
    token_result_free(&tr);
  }
  chat_tokenizer_free(&ct);
  PASS();
}

void run_tokenizer_selector_tests(void) {
  TEST_SUITE("Tokenizer Selector Tests");
  RUN_TEST(tokenizer_selector_init);
  RUN_TEST(tokenizer_selector_init_null);
  RUN_TEST(tokenizer_selector_free_null);
  RUN_TEST(tokenizer_selector_free_unloaded);
  RUN_TEST(tokenizer_selector_set_api);
  RUN_TEST(tokenizer_selector_is_api);
  RUN_TEST(tokenizer_selector_is_api_null);
  RUN_TEST(tokenizer_selector_count_null);
  RUN_TEST(tokenizer_selector_count_api_returns_negative);
  RUN_TEST(tokenizer_selection_name_all);
  RUN_TEST(tokenizer_selection_name_invalid);
  RUN_TEST(tokenizer_selection_description_all);
  RUN_TEST(tokenizer_selection_from_name_valid);
  RUN_TEST(tokenizer_selection_from_name_invalid);
  RUN_TEST(tokenizer_selector_set_invalid);
  RUN_TEST(tokenizer_selector_double_set);
  RUN_TEST(tokenizer_selector_load_openai);
  RUN_TEST(tokenizer_selector_load_llama3);
  RUN_TEST(tokenizer_selector_switch_tokenizers);
  RUN_TEST(tokenizer_count_empty_string);
  RUN_TEST(tokenizer_count_simple_text);
  RUN_TEST(tokenizer_count_multiline);
  RUN_TEST(tokenizer_count_special_chars);
  RUN_TEST(token_result_init_free);
  RUN_TEST(tokenizer_encode_null);
  RUN_TEST(tokenizer_encode_api_returns_negative);
  RUN_TEST(tokenizer_encode_with_offsets);
  RUN_TEST(tokenizer_encode_offsets_monotonic);
  RUN_TEST(tokenizer_encode_reuses_buffer);
}

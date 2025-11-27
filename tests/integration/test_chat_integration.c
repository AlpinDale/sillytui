#include "../test_framework.h"
#include "chat/chat.h"
#include "chat/history.h"
#include "test_helper.h"

TEST(chat_save_and_load_roundtrip) {
  setup_test_environment();

  ChatHistory h1;
  history_init(&h1);
  history_add(&h1, "You: Hello, how are you?");
  history_add(&h1, "Bot: I'm doing great, thank you!");
  history_add(&h1, "You: That's wonderful!");

  char id[64];
  snprintf(id, sizeof(id), "%s", chat_generate_id());
  ASSERT(id[0] != '\0');

  bool saved =
      chat_save(&h1, id, "Test Chat", "/path/to/char.json", "TestChar");
  ASSERT_TRUE(saved);

  ChatHistory h2;
  history_init(&h2);
  char char_path[512];
  bool loaded = chat_load(&h2, id, "TestChar", char_path, sizeof(char_path));
  ASSERT_TRUE(loaded);

  ASSERT_EQ_SIZE(3, h2.count);
  ASSERT_EQ_STR("You: Hello, how are you?", history_get(&h2, 0));
  ASSERT_EQ_STR("Bot: I'm doing great, thank you!", history_get(&h2, 1));
  ASSERT_EQ_STR("You: That's wonderful!", history_get(&h2, 2));

  history_free(&h1);
  history_free(&h2);
  teardown_test_environment();
  PASS();
}

TEST(chat_list_load_empty) {
  setup_test_environment();

  ChatList list;
  chat_list_init(&list);
  chat_list_load(&list);
  ASSERT_EQ_SIZE(0, list.count);
  chat_list_free(&list);

  teardown_test_environment();
  PASS();
}

TEST(chat_list_after_save) {
  setup_test_environment();

  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test message");

  char id[64];
  snprintf(id, sizeof(id), "%s", chat_generate_id());
  chat_save(&h, id, "My Chat", NULL, "TestChar");

  ChatList list;
  chat_list_init(&list);
  chat_list_load_for_character(&list, "TestChar");
  ASSERT_EQ_SIZE(1, list.count);
  ASSERT_EQ_STR("My Chat", list.chats[0].title);

  chat_list_free(&list);
  history_free(&h);
  teardown_test_environment();
  PASS();
}

TEST(chat_delete_removes_chat) {
  setup_test_environment();

  ChatHistory h;
  history_init(&h);
  history_add(&h, "Delete me");

  char id[64];
  snprintf(id, sizeof(id), "%s", chat_generate_id());
  chat_save(&h, id, "To Delete", NULL, "TestChar");

  ChatList list1;
  chat_list_init(&list1);
  chat_list_load_for_character(&list1, "TestChar");
  ASSERT_EQ_SIZE(1, list1.count);
  chat_list_free(&list1);

  bool deleted = chat_delete(id, "TestChar");
  ASSERT_TRUE(deleted);

  ChatList list2;
  chat_list_init(&list2);
  chat_list_load_for_character(&list2, "TestChar");
  ASSERT_EQ_SIZE(0, list2.count);
  chat_list_free(&list2);

  history_free(&h);
  teardown_test_environment();
  PASS();
}

TEST(chat_generate_id_not_null) {
  const char *id = chat_generate_id();
  ASSERT_NOT_NULL(id);
  ASSERT(strlen(id) > 0);
  PASS();
}

TEST(chat_auto_title_from_first_message) {
  ChatHistory h;
  history_init(&h);
  history_add(&h, "You: This is a test message for auto title");

  const char *title = chat_auto_title(&h);
  ASSERT_NOT_NULL(title);
  ASSERT(strlen(title) > 0);

  history_free(&h);
  PASS();
}

TEST(chat_find_by_title_works) {
  setup_test_environment();

  ChatHistory h;
  history_init(&h);
  history_add(&h, "Test");

  char id[64];
  snprintf(id, sizeof(id), "%s", chat_generate_id());
  chat_save(&h, id, "Unique Title", NULL, "TestChar");

  char found_id[CHAT_ID_MAX];
  bool found = chat_find_by_title("Unique Title", "TestChar", found_id,
                                  sizeof(found_id));
  ASSERT_TRUE(found);
  ASSERT_EQ_STR(id, found_id);

  history_free(&h);
  teardown_test_environment();
  PASS();
}

TEST(chat_save_with_swipes) {
  setup_test_environment();

  ChatHistory h1;
  history_init(&h1);
  history_add(&h1, "You: Hello");
  history_add(&h1, "Bot: Response 1");
  history_add_swipe(&h1, 1, "Bot: Response 2");
  history_add_swipe(&h1, 1, "Bot: Response 3");
  history_set_active_swipe(&h1, 1, 1);

  char id[64];
  snprintf(id, sizeof(id), "%s", chat_generate_id());
  bool saved = chat_save(&h1, id, "Swipe Test", NULL, "TestChar");
  ASSERT_TRUE(saved);

  ChatHistory h2;
  history_init(&h2);
  char path[512];
  bool loaded = chat_load(&h2, id, "TestChar", path, sizeof(path));
  ASSERT_TRUE(loaded);

  ASSERT_EQ_SIZE(2, h2.count);
  ASSERT_EQ_SIZE(3, history_get_swipe_count(&h2, 1));
  ASSERT_EQ_STR("Bot: Response 1", history_get_swipe(&h2, 1, 0));
  ASSERT_EQ_STR("Bot: Response 2", history_get_swipe(&h2, 1, 1));
  ASSERT_EQ_STR("Bot: Response 3", history_get_swipe(&h2, 1, 2));

  history_free(&h1);
  history_free(&h2);
  teardown_test_environment();
  PASS();
}

TEST(chat_sanitize_dirname_special_chars) {
  char out[128];
  chat_sanitize_dirname("Test/Char:Name?", out, sizeof(out));
  ASSERT(strchr(out, '/') == NULL);
  ASSERT(strchr(out, ':') == NULL);
  ASSERT(strchr(out, '?') == NULL);
  PASS();
}

TEST(chat_character_list_load_empty) {
  setup_test_environment();

  ChatCharacterList list;
  chat_character_list_init(&list);
  chat_character_list_load(&list);
  ASSERT_EQ_SIZE(0, list.count);
  chat_character_list_free(&list);

  teardown_test_environment();
  PASS();
}

void run_chat_integration_tests(void) {
  TEST_SUITE("Chat Integration");
  RUN_TEST(chat_save_and_load_roundtrip);
  RUN_TEST(chat_list_load_empty);
  RUN_TEST(chat_list_after_save);
  RUN_TEST(chat_delete_removes_chat);
  RUN_TEST(chat_generate_id_not_null);
  RUN_TEST(chat_auto_title_from_first_message);
  RUN_TEST(chat_find_by_title_works);
  RUN_TEST(chat_save_with_swipes);
  RUN_TEST(chat_sanitize_dirname_special_chars);
  RUN_TEST(chat_character_list_load_empty);
}

#include "lore/lorebook.h"
#include "test_framework.h"
#include <stdlib.h>
#include <string.h>

TEST(lorebook_init_free) {
  Lorebook lb;
  lorebook_init(&lb);
  ASSERT_EQ_SIZE(0, lb.entry_count);
  ASSERT_EQ_SIZE(0, lb.entry_capacity);
  ASSERT(lb.entries == NULL);
  ASSERT_EQ_INT(1, lb.next_uid);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_init_null_safe) {
  lorebook_init(NULL);
  lorebook_free(NULL);
  PASS();
}

TEST(lore_entry_init_free) {
  LoreEntry entry;
  lore_entry_init(&entry);
  ASSERT_EQ_INT(0, entry.uid);
  ASSERT_EQ_SIZE(0, entry.key_count);
  ASSERT(entry.keys == NULL);
  ASSERT(entry.content == NULL);
  ASSERT(!entry.constant);
  ASSERT(!entry.selective);
  ASSERT(!entry.disabled);
  ASSERT(!entry.case_sensitive);
  ASSERT(!entry.match_whole_words);
  ASSERT_EQ_INT(LORE_POS_AFTER_CHAR, entry.position);
  ASSERT_EQ_INT(LORE_ROLE_SYSTEM, entry.role);
  ASSERT_EQ_INT(4, entry.depth);
  ASSERT_EQ_INT(50, entry.scan_depth);
  ASSERT(entry.probability > 0.99f);
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_add_keys) {
  LoreEntry entry;
  lore_entry_init(&entry);
  ASSERT(lore_entry_add_key(&entry, "dragon"));
  ASSERT(lore_entry_add_key(&entry, "wyrm"));
  ASSERT_EQ_SIZE(2, entry.key_count);
  ASSERT_EQ_STR("dragon", entry.keys[0]);
  ASSERT_EQ_STR("wyrm", entry.keys[1]);
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_add_secondary_keys) {
  LoreEntry entry;
  lore_entry_init(&entry);
  ASSERT(lore_entry_add_secondary_key(&entry, "fire"));
  ASSERT(lore_entry_add_secondary_key(&entry, "ancient"));
  ASSERT_EQ_SIZE(2, entry.key_secondary_count);
  ASSERT_EQ_STR("fire", entry.keys_secondary[0]);
  ASSERT_EQ_STR("ancient", entry.keys_secondary[1]);
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_set_content) {
  LoreEntry entry;
  lore_entry_init(&entry);
  lore_entry_set_content(&entry, "Dragons are ancient beings of great power.");
  ASSERT_NOT_NULL(entry.content);
  ASSERT_EQ_STR("Dragons are ancient beings of great power.", entry.content);
  lore_entry_set_content(&entry, "Updated content.");
  ASSERT_EQ_STR("Updated content.", entry.content);
  lore_entry_free(&entry);
  PASS();
}

TEST(lorebook_add_entry) {
  Lorebook lb;
  lorebook_init(&lb);
  LoreEntry entry;
  lore_entry_init(&entry);
  lore_entry_add_key(&entry, "dragon");
  lore_entry_set_content(&entry, "A dragon is a mythical creature.");
  int uid = lorebook_add_entry(&lb, &entry);
  ASSERT(uid > 0);
  ASSERT_EQ_SIZE(1, lb.entry_count);
  LoreEntry *got = lorebook_get_entry(&lb, uid);
  ASSERT_NOT_NULL(got);
  ASSERT_EQ_INT(uid, got->uid);
  ASSERT_EQ_STR("dragon", got->keys[0]);
  lore_entry_free(&entry);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_add_multiple_entries) {
  Lorebook lb;
  lorebook_init(&lb);
  for (int i = 0; i < 10; i++) {
    LoreEntry entry;
    lore_entry_init(&entry);
    char key[32];
    snprintf(key, sizeof(key), "key%d", i);
    lore_entry_add_key(&entry, key);
    int uid = lorebook_add_entry(&lb, &entry);
    ASSERT_EQ_INT(i + 1, uid);
    lore_entry_free(&entry);
  }
  ASSERT_EQ_SIZE(10, lb.entry_count);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_remove_entry) {
  Lorebook lb;
  lorebook_init(&lb);
  LoreEntry entry;
  lore_entry_init(&entry);
  lore_entry_add_key(&entry, "dragon");
  int uid = lorebook_add_entry(&lb, &entry);
  ASSERT_EQ_SIZE(1, lb.entry_count);
  ASSERT(lorebook_remove_entry(&lb, uid));
  ASSERT_EQ_SIZE(0, lb.entry_count);
  ASSERT(!lorebook_remove_entry(&lb, uid));
  lore_entry_free(&entry);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_toggle_entry) {
  Lorebook lb;
  lorebook_init(&lb);
  LoreEntry entry;
  lore_entry_init(&entry);
  lore_entry_add_key(&entry, "dragon");
  int uid = lorebook_add_entry(&lb, &entry);
  LoreEntry *got = lorebook_get_entry(&lb, uid);
  ASSERT(!got->disabled);
  ASSERT(lorebook_toggle_entry(&lb, uid));
  ASSERT(got->disabled);
  ASSERT(lorebook_toggle_entry(&lb, uid));
  ASSERT(!got->disabled);
  lore_entry_free(&entry);
  lorebook_free(&lb);
  PASS();
}

TEST(lore_entry_matches_simple) {
  LoreEntry entry;
  lore_entry_init(&entry);
  lore_entry_add_key(&entry, "dragon");
  ASSERT(lore_entry_matches(&entry, "I saw a dragon yesterday"));
  ASSERT(lore_entry_matches(&entry, "dragon"));
  ASSERT(!lore_entry_matches(&entry, "I saw a wyrm yesterday"));
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_matches_multiple_keys) {
  LoreEntry entry;
  lore_entry_init(&entry);
  lore_entry_add_key(&entry, "dragon");
  lore_entry_add_key(&entry, "wyrm");
  ASSERT(lore_entry_matches(&entry, "I saw a dragon yesterday"));
  ASSERT(lore_entry_matches(&entry, "The wyrm attacked"));
  ASSERT(!lore_entry_matches(&entry, "I saw a bird"));
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_matches_case_insensitive) {
  LoreEntry entry;
  lore_entry_init(&entry);
  entry.case_sensitive = false;
  lore_entry_add_key(&entry, "Dragon");
  ASSERT(lore_entry_matches(&entry, "I saw a dragon"));
  ASSERT(lore_entry_matches(&entry, "I saw a DRAGON"));
  ASSERT(lore_entry_matches(&entry, "I saw a DrAgOn"));
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_matches_case_sensitive) {
  LoreEntry entry;
  lore_entry_init(&entry);
  entry.case_sensitive = true;
  lore_entry_add_key(&entry, "Dragon");
  ASSERT(lore_entry_matches(&entry, "I saw a Dragon"));
  ASSERT(!lore_entry_matches(&entry, "I saw a dragon"));
  ASSERT(!lore_entry_matches(&entry, "I saw a DRAGON"));
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_matches_whole_words) {
  LoreEntry entry;
  lore_entry_init(&entry);
  entry.match_whole_words = true;
  lore_entry_add_key(&entry, "dragon");
  ASSERT(lore_entry_matches(&entry, "I saw a dragon yesterday"));
  ASSERT(lore_entry_matches(&entry, "dragon"));
  ASSERT(lore_entry_matches(&entry, "dragon!"));
  ASSERT(!lore_entry_matches(&entry, "dragonfly"));
  ASSERT(!lore_entry_matches(&entry, "the dragonborn"));
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_matches_selective) {
  LoreEntry entry;
  lore_entry_init(&entry);
  entry.selective = true;
  lore_entry_add_key(&entry, "dragon");
  lore_entry_add_secondary_key(&entry, "fire");
  ASSERT(lore_entry_matches(&entry, "The fire dragon breathed flames"));
  ASSERT(!lore_entry_matches(&entry, "I saw a dragon"));
  ASSERT(!lore_entry_matches(&entry, "The fire burned"));
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_matches_constant) {
  LoreEntry entry;
  lore_entry_init(&entry);
  entry.constant = true;
  ASSERT(lore_entry_matches(&entry, "any text at all"));
  ASSERT(lore_entry_matches(&entry, ""));
  lore_entry_free(&entry);
  PASS();
}

TEST(lore_entry_matches_disabled) {
  LoreEntry entry;
  lore_entry_init(&entry);
  entry.disabled = true;
  lore_entry_add_key(&entry, "dragon");
  ASSERT(!lore_entry_matches(&entry, "I saw a dragon"));
  lore_entry_free(&entry);
  PASS();
}

TEST(lorebook_find_matches) {
  Lorebook lb;
  lorebook_init(&lb);
  LoreEntry e1, e2, e3;
  lore_entry_init(&e1);
  lore_entry_init(&e2);
  lore_entry_init(&e3);
  lore_entry_add_key(&e1, "dragon");
  lore_entry_add_key(&e2, "wizard");
  lore_entry_add_key(&e3, "sword");
  lorebook_add_entry(&lb, &e1);
  lorebook_add_entry(&lb, &e2);
  lorebook_add_entry(&lb, &e3);
  LoreMatchResult result;
  lore_match_init(&result);
  lorebook_find_matches(&lb, "The dragon fought the wizard", &result);
  ASSERT_EQ_SIZE(2, result.count);
  lore_match_free(&result);
  lore_entry_free(&e1);
  lore_entry_free(&e2);
  lore_entry_free(&e3);
  lorebook_free(&lb);
  PASS();
}

TEST(lore_position_conversion) {
  ASSERT_EQ_INT(LORE_POS_BEFORE_CHAR, lore_position_from_string("before_char"));
  ASSERT_EQ_INT(LORE_POS_AFTER_CHAR, lore_position_from_string("after_char"));
  ASSERT_EQ_INT(LORE_POS_AT_DEPTH, lore_position_from_string("at_depth"));
  ASSERT_EQ_STR("before_char", lore_position_to_string(LORE_POS_BEFORE_CHAR));
  ASSERT_EQ_STR("after_char", lore_position_to_string(LORE_POS_AFTER_CHAR));
  ASSERT_EQ_STR("at_depth", lore_position_to_string(LORE_POS_AT_DEPTH));
  PASS();
}

TEST(lore_role_conversion) {
  ASSERT_EQ_INT(LORE_ROLE_SYSTEM, lore_role_from_string("system"));
  ASSERT_EQ_INT(LORE_ROLE_USER, lore_role_from_string("user"));
  ASSERT_EQ_INT(LORE_ROLE_ASSISTANT, lore_role_from_string("assistant"));
  ASSERT_EQ_STR("system", lore_role_to_string(LORE_ROLE_SYSTEM));
  ASSERT_EQ_STR("user", lore_role_to_string(LORE_ROLE_USER));
  ASSERT_EQ_STR("assistant", lore_role_to_string(LORE_ROLE_ASSISTANT));
  PASS();
}

TEST(lorebook_load_json_basic) {
  Lorebook lb;
  lorebook_init(&lb);
  bool loaded = lorebook_load_json(&lb, "tests/data/test_lorebook.json");
  ASSERT(loaded);
  ASSERT_EQ_STR("Fantasy Lore", lb.name);
  ASSERT_EQ_SIZE(3, lb.entry_count);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_load_json_entries) {
  Lorebook lb;
  lorebook_init(&lb);
  ASSERT(lorebook_load_json(&lb, "tests/data/test_lorebook.json"));
  LoreEntry *dragon = lorebook_get_entry(&lb, 1);
  ASSERT_NOT_NULL(dragon);
  ASSERT_EQ_SIZE(3, dragon->key_count);
  ASSERT_EQ_STR("dragon", dragon->keys[0]);
  ASSERT_EQ_STR("wyrm", dragon->keys[1]);
  ASSERT_EQ_STR("drake", dragon->keys[2]);
  ASSERT(dragon->match_whole_words);
  ASSERT(!dragon->case_sensitive);
  LoreEntry *tavern = lorebook_get_entry(&lb, 3);
  ASSERT_NOT_NULL(tavern);
  ASSERT(tavern->selective);
  ASSERT_EQ_SIZE(2, tavern->key_secondary_count);
  ASSERT_EQ_STR("ale", tavern->keys_secondary[0]);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_load_json_nonexistent) {
  Lorebook lb;
  lorebook_init(&lb);
  bool loaded = lorebook_load_json(&lb, "nonexistent.json");
  ASSERT(!loaded);
  ASSERT_EQ_SIZE(0, lb.entry_count);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_load_json_matching) {
  Lorebook lb;
  lorebook_init(&lb);
  ASSERT(lorebook_load_json(&lb, "tests/data/test_lorebook.json"));
  LoreMatchResult result;
  lore_match_init(&result);
  lorebook_find_matches(&lb, "I met a dragon at the old castle", &result);
  ASSERT_EQ_SIZE(1, result.count);
  ASSERT_EQ_INT(1, result.entries[0]->uid);
  lore_match_free(&result);
  lore_match_init(&result);
  lorebook_find_matches(&lb, "The tavern served excellent ale", &result);
  ASSERT_EQ_SIZE(1, result.count);
  ASSERT_EQ_INT(3, result.entries[0]->uid);
  lore_match_free(&result);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_load_sillytavern_format) {
  Lorebook lb;
  lorebook_init(&lb);
  bool loaded = lorebook_load_json(&lb, "tests/data/honkai.json");
  ASSERT(loaded);
  ASSERT_EQ_STR("Honkai Star Rail", lb.name);
  ASSERT_EQ_SIZE(63, lb.entry_count);
  ASSERT(lb.recursive_scanning);
  lorebook_free(&lb);
  PASS();
}

TEST(lorebook_sillytavern_matching) {
  Lorebook lb;
  lorebook_init(&lb);
  ASSERT(lorebook_load_json(&lb, "tests/data/honkai.json"));
  LoreMatchResult result;
  lore_match_init(&result);
  lorebook_find_matches(&lb, "I heard about the Astral Express", &result);
  ASSERT(result.count >= 1);
  bool found_astral = false;
  for (size_t i = 0; i < result.count; i++) {
    if (strstr(result.entries[i]->content, "Astral Express"))
      found_astral = true;
  }
  ASSERT(found_astral);
  lore_match_free(&result);
  lore_match_init(&result);
  lorebook_find_matches(&lb, "Kafka used her Spirit Whisper", &result);
  ASSERT(result.count >= 1);
  lore_match_free(&result);
  lorebook_free(&lb);
  PASS();
}

void run_lorebook_tests(void) {
  TEST_SUITE("Lorebook Tests");
  RUN_TEST(lorebook_init_free);
  RUN_TEST(lorebook_init_null_safe);
  RUN_TEST(lore_entry_init_free);
  RUN_TEST(lore_entry_add_keys);
  RUN_TEST(lore_entry_add_secondary_keys);
  RUN_TEST(lore_entry_set_content);
  RUN_TEST(lorebook_add_entry);
  RUN_TEST(lorebook_add_multiple_entries);
  RUN_TEST(lorebook_remove_entry);
  RUN_TEST(lorebook_toggle_entry);
  RUN_TEST(lore_entry_matches_simple);
  RUN_TEST(lore_entry_matches_multiple_keys);
  RUN_TEST(lore_entry_matches_case_insensitive);
  RUN_TEST(lore_entry_matches_case_sensitive);
  RUN_TEST(lore_entry_matches_whole_words);
  RUN_TEST(lore_entry_matches_selective);
  RUN_TEST(lore_entry_matches_constant);
  RUN_TEST(lore_entry_matches_disabled);
  RUN_TEST(lorebook_find_matches);
  RUN_TEST(lore_position_conversion);
  RUN_TEST(lore_role_conversion);
  RUN_TEST(lorebook_load_json_basic);
  RUN_TEST(lorebook_load_json_entries);
  RUN_TEST(lorebook_load_json_nonexistent);
  RUN_TEST(lorebook_load_json_matching);
  RUN_TEST(lorebook_load_sillytavern_format);
  RUN_TEST(lorebook_sillytavern_matching);
}

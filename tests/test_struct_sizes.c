#include "character/character.h"
#include "character/persona.h"
#include "chat/history.h"
#include "llm/sampler.h"
#include "test_framework.h"
#include "ui/modal.h"

#define MAX_SAFE_STACK_SIZE (4 * 1024 * 1024)

TEST(struct_size_custom_sampler) {
  size_t size = sizeof(CustomSampler);
  printf("    CustomSampler: %zu bytes (%.1f KB)\n", size, (double)size / 1024);
  ASSERT(size < 64 * 1024);
  PASS();
}

TEST(struct_size_sampler_settings) {
  size_t size = sizeof(SamplerSettings);
  printf("    SamplerSettings: %zu bytes (%.2f MB)\n", size,
         (double)size / (1024 * 1024));
  ASSERT(size < MAX_SAFE_STACK_SIZE);
  PASS();
}

TEST(struct_size_chat_history) {
  size_t size = sizeof(ChatHistory);
  printf("    ChatHistory: %zu bytes\n", size);
  ASSERT(size < MAX_SAFE_STACK_SIZE);
  PASS();
}

TEST(struct_size_persona) {
  size_t size = sizeof(Persona);
  printf("    Persona: %zu bytes (%.2f MB)\n", size,
         (double)size / (1024 * 1024));
  ASSERT(size < MAX_SAFE_STACK_SIZE);
  PASS();
}

TEST(struct_size_character_card) {
  size_t size = sizeof(CharacterCard);
  printf("    CharacterCard: %zu bytes\n", size);
  ASSERT(size < MAX_SAFE_STACK_SIZE);
  PASS();
}

TEST(struct_size_modal) {
  size_t size = sizeof(Modal);
  printf("    Modal: %zu bytes (%.2f MB)\n", size,
         (double)size / (1024 * 1024));
  ASSERT(size < MAX_SAFE_STACK_SIZE);
  PASS();
}

TEST(struct_size_combined_stack_usage) {
  size_t total = sizeof(SamplerSettings) + sizeof(ChatHistory) +
                 sizeof(Persona) + sizeof(CharacterCard) + sizeof(Modal);
  printf("    Combined stack structs: %zu bytes (%.2f MB)\n", total,
         (double)total / (1024 * 1024));
  ASSERT(total < 7 * 1024 * 1024);
  PASS();
}

void run_struct_size_tests(void) {
  TEST_SUITE("Struct Size Safety");
  RUN_TEST(struct_size_custom_sampler);
  RUN_TEST(struct_size_sampler_settings);
  RUN_TEST(struct_size_chat_history);
  RUN_TEST(struct_size_persona);
  RUN_TEST(struct_size_character_card);
  RUN_TEST(struct_size_modal);
  RUN_TEST(struct_size_combined_stack_usage);
}

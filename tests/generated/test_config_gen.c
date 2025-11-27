#include "core/config.h"
#include "test_framework.h"
#include <stdlib.h>
#include <string.h>

TEST(config_add_1_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 1; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(1, mf.count);
  for (int i = 0; i < 1; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_2_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 2; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(2, mf.count);
  for (int i = 0; i < 2; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_3_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 3; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(3, mf.count);
  for (int i = 0; i < 3; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_4_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 4; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(4, mf.count);
  for (int i = 0; i < 4; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_5_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 5; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(5, mf.count);
  for (int i = 0; i < 5; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_6_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 6; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(6, mf.count);
  for (int i = 0; i < 6; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_7_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 7; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(7, mf.count);
  for (int i = 0; i < 7; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_8_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 8; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(8, mf.count);
  for (int i = 0; i < 8; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_9_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 9; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(9, mf.count);
  for (int i = 0; i < 9; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_10_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 10; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(10, mf.count);
  for (int i = 0; i < 10; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_11_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 11; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(11, mf.count);
  for (int i = 0; i < 11; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_12_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 12; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(12, mf.count);
  for (int i = 0; i < 12; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_13_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 13; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(13, mf.count);
  for (int i = 0; i < 13; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_14_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 14; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(14, mf.count);
  for (int i = 0; i < 14; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_15_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 15; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(15, mf.count);
  for (int i = 0; i < 15; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_16_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 16; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(16, mf.count);
  for (int i = 0; i < 16; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_17_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 17; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(17, mf.count);
  for (int i = 0; i < 17; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_18_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 18; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(18, mf.count);
  for (int i = 0; i < 18; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_19_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 19; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(19, mf.count);
  for (int i = 0; i < 19; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_20_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 20; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(20, mf.count);
  for (int i = 0; i < 20; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_21_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 21; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(21, mf.count);
  for (int i = 0; i < 21; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_22_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 22; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(22, mf.count);
  for (int i = 0; i < 22; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_23_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 23; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(23, mf.count);
  for (int i = 0; i < 23; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_24_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 24; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(24, mf.count);
  for (int i = 0; i < 24; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_25_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 25; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(25, mf.count);
  for (int i = 0; i < 25; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_26_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 26; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(26, mf.count);
  for (int i = 0; i < 26; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_27_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 27; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(27, mf.count);
  for (int i = 0; i < 27; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_28_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 28; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(28, mf.count);
  for (int i = 0; i < 28; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_29_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 29; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(29, mf.count);
  for (int i = 0; i < 29; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_30_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 30; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(30, mf.count);
  for (int i = 0; i < 30; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_31_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 31; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(31, mf.count);
  for (int i = 0; i < 31; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_add_32_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 32; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    snprintf(m.base_url, sizeof(m.base_url), "http://localhost:%d", 8000 + i);
    m.context_length = 4096 + i;
    m.api_type = i % 7;
    ASSERT_TRUE(config_add_model(&mf, &m));
  }
  ASSERT_EQ_SIZE(32, mf.count);
  for (int i = 0; i < 32; i++) {
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %d", i);
    ASSERT_EQ_INT(0, strcmp(mf.models[i].name, expected));
  }
  PASS();
}

TEST(config_remove_all_1_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 1; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 1; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_2_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 2; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 2; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_3_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 3; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 3; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_4_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 4; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 4; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_5_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 5; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 5; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_6_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 6; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 6; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_7_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 7; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 7; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_8_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 8; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 8; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_9_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 9; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 9; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_10_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 10; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 10; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_11_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 11; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 11; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_12_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 12; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 12; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_13_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 13; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 13; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_14_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 14; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 14; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_15_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 15; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 15; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_16_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 16; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 16; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_17_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 17; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 17; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_18_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 18; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 18; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_19_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 19; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 19; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_remove_all_20_models) {
  ModelsFile mf = {0};
  for (int i = 0; i < 20; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (int i = 0; i < 20; i++) {
    ASSERT_TRUE(config_remove_model(&mf, 0));
  }
  ASSERT_EQ_SIZE(0, mf.count);
  PASS();
}

TEST(config_api_type_roundtrip) {
  for (int t = 0; t < API_TYPE_COUNT; t++) {
    const char *name = api_type_name((ApiType)t);
    ASSERT_NOT_NULL(name);
    ApiType parsed = api_type_from_name(name);
    ASSERT_EQ_INT(t, (int)parsed);
  }
  PASS();
}

TEST(config_active_index_tracking) {
  ModelsFile mf = {0};
  for (int i = 0; i < 10; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  for (size_t i = 0; i < 10; i++) {
    config_set_active(&mf, i);
    ASSERT_EQ_SIZE(i, mf.active_index);
    ModelConfig *active = config_get_active(&mf);
    ASSERT_NOT_NULL(active);
    char expected[64];
    snprintf(expected, sizeof(expected), "Model %zu", i);
    ASSERT_EQ_INT(0, strcmp(active->name, expected));
  }
  PASS();
}

TEST(config_remove_updates_active) {
  ModelsFile mf = {0};
  for (int i = 0; i < 5; i++) {
    ModelConfig m = {0};
    snprintf(m.name, sizeof(m.name), "Model %d", i);
    config_add_model(&mf, &m);
  }
  config_set_active(&mf, 2);
  config_remove_model(&mf, 0);
  ASSERT_EQ_SIZE(1, mf.active_index);
  config_remove_model(&mf, 0);
  ASSERT_EQ_SIZE(0, mf.active_index);
  PASS();
}

TEST(config_context_length_values) {
  int values[] = {1,    128,  256,   512,   1024,  2048,
                  4096, 8192, 16384, 32768, 65536, 131072};
  ModelsFile mf = {0};
  for (size_t i = 0; i < 12; i++) {
    ModelConfig m = {0};
    m.context_length = values[i];
    snprintf(m.name, sizeof(m.name), "Ctx%d", values[i]);
    config_add_model(&mf, &m);
    ASSERT_EQ_INT(values[i], mf.models[i].context_length);
  }
  PASS();
}

void run_config_generated_tests(void) {
  TEST_SUITE("Config Generated Tests");
  RUN_TEST(config_add_1_models);
  RUN_TEST(config_add_2_models);
  RUN_TEST(config_add_3_models);
  RUN_TEST(config_add_4_models);
  RUN_TEST(config_add_5_models);
  RUN_TEST(config_add_6_models);
  RUN_TEST(config_add_7_models);
  RUN_TEST(config_add_8_models);
  RUN_TEST(config_add_9_models);
  RUN_TEST(config_add_10_models);
  RUN_TEST(config_add_11_models);
  RUN_TEST(config_add_12_models);
  RUN_TEST(config_add_13_models);
  RUN_TEST(config_add_14_models);
  RUN_TEST(config_add_15_models);
  RUN_TEST(config_add_16_models);
  RUN_TEST(config_add_17_models);
  RUN_TEST(config_add_18_models);
  RUN_TEST(config_add_19_models);
  RUN_TEST(config_add_20_models);
  RUN_TEST(config_add_21_models);
  RUN_TEST(config_add_22_models);
  RUN_TEST(config_add_23_models);
  RUN_TEST(config_add_24_models);
  RUN_TEST(config_add_25_models);
  RUN_TEST(config_add_26_models);
  RUN_TEST(config_add_27_models);
  RUN_TEST(config_add_28_models);
  RUN_TEST(config_add_29_models);
  RUN_TEST(config_add_30_models);
  RUN_TEST(config_add_31_models);
  RUN_TEST(config_add_32_models);
  RUN_TEST(config_remove_all_1_models);
  RUN_TEST(config_remove_all_2_models);
  RUN_TEST(config_remove_all_3_models);
  RUN_TEST(config_remove_all_4_models);
  RUN_TEST(config_remove_all_5_models);
  RUN_TEST(config_remove_all_6_models);
  RUN_TEST(config_remove_all_7_models);
  RUN_TEST(config_remove_all_8_models);
  RUN_TEST(config_remove_all_9_models);
  RUN_TEST(config_remove_all_10_models);
  RUN_TEST(config_remove_all_11_models);
  RUN_TEST(config_remove_all_12_models);
  RUN_TEST(config_remove_all_13_models);
  RUN_TEST(config_remove_all_14_models);
  RUN_TEST(config_remove_all_15_models);
  RUN_TEST(config_remove_all_16_models);
  RUN_TEST(config_remove_all_17_models);
  RUN_TEST(config_remove_all_18_models);
  RUN_TEST(config_remove_all_19_models);
  RUN_TEST(config_remove_all_20_models);
  RUN_TEST(config_api_type_roundtrip);
  RUN_TEST(config_active_index_tracking);
  RUN_TEST(config_remove_updates_active);
  RUN_TEST(config_context_length_values);
}

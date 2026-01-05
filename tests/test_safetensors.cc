#include "test_framework.h"
#include <cstring>

#define SAFETENSORS_CPP_IMPLEMENTATION
#include "inference/model_loader/safetensors.hh"

static const char *get_test_file_path(void) {
  return "tests/hadamard.safetensors";
}

static const char *get_optional_model_path(void) {
  static char path[512];
  const char *home = getenv("HOME");
  if (!home) {
    return nullptr;
  }
  snprintf(path, sizeof(path), "%s/models/Qwen3-0.6B/model.safetensors", home);
  return path;
}

TEST(safetensors_load_from_file_basic) {
  const char *path = get_test_file_path();

  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::load_from_file(path, &st, &warn, &err);
  ASSERT_TRUE(ret);
  ASSERT(st.tensors.size() > 0);
  ASSERT(st.header_size > 0);

  PASS();
}

TEST(safetensors_mmap_from_file) {
  const char *path = get_test_file_path();

  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::mmap_from_file(path, &st, &warn, &err);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(st.mmaped);
  ASSERT_NOT_NULL(st.databuffer_addr);
  ASSERT(st.databuffer_size > 0);
  ASSERT(st.tensors.size() > 0);

  PASS();
}

TEST(safetensors_validate_data_offsets) {
  const char *path = get_test_file_path();

  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::mmap_from_file(path, &st, &warn, &err);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(safetensors::validate_data_offsets(st, err));
  ASSERT_EQ_SIZE(0, err.size());

  PASS();
}

TEST(safetensors_tensor_access) {
  const char *path = get_test_file_path();

  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::mmap_from_file(path, &st, &warn, &err);
  ASSERT_TRUE(ret);
  ASSERT(st.tensors.size() > 0);

  for (size_t i = 0; i < st.tensors.size(); i++) {
    safetensors::tensor_t tensor;
    st.tensors.at(i, &tensor);

    ASSERT(tensor.shape.size() > 0);
    ASSERT(tensor.data_offsets[1] >= tensor.data_offsets[0]);
  }

  PASS();
}

TEST(safetensors_get_shape_size) {
  safetensors::tensor_t t;
  t.shape = {10, 20, 30};

  size_t size = safetensors::get_shape_size(t);
  ASSERT_EQ_SIZE(6000, size);

  PASS();
}

TEST(safetensors_get_shape_size_empty) {
  safetensors::tensor_t t;
  t.shape = {};

  size_t size = safetensors::get_shape_size(t);
  ASSERT_EQ_SIZE(1, size);

  PASS();
}

TEST(safetensors_get_shape_size_zero) {
  safetensors::tensor_t t;
  t.shape = {10, 0, 30};

  size_t size = safetensors::get_shape_size(t);
  ASSERT_EQ_SIZE(0, size);

  PASS();
}

TEST(safetensors_get_dtype_bytes) {
  ASSERT_EQ_SIZE(1, safetensors::get_dtype_bytes(safetensors::dtype::kBOOL));
  ASSERT_EQ_SIZE(1, safetensors::get_dtype_bytes(safetensors::dtype::kUINT8));
  ASSERT_EQ_SIZE(1, safetensors::get_dtype_bytes(safetensors::dtype::kINT8));
  ASSERT_EQ_SIZE(2, safetensors::get_dtype_bytes(safetensors::dtype::kINT16));
  ASSERT_EQ_SIZE(2, safetensors::get_dtype_bytes(safetensors::dtype::kUINT16));
  ASSERT_EQ_SIZE(2, safetensors::get_dtype_bytes(safetensors::dtype::kFLOAT16));
  ASSERT_EQ_SIZE(2, safetensors::get_dtype_bytes(safetensors::dtype::kBFLOAT16));
  ASSERT_EQ_SIZE(4, safetensors::get_dtype_bytes(safetensors::dtype::kINT32));
  ASSERT_EQ_SIZE(4, safetensors::get_dtype_bytes(safetensors::dtype::kUINT32));
  ASSERT_EQ_SIZE(4, safetensors::get_dtype_bytes(safetensors::dtype::kFLOAT32));
  ASSERT_EQ_SIZE(8, safetensors::get_dtype_bytes(safetensors::dtype::kFLOAT64));
  ASSERT_EQ_SIZE(8, safetensors::get_dtype_bytes(safetensors::dtype::kINT64));
  ASSERT_EQ_SIZE(8, safetensors::get_dtype_bytes(safetensors::dtype::kUINT64));

  PASS();
}

TEST(safetensors_get_dtype_str) {
  std::string s;
  
  s = safetensors::get_dtype_str(safetensors::dtype::kBOOL);
  ASSERT_EQ_STR("BOOL", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kUINT8);
  ASSERT_EQ_STR("U8", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kINT8);
  ASSERT_EQ_STR("I8", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kINT16);
  ASSERT_EQ_STR("I16", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kUINT16);
  ASSERT_EQ_STR("U16", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kFLOAT16);
  ASSERT_EQ_STR("F16", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kBFLOAT16);
  ASSERT_EQ_STR("BF16", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kINT32);
  ASSERT_EQ_STR("I32", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kUINT32);
  ASSERT_EQ_STR("U32", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kFLOAT32);
  ASSERT_EQ_STR("F32", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kFLOAT64);
  ASSERT_EQ_STR("F64", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kINT64);
  ASSERT_EQ_STR("I64", s.c_str());
  s = safetensors::get_dtype_str(safetensors::dtype::kUINT64);
  ASSERT_EQ_STR("U64", s.c_str());

  PASS();
}

TEST(safetensors_bfloat16_conversion) {
  float original = 3.14159f;
  uint16_t bf16 = safetensors::float_to_bfloat16(original);
  float converted = safetensors::bfloat16_to_float(bf16);

  ASSERT_NEAR(original, converted, 0.01);

  PASS();
}

TEST(safetensors_bfloat16_zero) {
  float original = 0.0f;
  uint16_t bf16 = safetensors::float_to_bfloat16(original);
  float converted = safetensors::bfloat16_to_float(bf16);

  ASSERT_EQ(0.0, (double)converted);

  PASS();
}

TEST(safetensors_fp16_conversion) {
  float original = 2.71828f;
  uint16_t fp16 = safetensors::float_to_fp16(original);
  float converted = safetensors::fp16_to_float(fp16);

  ASSERT_NEAR(original, converted, 0.01);

  PASS();
}

TEST(safetensors_load_nonexistent_file) {
  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::load_from_file("/nonexistent/path/model.safetensors", 
                                         &st, &warn, &err);

  ASSERT_FALSE(ret);
  ASSERT(err.size() > 0);

  PASS();
}

TEST(safetensors_tensor_data_access) {
  const char *path = get_test_file_path();

  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::mmap_from_file(path, &st, &warn, &err);
  ASSERT_TRUE(ret);
  ASSERT(st.tensors.size() > 0);

  safetensors::tensor_t tensor;
  st.tensors.at(0, &tensor);

  ASSERT_NOT_NULL(st.databuffer_addr);
  ASSERT(tensor.data_offsets[0] < st.databuffer_size);
  ASSERT(tensor.data_offsets[1] <= st.databuffer_size);

  const uint8_t *data = st.databuffer_addr + tensor.data_offsets[0];
  ASSERT_NOT_NULL(data);

  PASS();
}

TEST(safetensors_tensor_ordering) {
  const char *path = get_test_file_path();

  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::mmap_from_file(path, &st, &warn, &err);
  ASSERT_TRUE(ret);
  ASSERT(st.tensors.size() > 0);

  for (size_t i = 0; i < st.tensors.size(); i++) {
    std::string key = st.tensors.keys()[i];
    ASSERT(key.size() > 0);

    safetensors::tensor_t tensor;
    st.tensors.at(i, &tensor);
    ASSERT(tensor.shape.size() > 0);
  }

  PASS();
}

TEST(safetensors_large_model_optional) {
  const char *path = get_optional_model_path();
  if (!path) {
    printf("(skipped - no large model) ");
    PASS();
  }

  safetensors::safetensors_t st;
  std::string warn, err;

  bool ret = safetensors::mmap_from_file(path, &st, &warn, &err);
  if (!ret) {
    printf("(skipped - model not found) ");
    PASS();
  }

  ASSERT_TRUE(ret);
  ASSERT(st.tensors.size() > 100);

  PASS();
}

extern "C" {
void run_safetensors_tests(void) {
  TEST_SUITE("Safetensors Loader");
  RUN_TEST(safetensors_load_from_file_basic);
  RUN_TEST(safetensors_mmap_from_file);
  RUN_TEST(safetensors_validate_data_offsets);
  RUN_TEST(safetensors_tensor_access);
  RUN_TEST(safetensors_get_shape_size);
  RUN_TEST(safetensors_get_shape_size_empty);
  RUN_TEST(safetensors_get_shape_size_zero);
  RUN_TEST(safetensors_get_dtype_bytes);
  RUN_TEST(safetensors_get_dtype_str);
  RUN_TEST(safetensors_bfloat16_conversion);
  RUN_TEST(safetensors_bfloat16_zero);
  RUN_TEST(safetensors_fp16_conversion);
  RUN_TEST(safetensors_load_nonexistent_file);
  RUN_TEST(safetensors_tensor_data_access);
  RUN_TEST(safetensors_tensor_ordering);
  RUN_TEST(safetensors_large_model_optional);
}
}

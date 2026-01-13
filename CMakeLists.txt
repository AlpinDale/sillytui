cmake_minimum_required(VERSION 3.24)
project(sillytui C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
        message(FATAL_ERROR "GCC version 13.0 or later is required for C++23 support. Found: ${CMAKE_CXX_COMPILER_VERSION}\n"
                "Please upgrade GCC or use Clang 16+")
    endif()
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-stringop-truncation)
endif()

# POSIX feature test macros for nftw and other functions (Linux only)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(_XOPEN_SOURCE=500 _GNU_SOURCE)
endif()

# Enable AVX2 for x86_64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_compile_options(-march=x86-64-v3)
endif()

find_package(Curses REQUIRED)
find_package(CURL REQUIRED)

set(SOURCES
    src/main.c
    src/core/config.c
    src/core/macros.c
    src/core/time.c
    src/core/error.c
    src/core/log.c
    src/inference/core/dtype.c
    src/inference/core/tensor.c
    src/inference/core/error.c
    src/inference/backend/caps.c
    src/inference/backend/registry.c
    src/inference/backend/cpu/scalar/scalar_backend.c
    src/inference/backend/cpu/neon/neon_backend.c
    src/inference/backend/cpu/amx/amx_backend.c
    src/inference/backend/accelerate/accelerate_backend.c
    src/inference/model/config.c
    src/inference/model/common/ffn.c
    src/inference/model/common/attention.c
    src/inference/model/common/transformer.c
    src/ui/ui.c
    src/ui/modal.c
    src/ui/markdown.c
    src/ui/console.c
    src/ui/syntax/syntax_ncurses.c
    src/ui/syntax/c/highlight.c
    src/ui/syntax/c/hashtable.c
    src/chat/chat.c
    src/chat/history.c
    src/chat/author_note.c
    src/llm/llm.c
    src/llm/common.c
    src/llm/sampler.c
    src/llm/backends/openai.c
    src/llm/backends/anthropic.c
    src/llm/backends/kobold.c
    src/character/character.c
    src/character/persona.c
    src/lore/lorebook.c
    src/inference/tokenizer/tiktoken.c
    src/inference/tokenizer/sentencepiece.c
    src/inference/tokenizer/gpt2bpe.c
    src/inference/tokenizer/selector.c
    src/inference/tokenizer/simd.c
    src/inference/tokenizer/unicode_tables.c
    src/inference/kernels/gemm/gemm.c
    src/inference/kernels/gemm/gemm_neon.c
    src/inference/kernels/gemm/gemm_amx.c
    src/inference/kernels/norm/layernorm.c
    src/inference/kernels/norm/layernorm_neon.c
    src/inference/kernels/activation/activation.c
    src/inference/kernels/activation/activation_neon.c
    src/inference/kernels/rope/rope.c
    src/inference/kernels/rope/rope_neon.c
    src/inference/kernels/softmax/softmax.c
    src/inference/kernels/softmax/softmax_neon.c
    src/inference/kernels/attention/attention.c
    src/inference/kernels/attention/attention_neon.c
    src/inference/kernels/embedding/embedding.c
    src/inference/kernels/embedding/embedding_neon.c
    src/inference/kernels/sampling/sampling.c
    src/inference/kernels/sampling/sampling_neon.c
    src/inference/kernels/kv_cache/kv_cache.c
    src/inference/kernels/kv_cache/kv_cache_neon.c
    src/inference/model/base.c
    src/inference/model/registry.c
    src/inference/model/qwen3/weights.c
    src/inference/model/qwen3/qwen3.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    enable_language(ASM)
    list(APPEND SOURCES src/inference/tokenizer/simd_arm64.S)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    enable_language(ASM)
    list(APPEND SOURCES src/inference/tokenizer/simd_x86_64.S)
endif()

add_executable(sillytui ${SOURCES})

target_compile_definitions(sillytui PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(sillytui PRIVATE ${CURSES_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} src)
find_package(Threads REQUIRED)
target_link_libraries(sillytui PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} Threads::Threads)
if(APPLE)
    target_link_libraries(sillytui PRIVATE "-framework Accelerate")
endif()

set(TEST_SOURCES
    tests/run_tests.c
    tests/test_history.c
    tests/test_macros.c
    tests/test_config.c
    tests/test_sampler.c
    tests/test_simd.c
    tests/test_tokenizer.c
    tests/test_modal.c
    tests/test_attachments.c
    tests/safetensors_impl.cc
    tests/test_safetensors.cc
    tests/kernels/test_gemm.cc
    tests/kernels/test_gemm_pytorch_accuracy.cc
    tests/kernels/test_layernorm.cc
    tests/kernels/test_activation.cc
    tests/kernels/test_activation_pytorch_accuracy.cc
    tests/kernels/test_rope.cc
    tests/kernels/test_rope_pytorch_accuracy.cc
    tests/kernels/test_softmax.cc
    tests/kernels/test_softmax_pytorch_accuracy.cc
    tests/kernels/test_attention.cc
    tests/kernels/test_embedding.cc
    tests/kernels/test_embedding_pytorch_accuracy.cc
    tests/kernels/test_sampling.cc
    tests/kernels/test_sampling_pytorch_accuracy.cc
    tests/kernels/test_kv_cache.cc
    tests/kernels/test_kv_cache_pytorch_accuracy.cc
    src/core/config.c
    src/core/macros.c
    src/core/time.c
    src/core/error.c
    src/core/log.c
    src/chat/history.c
    src/chat/author_note.c
    src/llm/sampler.c
    src/llm/common.c
    src/llm/llm.c
    src/llm/backends/openai.c
    src/llm/backends/anthropic.c
    src/llm/backends/kobold.c
    src/inference/tokenizer/tiktoken.c
    src/inference/tokenizer/simd.c
    src/inference/tokenizer/unicode_tables.c
    src/inference/tokenizer/selector.c
    src/inference/tokenizer/gpt2bpe.c
    src/inference/tokenizer/sentencepiece.c
    src/inference/kernels/gemm/gemm.c
    src/inference/kernels/gemm/gemm_neon.c
    src/inference/kernels/gemm/gemm_amx.c
    src/inference/kernels/norm/layernorm.c
    src/inference/kernels/norm/layernorm_neon.c
    src/inference/kernels/activation/activation.c
    src/inference/kernels/activation/activation_neon.c
    src/inference/kernels/rope/rope.c
    src/inference/kernels/rope/rope_neon.c
    src/inference/kernels/softmax/softmax.c
    src/inference/kernels/softmax/softmax_neon.c
    src/inference/kernels/attention/attention.c
    src/inference/kernels/attention/attention_neon.c
    src/inference/kernels/embedding/embedding.c
    src/inference/kernels/embedding/embedding_neon.c
    src/inference/kernels/sampling/sampling.c
    src/inference/kernels/sampling/sampling_neon.c
    src/inference/kernels/kv_cache/kv_cache.c
    src/inference/kernels/kv_cache/kv_cache_neon.c
    src/inference/backend/caps.c
    src/ui/modal.c
    src/ui/ui.c
    src/ui/markdown.c
    src/ui/console.c
    src/ui/syntax/syntax_ncurses.c
    src/ui/syntax/c/highlight.c
    src/ui/syntax/c/hashtable.c
    src/chat/chat.c
    src/character/character.c
    src/character/persona.c
    src/lore/lorebook.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    list(APPEND TEST_SOURCES src/inference/tokenizer/simd_arm64.S)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    list(APPEND TEST_SOURCES src/inference/tokenizer/simd_x86_64.S)
endif()

add_executable(run_tests ${TEST_SOURCES})
target_compile_definitions(run_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(run_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests)
target_link_libraries(run_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m Threads::Threads)
if(APPLE)
    target_link_libraries(run_tests PRIVATE "-framework Accelerate")
endif()
set_source_files_properties(tests/test_safetensors.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")
set_source_files_properties(tests/kernels/test_gemm.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")
set_source_files_properties(tests/kernels/test_gemm_pytorch_accuracy.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")
set_source_files_properties(tests/kernels/test_layernorm.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")
set_source_files_properties(tests/kernels/test_activation.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")
set_source_files_properties(tests/kernels/test_activation_pytorch_accuracy.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")
set_source_files_properties(tests/kernels/test_rope.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")
set_source_files_properties(tests/kernels/test_rope_pytorch_accuracy.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")

enable_testing()
add_test(NAME unit_tests COMMAND run_tests)

set(INTEGRATION_TEST_SOURCES
    tests/integration/run_integration_tests.c
    tests/integration/test_chat_integration.c
    tests/integration/test_config_integration.c
    tests/integration/test_persona_integration.c
    tests/integration/test_tokenizer_integration.c
    src/core/config.c
    src/core/macros.c
    src/core/time.c
    src/core/error.c
    src/core/log.c
    src/chat/chat.c
    src/chat/history.c
    src/chat/author_note.c
    src/llm/sampler.c
    src/character/character.c
    src/character/persona.c
    src/inference/tokenizer/tiktoken.c
    src/inference/tokenizer/gpt2bpe.c
    src/inference/tokenizer/sentencepiece.c
    src/inference/tokenizer/simd.c
    src/inference/tokenizer/unicode_tables.c
    src/inference/tokenizer/selector.c
    src/ui/modal.c
    src/ui/ui.c
    src/ui/markdown.c
    src/ui/console.c
    src/ui/syntax/syntax_ncurses.c
    src/ui/syntax/c/highlight.c
    src/ui/syntax/c/hashtable.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    list(APPEND INTEGRATION_TEST_SOURCES src/inference/tokenizer/simd_arm64.S)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    list(APPEND INTEGRATION_TEST_SOURCES src/inference/tokenizer/simd_x86_64.S)
endif()

add_executable(run_integration_tests ${INTEGRATION_TEST_SOURCES})
target_compile_definitions(run_integration_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(run_integration_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests tests/integration)
target_link_libraries(run_integration_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m Threads::Threads)
if(APPLE)
    target_link_libraries(run_integration_tests PRIVATE "-framework Accelerate")
endif()

add_test(NAME integration_tests COMMAND run_integration_tests)

set(ALL_TEST_SOURCES
    tests/run_all_tests.c
    tests/test_history.c
    tests/test_macros.c
    tests/test_config.c
    tests/test_sampler.c
    tests/test_simd.c
    tests/test_tokenizer.c
    tests/test_modal.c
    tests/test_struct_sizes.c
    tests/test_robustness.c
    tests/test_lorebook.c
    tests/test_tokenizer_selector.c
    tests/safetensors_impl.cc
    tests/test_safetensors.cc
    tests/kernels/test_gemm.cc
    tests/kernels/test_gemm_pytorch_accuracy.cc
    tests/kernels/test_layernorm.cc
    tests/kernels/test_activation.cc
    tests/kernels/test_activation_pytorch_accuracy.cc
    tests/kernels/test_rope.cc
    tests/kernels/test_rope_pytorch_accuracy.cc
    tests/kernels/test_softmax.cc
    tests/kernels/test_softmax_pytorch_accuracy.cc
    tests/kernels/test_attention.cc
    tests/kernels/test_embedding.cc
    tests/kernels/test_embedding_pytorch_accuracy.cc
    tests/kernels/test_sampling.cc
    tests/kernels/test_sampling_pytorch_accuracy.cc
    tests/kernels/test_kv_cache.cc
    tests/kernels/test_kv_cache_pytorch_accuracy.cc
    tests/boundary/test_boundary.c
    tests/stress/test_stress.c
    tests/generated/test_unicode_gen.c
    tests/generated/test_utf8_gen.c
    tests/generated/test_pretokenize_gen.c
    tests/generated/test_history_gen.c
    tests/generated/test_config_gen.c
    tests/generated/test_sampler_gen.c
    tests/generated/test_simd_gen.c
    tests/generated/test_macro_gen.c
    tests/generated/test_edge_gen.c
    tests/property/test_property.c
    tests/integration/test_chat_integration.c
    tests/integration/test_config_integration.c
    tests/integration/test_persona_integration.c
    tests/integration/test_tokenizer_integration.c
    src/core/config.c
    src/core/macros.c
    src/core/time.c
    src/core/error.c
    src/core/log.c
    src/chat/chat.c
    src/chat/history.c
    src/chat/author_note.c
    src/llm/sampler.c
    src/character/character.c
    src/character/persona.c
    src/lore/lorebook.c
    src/inference/tokenizer/tiktoken.c
    src/inference/tokenizer/gpt2bpe.c
    src/inference/tokenizer/sentencepiece.c
    src/inference/tokenizer/selector.c
    src/inference/tokenizer/simd.c
    src/inference/tokenizer/unicode_tables.c
    src/inference/kernels/gemm/gemm.c
    src/inference/kernels/gemm/gemm_neon.c
    src/inference/kernels/gemm/gemm_amx.c
    src/inference/kernels/norm/layernorm.c
    src/inference/kernels/norm/layernorm_neon.c
    src/inference/kernels/activation/activation.c
    src/inference/kernels/activation/activation_neon.c
    src/inference/kernels/rope/rope.c
    src/inference/kernels/rope/rope_neon.c
    src/inference/kernels/softmax/softmax.c
    src/inference/kernels/softmax/softmax_neon.c
    src/inference/kernels/attention/attention.c
    src/inference/kernels/attention/attention_neon.c
    src/inference/kernels/embedding/embedding.c
    src/inference/kernels/embedding/embedding_neon.c
    src/inference/kernels/sampling/sampling.c
    src/inference/kernels/sampling/sampling_neon.c
    src/inference/kernels/kv_cache/kv_cache.c
    src/inference/kernels/kv_cache/kv_cache_neon.c
    src/inference/backend/caps.c
    src/ui/modal.c
    src/ui/ui.c
    src/ui/markdown.c
    src/ui/syntax/syntax_ncurses.c
    src/ui/syntax/c/highlight.c
    src/ui/syntax/c/hashtable.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    list(APPEND ALL_TEST_SOURCES src/inference/tokenizer/simd_arm64.S)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    list(APPEND ALL_TEST_SOURCES src/inference/tokenizer/simd_x86_64.S)
endif()

add_executable(run_all_tests ${ALL_TEST_SOURCES})

target_compile_definitions(run_all_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(run_all_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests tests/boundary tests/stress tests/generated tests/property)
target_link_libraries(run_all_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m Threads::Threads)
if(APPLE)
    target_link_libraries(run_all_tests PRIVATE "-framework Accelerate")
endif()
set_source_files_properties(tests/test_safetensors.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")

add_test(NAME all_tests COMMAND run_all_tests)

option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

add_custom_target(coverage
    COMMAND lcov --capture --directory . --output-file coverage.info
    COMMAND lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
    COMMAND genhtml coverage.info --output-directory coverage_report
    COMMAND echo "Coverage report: coverage_report/index.html"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS run_all_tests
)

add_executable(gen_unicode_tests tests/generators/gen_unicode_tests.c)
add_executable(gen_utf8_tests tests/generators/gen_utf8_tests.c)
add_executable(gen_pretokenize_tests tests/generators/gen_pretokenize_tests.c)

add_custom_target(generate_tests
    COMMAND gen_unicode_tests > ${CMAKE_SOURCE_DIR}/tests/generated/test_unicode_gen.c
    COMMAND gen_utf8_tests > ${CMAKE_SOURCE_DIR}/tests/generated/test_utf8_gen.c
    COMMAND gen_pretokenize_tests > ${CMAKE_SOURCE_DIR}/tests/generated/test_pretokenize_gen.c
    DEPENDS gen_unicode_tests gen_utf8_tests gen_pretokenize_tests
)

add_executable(load_model examples/load_model.cc)
target_include_directories(load_model PRIVATE src)
set_target_properties(load_model PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
target_compile_options(load_model PRIVATE 
    -fno-rtti 
    -fno-exceptions 
    -Wno-ignored-qualifiers 
    -Wno-unused-parameter 
    -Wno-unused-variable
    -Wno-error
)

add_executable(qwen3_inference
    examples/qwen3_inference.c
    src/inference/core/dtype.c
    src/inference/core/tensor.c
    src/inference/model/base.c
    src/inference/model/registry.c
    src/inference/model/config.c
    src/inference/model/common/ffn.c
    src/inference/model/common/attention.c
    src/inference/model/common/transformer.c
    src/inference/model/qwen3/weights.c
    src/inference/model/qwen3/qwen3.c
    src/inference/tokenizer/gpt2bpe.c
    src/inference/tokenizer/simd.c
    src/inference/tokenizer/unicode_tables.c
    src/inference/backend/caps.c
    src/inference/kernels/gemm/gemm.c
    src/inference/kernels/gemm/gemm_neon.c
    src/inference/kernels/gemm/gemm_amx.c
    src/inference/kernels/norm/layernorm.c
    src/inference/kernels/norm/layernorm_neon.c
    src/inference/kernels/activation/activation.c
    src/inference/kernels/activation/activation_neon.c
    src/inference/kernels/rope/rope.c
    src/inference/kernels/rope/rope_neon.c
    src/inference/kernels/embedding/embedding.c
    src/inference/kernels/embedding/embedding_neon.c
    src/inference/kernels/attention/attention.c
    src/inference/kernels/attention/attention_neon.c
    src/inference/kernels/kv_cache/kv_cache.c
    src/inference/kernels/kv_cache/kv_cache_neon.c
    src/inference/kernels/sampling/sampling.c
    src/inference/kernels/sampling/sampling_neon.c
)
set_source_files_properties(src/inference/model/qwen3/weights.c PROPERTIES
    COMPILE_FLAGS "-x c++ -std=c++11"
    LANGUAGE CXX
)
target_include_directories(qwen3_inference PRIVATE src)
target_compile_options(qwen3_inference PRIVATE 
    -Wno-ignored-qualifiers 
    -Wno-unused-parameter 
    -Wno-unused-variable
    -Wno-error
    -std=c11
)
target_link_libraries(qwen3_inference PRIVATE Threads::Threads)
if(APPLE)
    target_link_libraries(qwen3_inference PRIVATE "-framework Accelerate")
endif()
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    target_sources(qwen3_inference PRIVATE src/inference/tokenizer/simd_arm64.S)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    enable_language(ASM)
    target_sources(qwen3_inference PRIVATE src/inference/tokenizer/simd_x86_64.S)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_gemm.c")
  add_executable(bench_gemm bench/bench_gemm.c src/inference/kernels/gemm/gemm.c src/inference/kernels/gemm/gemm_neon.c src/inference/kernels/gemm/gemm_amx.c)
  target_include_directories(bench_gemm PRIVATE src)
  target_compile_options(bench_gemm PRIVATE -O3 -ffast-math)
  target_link_libraries(bench_gemm PRIVATE Threads::Threads)
  if(APPLE)
      target_link_libraries(bench_gemm PRIVATE "-framework Accelerate")
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_layernorm.c")
  add_executable(bench_layernorm bench/bench_layernorm.c src/inference/kernels/norm/layernorm.c src/inference/kernels/norm/layernorm_neon.c src/inference/kernels/gemm/gemm.c src/inference/kernels/gemm/gemm_neon.c src/inference/kernels/gemm/gemm_amx.c)
  target_include_directories(bench_layernorm PRIVATE src)
  target_compile_options(bench_layernorm PRIVATE -O3 -ffast-math)
  target_link_libraries(bench_layernorm PRIVATE Threads::Threads)
  if(APPLE)
      target_link_libraries(bench_layernorm PRIVATE "-framework Accelerate")
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/profile_gemm.c")
  add_executable(profile_gemm bench/profile_gemm.c src/inference/kernels/gemm/gemm.c src/inference/kernels/gemm/gemm_neon.c src/inference/kernels/gemm/gemm_amx.c)
  target_include_directories(profile_gemm PRIVATE src)
  target_compile_options(profile_gemm PRIVATE -O3 -ffast-math -g)
  target_link_libraries(profile_gemm PRIVATE Threads::Threads)
  if(APPLE)
      target_link_libraries(profile_gemm PRIVATE "-framework Accelerate")
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/profile_detailed.c")
  add_executable(profile_detailed bench/profile_detailed.c src/inference/kernels/gemm/gemm.c src/inference/kernels/gemm/gemm_neon.c src/inference/kernels/gemm/gemm_amx.c)
  target_include_directories(profile_detailed PRIVATE src)
  target_compile_options(profile_detailed PRIVATE -O3 -ffast-math -g)
  target_link_libraries(profile_detailed PRIVATE Threads::Threads)
  if(APPLE)
      target_link_libraries(profile_detailed PRIVATE "-framework Accelerate")
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_activation.c")
  add_executable(bench_activation bench/bench_activation.c src/inference/kernels/activation/activation.c src/inference/kernels/activation/activation_neon.c)
  target_include_directories(bench_activation PRIVATE src)
  target_compile_options(bench_activation PRIVATE -O3 -ffast-math)
  target_link_libraries(bench_activation PRIVATE Threads::Threads m)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_rope.c")
  add_executable(bench_rope bench/bench_rope.c src/inference/kernels/rope/rope.c src/inference/kernels/rope/rope_neon.c)
  target_include_directories(bench_rope PRIVATE src)
  target_compile_options(bench_rope PRIVATE -O3 -ffast-math)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_softmax.c")
  add_executable(bench_softmax bench/bench_softmax.c src/inference/kernels/softmax/softmax.c src/inference/kernels/softmax/softmax_neon.c)
  target_include_directories(bench_softmax PRIVATE src)
  target_compile_options(bench_softmax PRIVATE -O3 -ffast-math)
  if(APPLE)
    target_link_libraries(bench_softmax PRIVATE "-framework Accelerate")
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_attention.c")
  add_executable(bench_attention bench/bench_attention.c src/inference/kernels/attention/attention.c src/inference/kernels/attention/attention_neon.c)
  target_include_directories(bench_attention PRIVATE src)
  target_compile_options(bench_attention PRIVATE -O3 -ffast-math)
  if(APPLE)
    target_link_libraries(bench_attention PRIVATE "-framework Accelerate")
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_embedding.c")
  add_executable(bench_embedding bench/bench_embedding.c src/inference/kernels/embedding/embedding.c src/inference/kernels/embedding/embedding_neon.c)
  target_include_directories(bench_embedding PRIVATE src)
  target_compile_options(bench_embedding PRIVATE -O3 -ffast-math)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_sampling.c")
  add_executable(bench_sampling bench/bench_sampling.c src/inference/kernels/sampling/sampling.c src/inference/kernels/sampling/sampling_neon.c)
  target_include_directories(bench_sampling PRIVATE src)
  target_compile_options(bench_sampling PRIVATE -O3 -ffast-math)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/bench/bench_kv_cache.c")
  add_executable(bench_kv_cache bench/bench_kv_cache.c src/inference/kernels/kv_cache/kv_cache.c src/inference/kernels/kv_cache/kv_cache_neon.c)
  target_include_directories(bench_kv_cache PRIVATE src)
  target_compile_options(bench_kv_cache PRIVATE -O3 -ffast-math)
endif()

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

add_compile_options(-Wall -Wextra -Werror)

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

set(ULIGHT_SOURCES
    src/ui/syntax/ulight/chars.cpp
    src/ui/syntax/ulight/io.cpp
    src/ui/syntax/ulight/parse_utils.cpp
    src/ui/syntax/ulight/ulight.cpp
    src/ui/syntax/ulight/lang/bash.cpp
    src/ui/syntax/ulight/lang/cowel.cpp
    src/ui/syntax/ulight/lang/cpp.cpp
    src/ui/syntax/ulight/lang/css.cpp
    src/ui/syntax/ulight/lang/diff.cpp
    src/ui/syntax/ulight/lang/ebnf.cpp
    src/ui/syntax/ulight/lang/html.cpp
    src/ui/syntax/ulight/lang/js.cpp
    src/ui/syntax/ulight/lang/json.cpp
    src/ui/syntax/ulight/lang/kotlin.cpp
    src/ui/syntax/ulight/lang/llvm.cpp
    src/ui/syntax/ulight/lang/lua.cpp
    src/ui/syntax/ulight/lang/nasm.cpp
    src/ui/syntax/ulight/lang/python.cpp
    src/ui/syntax/ulight/lang/rust.cpp
    src/ui/syntax/ulight/lang/tex.cpp
    src/ui/syntax/ulight/lang/xml.cpp
)

set(SOURCES
    src/main.c
    src/core/config.c
    src/core/macros.c
    src/core/time.c
    src/core/error.c
    src/core/log.c
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
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    enable_language(ASM)
    list(APPEND SOURCES src/inference/tokenizer/simd_arm64.S)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    enable_language(ASM)
    list(APPEND SOURCES src/inference/tokenizer/simd_x86_64.S)
endif()

add_executable(sillytui ${SOURCES} ${ULIGHT_SOURCES})

set_source_files_properties(${ULIGHT_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-trigraphs")

target_compile_definitions(sillytui PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(sillytui PRIVATE ${CURSES_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} src src/ui/syntax/ulight/include)
target_link_libraries(sillytui PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES})

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
    tests/test_safetensors.cc
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

add_executable(run_tests ${TEST_SOURCES} ${ULIGHT_SOURCES})
target_compile_definitions(run_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(run_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests src/ui/syntax/ulight/include)
target_link_libraries(run_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m)
set_source_files_properties(tests/test_safetensors.cc PROPERTIES COMPILE_FLAGS "-Wno-error -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable")

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

add_executable(run_integration_tests ${INTEGRATION_TEST_SOURCES} ${ULIGHT_SOURCES})
target_compile_definitions(run_integration_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(run_integration_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests tests/integration src/ui/syntax/ulight/include)
target_link_libraries(run_integration_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m)

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
    tests/test_safetensors.cc
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

add_executable(run_all_tests ${ALL_TEST_SOURCES} ${ULIGHT_SOURCES})

set_source_files_properties(${ULIGHT_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-trigraphs")

target_compile_definitions(run_all_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(run_all_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests tests/boundary tests/stress tests/generated tests/property src/ui/syntax/ulight/include)
target_link_libraries(run_all_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m)
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

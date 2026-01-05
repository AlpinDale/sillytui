cmake_minimum_required(VERSION 3.24)
project(sillytui C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Werror)

# POSIX feature test macros for nftw and other functions (Linux only)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(_XOPEN_SOURCE=500 _GNU_SOURCE)
endif()

# Enable AVX2 for x86_64 (not on Windows, different ABI)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64" AND NOT WIN32)
    add_compile_options(-march=x86-64-v3)
endif()

# Curses library setup
if(WIN32)
    # Build PDCursesMod VT backend manually
    file(GLOB PDCURSES_CORE_SOURCES 
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/pdcurses/*.c")
    set(PDCURSES_VT_SOURCES
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt/pdcclip.c"
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt/pdcdisp.c"
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt/pdcgetsc.c"
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt/pdckbd.c"
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt/pdcscrn.c"
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt/pdcsetsc.c"
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt/pdcutil.c"
    )
    
    add_library(pdcurses_vt STATIC ${PDCURSES_CORE_SOURCES} ${PDCURSES_VT_SOURCES})
    target_include_directories(pdcurses_vt PUBLIC 
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod"
        "${CMAKE_SOURCE_DIR}/third-party/PDCursesMod/vt"
    )
    target_compile_definitions(pdcurses_vt PUBLIC PDC_FORCE_UTF8)
    
    set(CURSES_LIBRARIES pdcurses_vt)
    set(CURSES_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third-party/PDCursesMod)
else()
    find_package(Curses REQUIRED)
endif()

if(WIN32)
    # For Windows cross-compile, CURL is optional (can be added later)
    find_package(CURL)
    if(CURL_FOUND)
        add_compile_definitions(CURL_FOUND=1)
    else()
        message(WARNING "CURL not found - LLM API features will not work")
        set(CURL_LIBRARIES "")
        set(CURL_INCLUDE_DIRS "")
    endif()
else()
    find_package(CURL REQUIRED)
    add_compile_definitions(CURL_FOUND=1)
endif()

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
    src/tokenizer/tiktoken.c
    src/tokenizer/sentencepiece.c
    src/tokenizer/gpt2bpe.c
    src/tokenizer/selector.c
    src/tokenizer/simd.c
    src/tokenizer/unicode_tables.c
)

# SIMD assembly (not on Windows - different ABI, use C fallbacks)
if(NOT WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        enable_language(ASM)
        list(APPEND SOURCES src/tokenizer/simd_arm64.S)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        enable_language(ASM)
        list(APPEND SOURCES src/tokenizer/simd_x86_64.S)
    endif()
endif()

add_executable(sillytui ${SOURCES} ${ULIGHT_SOURCES})

set_source_files_properties(${ULIGHT_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-trigraphs")

if(WIN32)
    target_compile_definitions(sillytui PRIVATE PDC_FORCE_UTF8)
else()
    target_compile_definitions(sillytui PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
endif()
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
    src/tokenizer/tiktoken.c
    src/tokenizer/simd.c
    src/tokenizer/unicode_tables.c
    src/tokenizer/selector.c
    src/tokenizer/gpt2bpe.c
    src/tokenizer/sentencepiece.c
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

if(NOT WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        list(APPEND TEST_SOURCES src/tokenizer/simd_arm64.S)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        list(APPEND TEST_SOURCES src/tokenizer/simd_x86_64.S)
    endif()
endif()

if(NOT WIN32)
add_executable(run_tests ${TEST_SOURCES} ${ULIGHT_SOURCES})
if(WIN32)
    target_compile_definitions(run_tests PRIVATE PDC_FORCE_UTF8)
else()
    target_compile_definitions(run_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
endif()
target_include_directories(run_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests src/ui/syntax/ulight/include)
target_link_libraries(run_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m)

enable_testing()
add_test(NAME unit_tests COMMAND run_tests)
endif()

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
    src/tokenizer/tiktoken.c
    src/tokenizer/gpt2bpe.c
    src/tokenizer/sentencepiece.c
    src/tokenizer/simd.c
    src/tokenizer/unicode_tables.c
    src/tokenizer/selector.c
    src/ui/modal.c
    src/ui/ui.c
    src/ui/markdown.c
    src/ui/console.c
    src/ui/syntax/syntax_ncurses.c
    src/ui/syntax/c/highlight.c
    src/ui/syntax/c/hashtable.c
)

if(NOT WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        list(APPEND INTEGRATION_TEST_SOURCES src/tokenizer/simd_arm64.S)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        list(APPEND INTEGRATION_TEST_SOURCES src/tokenizer/simd_x86_64.S)
    endif()
endif()

if(NOT WIN32)
add_executable(run_integration_tests ${INTEGRATION_TEST_SOURCES} ${ULIGHT_SOURCES})
if(WIN32)
    target_compile_definitions(run_integration_tests PRIVATE PDC_FORCE_UTF8)
else()
    target_compile_definitions(run_integration_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
endif()
target_include_directories(run_integration_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests tests/integration src/ui/syntax/ulight/include)
target_link_libraries(run_integration_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m)

add_test(NAME integration_tests COMMAND run_integration_tests)
endif()

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
    src/tokenizer/tiktoken.c
    src/tokenizer/gpt2bpe.c
    src/tokenizer/sentencepiece.c
    src/tokenizer/selector.c
    src/tokenizer/simd.c
    src/tokenizer/unicode_tables.c
    src/ui/modal.c
    src/ui/ui.c
    src/ui/markdown.c
    src/ui/syntax/syntax_ncurses.c
    src/ui/syntax/c/highlight.c
    src/ui/syntax/c/hashtable.c
)

if(NOT WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        list(APPEND ALL_TEST_SOURCES src/tokenizer/simd_arm64.S)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        list(APPEND ALL_TEST_SOURCES src/tokenizer/simd_x86_64.S)
    endif()
endif()

if(NOT WIN32)
add_executable(run_all_tests ${ALL_TEST_SOURCES} ${ULIGHT_SOURCES})

set_source_files_properties(${ULIGHT_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-trigraphs")

if(WIN32)
    target_compile_definitions(run_all_tests PRIVATE PDC_FORCE_UTF8)
else()
    target_compile_definitions(run_all_tests PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
endif()
target_include_directories(run_all_tests PRIVATE ${CURSES_INCLUDE_DIR} src tests tests/boundary tests/stress tests/generated tests/property src/ui/syntax/ulight/include)
target_link_libraries(run_all_tests PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES} m)

add_test(NAME all_tests COMMAND run_all_tests)
endif()

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

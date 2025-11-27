cmake_minimum_required(VERSION 3.16)
project(sillytui C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Werror)

find_package(Curses REQUIRED)
find_package(CURL REQUIRED)

set(SOURCES
    src/main.c
    src/core/config.c
    src/core/macros.c
    src/ui/ui.c
    src/ui/modal.c
    src/ui/markdown.c
    src/chat/chat.c
    src/chat/history.c
    src/llm/llm.c
    src/llm/sampler.c
    src/character/character.c
    src/character/persona.c
    src/tokenizer/tiktoken.c
    src/tokenizer/sentencepiece.c
    src/tokenizer/gpt2bpe.c
    src/tokenizer/simd.c
    src/tokenizer/unicode_tables.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    enable_language(ASM)
    list(APPEND SOURCES src/tokenizer/simd_arm64.S)
endif()

add_executable(sillytui ${SOURCES})

target_compile_definitions(sillytui PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(sillytui PRIVATE ${CURSES_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} src)
target_link_libraries(sillytui PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES})

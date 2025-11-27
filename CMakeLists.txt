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
    src/history.c
    src/markdown.c
    src/ui.c
    src/config.c
    src/llm.c
    src/modal.c
    src/chat.c
    src/character.c
    src/persona.c
    src/macros.c
    src/sampler.c
    src/tokenizer.c
    src/simd.c
    src/unicode_tables.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    enable_language(ASM)
    list(APPEND SOURCES src/simd_arm64.S)
endif()

add_executable(sillytui ${SOURCES})

target_compile_definitions(sillytui PRIVATE _XOPEN_SOURCE_EXTENDED NCURSES_WIDECHAR=1)
target_include_directories(sillytui PRIVATE ${CURSES_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} src)
target_link_libraries(sillytui PRIVATE ${CURSES_LIBRARIES} ${CURL_LIBRARIES})

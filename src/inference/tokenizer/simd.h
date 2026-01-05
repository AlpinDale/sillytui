#ifndef SIMD_H
#define SIMD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__aarch64__) || defined(_M_ARM64)
#define SIMD_ARM64 1
#define SIMD_X86_64 0
#elif defined(__x86_64__) || defined(_M_X64)
#define SIMD_ARM64 0
#define SIMD_X86_64 1
#else
#define SIMD_ARM64 0
#define SIMD_X86_64 0
#endif

uint64_t simd_hash_bytes(const uint8_t *bytes, size_t len);

size_t simd_find_non_ascii(const uint8_t *data, size_t len);

bool simd_is_all_ascii(const uint8_t *data, size_t len);

size_t simd_count_utf8_chars(const uint8_t *data, size_t len);

size_t simd_argmin_u32(const uint32_t *values, size_t count, uint32_t *out_min);

size_t simd_match_ascii_letters(const uint8_t *data, size_t len);

size_t simd_base64_decode(const char *input, size_t input_len, uint8_t *output,
                          size_t output_cap);

void simd_init(void);
bool simd_available(void);

// Fallback implementations (non-SIMD) - exposed for testing
uint64_t hash_bytes_fallback(const uint8_t *bytes, size_t len);
size_t find_non_ascii_fallback(const uint8_t *data, size_t len);
bool is_all_ascii_fallback(const uint8_t *data, size_t len);
size_t count_utf8_chars_fallback(const uint8_t *data, size_t len);
size_t argmin_u32_fallback(const uint32_t *values, size_t count,
                           uint32_t *out_min);
size_t match_ascii_letters_fallback(const uint8_t *data, size_t len);
size_t base64_decode_fallback(const char *input, size_t input_len,
                              uint8_t *output, size_t output_cap);

#endif

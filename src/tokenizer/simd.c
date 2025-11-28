#include "tokenizer/simd.h"
#include <string.h>

#if SIMD_ARM64
extern uint64_t simd_hash_bytes_arm64(const uint8_t *bytes, size_t len);
extern size_t simd_find_non_ascii_arm64(const uint8_t *data, size_t len);
extern bool simd_is_all_ascii_arm64(const uint8_t *data, size_t len);
extern size_t simd_count_utf8_chars_arm64(const uint8_t *data, size_t len);
extern size_t simd_argmin_u32_arm64(const uint32_t *values, size_t count,
                                    uint32_t *out_min);
extern size_t simd_match_ascii_letters_arm64(const uint8_t *data, size_t len);
#endif

#if SIMD_X86_64
extern uint64_t simd_hash_bytes_x86_64(const uint8_t *bytes, size_t len);
extern size_t simd_find_non_ascii_x86_64(const uint8_t *data, size_t len);
extern bool simd_is_all_ascii_x86_64(const uint8_t *data, size_t len);
extern size_t simd_count_utf8_chars_x86_64(const uint8_t *data, size_t len);
extern size_t simd_argmin_u32_x86_64(const uint32_t *values, size_t count,
                                     uint32_t *out_min);
extern size_t simd_match_ascii_letters_x86_64(const uint8_t *data, size_t len);
#endif

static uint64_t hash_bytes_fallback(const uint8_t *bytes, size_t len) {
  uint64_t hash = 14695981039346656037ULL;
  for (size_t i = 0; i < len; i++) {
    hash ^= bytes[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

static size_t find_non_ascii_fallback(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] >= 0x80)
      return i;
  }
  return len;
}

static bool is_all_ascii_fallback(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] >= 0x80)
      return false;
  }
  return true;
}

static size_t count_utf8_chars_fallback(const uint8_t *data, size_t len) {
  size_t count = 0;
  for (size_t i = 0; i < len; i++) {
    if ((data[i] & 0xC0) != 0x80)
      count++;
  }
  return count;
}

static size_t argmin_u32_fallback(const uint32_t *values, size_t count,
                                  uint32_t *out_min) {
  if (count == 0) {
    *out_min = UINT32_MAX;
    return 0;
  }
  uint32_t min_val = values[0];
  size_t min_idx = 0;
  for (size_t i = 1; i < count; i++) {
    if (values[i] < min_val) {
      min_val = values[i];
      min_idx = i;
    }
  }
  *out_min = min_val;
  return min_idx;
}

static inline bool is_ascii_letter(uint8_t b) {
  return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z');
}

static size_t match_ascii_letters_fallback(const uint8_t *data, size_t len) {
  size_t i = 0;
  while (i < len && is_ascii_letter(data[i]))
    i++;
  return i;
}

static inline int base64_char_to_val(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}

static size_t base64_decode_fallback(const char *input, size_t input_len,
                                     uint8_t *output, size_t output_cap) {
  size_t out_len = 0;
  uint32_t accum = 0;
  int bits = 0;

  for (size_t i = 0; i < input_len && out_len < output_cap; i++) {
    if (input[i] == '=')
      break;
    int val = base64_char_to_val(input[i]);
    if (val < 0)
      continue;
    accum = (accum << 6) | val;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      output[out_len++] = (accum >> bits) & 0xFF;
    }
  }
  return out_len;
}

#if SIMD_ARM64
#include <arm_neon.h>

static size_t base64_decode_neon(const char *input, size_t input_len,
                                 uint8_t *output, size_t output_cap) {
  size_t in_pos = 0;
  size_t out_pos = 0;

  static const int8_t lut_lo[] = {0x15, 0x11, 0x11, 0x11, 0x11, 0x11,
                                  0x11, 0x11, 0x11, 0x11, 0x13, 0x1A,
                                  0x1B, 0x1B, 0x1B, 0x1A};
  static const int8_t lut_hi[] = {0x10, 0x10, 0x01, 0x02, 0x04, 0x08,
                                  0x04, 0x08, 0x10, 0x10, 0x10, 0x10,
                                  0x10, 0x10, 0x10, 0x10};
  static const int8_t lut_roll[] = {0, 16, 19, 4, -65, -65, -71, -71,
                                    0, 0,  0,  0, 0,   0,   0,   0};

  int8x16_t v_lut_lo = vld1q_s8(lut_lo);
  int8x16_t v_lut_hi = vld1q_s8(lut_hi);
  int8x16_t v_lut_roll = vld1q_s8(lut_roll);
  int8x16_t v_2f = vdupq_n_s8(0x2F);

  while (in_pos + 16 <= input_len && out_pos + 12 <= output_cap) {
    uint8x16_t v_in = vld1q_u8((const uint8_t *)(input + in_pos));

    uint8x16_t has_eq = vceqq_u8(v_in, vdupq_n_u8('='));
    if (vmaxvq_u8(has_eq))
      break;

    int8x16_t s_in = vreinterpretq_s8_u8(v_in);

    uint8x16_t hi_nibbles = vshrq_n_u8(v_in, 4);
    uint8x16_t lo_nibbles = vandq_u8(v_in, vdupq_n_u8(0x0F));

    int8x16_t lo = vqtbl1q_s8(v_lut_lo, lo_nibbles);
    int8x16_t hi = vqtbl1q_s8(v_lut_hi, hi_nibbles);

    int8x16_t check = vandq_s8(lo, hi);
    if (vmaxvq_s8(check))
      break;

    int8x16_t eq_2f = vceqq_s8(s_in, v_2f);
    uint8x16_t roll_idx = vsubq_u8(hi_nibbles, vreinterpretq_u8_s8(eq_2f));
    int8x16_t roll = vqtbl1q_s8(v_lut_roll, roll_idx);

    int8x16_t decoded = vaddq_s8(s_in, roll);

    uint8x16_t d = vreinterpretq_u8_s8(decoded);

    uint8_t tmp[16];
    vst1q_u8(tmp, d);
    output[out_pos + 0] = (tmp[0] << 2) | (tmp[1] >> 4);
    output[out_pos + 1] = (tmp[1] << 4) | (tmp[2] >> 2);
    output[out_pos + 2] = (tmp[2] << 6) | tmp[3];
    output[out_pos + 3] = (tmp[4] << 2) | (tmp[5] >> 4);
    output[out_pos + 4] = (tmp[5] << 4) | (tmp[6] >> 2);
    output[out_pos + 5] = (tmp[6] << 6) | tmp[7];
    output[out_pos + 6] = (tmp[8] << 2) | (tmp[9] >> 4);
    output[out_pos + 7] = (tmp[9] << 4) | (tmp[10] >> 2);
    output[out_pos + 8] = (tmp[10] << 6) | tmp[11];
    output[out_pos + 9] = (tmp[12] << 2) | (tmp[13] >> 4);
    output[out_pos + 10] = (tmp[13] << 4) | (tmp[14] >> 2);
    output[out_pos + 11] = (tmp[14] << 6) | tmp[15];

    in_pos += 16;
    out_pos += 12;
  }

  while (in_pos < input_len && out_pos < output_cap) {
    if (input[in_pos] == '=')
      break;
    int v0 = base64_char_to_val(input[in_pos]);
    if (v0 < 0) {
      in_pos++;
      continue;
    }
    if (in_pos + 1 >= input_len)
      break;
    int v1 = base64_char_to_val(input[in_pos + 1]);
    if (v1 < 0) {
      in_pos++;
      continue;
    }

    output[out_pos++] = (v0 << 2) | (v1 >> 4);
    in_pos += 2;

    if (in_pos >= input_len || input[in_pos] == '=' || out_pos >= output_cap)
      break;
    int v2 = base64_char_to_val(input[in_pos]);
    if (v2 < 0) {
      in_pos++;
      continue;
    }
    output[out_pos++] = (v1 << 4) | (v2 >> 2);
    in_pos++;

    if (in_pos >= input_len || input[in_pos] == '=' || out_pos >= output_cap)
      break;
    int v3 = base64_char_to_val(input[in_pos]);
    if (v3 < 0) {
      in_pos++;
      continue;
    }
    output[out_pos++] = (v2 << 6) | v3;
    in_pos++;
  }

  return out_pos;
}
#endif

static bool g_simd_available = false;
static bool g_simd_initialized = false;

void simd_init(void) {
  if (g_simd_initialized)
    return;
  g_simd_initialized = true;

#if SIMD_ARM64
  g_simd_available = true;
#elif SIMD_X86_64
  g_simd_available = true;
#else
  g_simd_available = false;
#endif
}

bool simd_available(void) {
  if (!g_simd_initialized)
    simd_init();
  return g_simd_available;
}

uint64_t simd_hash_bytes(const uint8_t *bytes, size_t len) {
  if (g_simd_available || !g_simd_initialized) {
    if (!g_simd_initialized)
      simd_init();
    if (g_simd_available) {
#if SIMD_ARM64
      return simd_hash_bytes_arm64(bytes, len);
#elif SIMD_X86_64
      return simd_hash_bytes_x86_64(bytes, len);
#endif
    }
  }
  return hash_bytes_fallback(bytes, len);
}

size_t simd_find_non_ascii(const uint8_t *data, size_t len) {
  if (g_simd_available || !g_simd_initialized) {
    if (!g_simd_initialized)
      simd_init();
    if (g_simd_available) {
#if SIMD_ARM64
      return simd_find_non_ascii_arm64(data, len);
#elif SIMD_X86_64
      return simd_find_non_ascii_x86_64(data, len);
#endif
    }
  }
  return find_non_ascii_fallback(data, len);
}

bool simd_is_all_ascii(const uint8_t *data, size_t len) {
  if (g_simd_available || !g_simd_initialized) {
    if (!g_simd_initialized)
      simd_init();
    if (g_simd_available) {
#if SIMD_ARM64
      return simd_is_all_ascii_arm64(data, len);
#elif SIMD_X86_64
      return simd_is_all_ascii_x86_64(data, len);
#endif
    }
  }
  return is_all_ascii_fallback(data, len);
}

size_t simd_count_utf8_chars(const uint8_t *data, size_t len) {
  if (g_simd_available || !g_simd_initialized) {
    if (!g_simd_initialized)
      simd_init();
    if (g_simd_available) {
#if SIMD_ARM64
      return simd_count_utf8_chars_arm64(data, len);
#elif SIMD_X86_64
      return simd_count_utf8_chars_x86_64(data, len);
#endif
    }
  }
  return count_utf8_chars_fallback(data, len);
}

size_t simd_argmin_u32(const uint32_t *values, size_t count,
                       uint32_t *out_min) {
  if (g_simd_available || !g_simd_initialized) {
    if (!g_simd_initialized)
      simd_init();
    if (g_simd_available) {
#if SIMD_ARM64
      return simd_argmin_u32_arm64(values, count, out_min);
#elif SIMD_X86_64
      return simd_argmin_u32_x86_64(values, count, out_min);
#endif
    }
  }
  return argmin_u32_fallback(values, count, out_min);
}

size_t simd_match_ascii_letters(const uint8_t *data, size_t len) {
  if (g_simd_available || !g_simd_initialized) {
    if (!g_simd_initialized)
      simd_init();
    if (g_simd_available) {
#if SIMD_ARM64
      return simd_match_ascii_letters_arm64(data, len);
#elif SIMD_X86_64
      return simd_match_ascii_letters_x86_64(data, len);
#endif
    }
  }
  return match_ascii_letters_fallback(data, len);
}

size_t simd_base64_decode(const char *input, size_t input_len, uint8_t *output,
                          size_t output_cap) {
#if SIMD_ARM64
  if (g_simd_available || !g_simd_initialized) {
    if (!g_simd_initialized)
      simd_init();
    if (g_simd_available)
      return base64_decode_neon(input, input_len, output, output_cap);
  }
#endif
  return base64_decode_fallback(input, input_len, output, output_cap);
}

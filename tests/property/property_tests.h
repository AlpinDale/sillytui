#ifndef PROPERTY_TESTS_H
#define PROPERTY_TESTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  uint64_t state;
  uint64_t inc;
} RngState;

static inline void rng_init(RngState *rng, uint64_t seed) {
  rng->state = 0U;
  rng->inc = (seed << 1u) | 1u;
  rng->state = rng->state * 6364136223846793005ULL + rng->inc;
  rng->state = rng->state * 6364136223846793005ULL + rng->inc;
}

static inline uint32_t rng_next(RngState *rng) {
  uint64_t oldstate = rng->state;
  rng->state = oldstate * 6364136223846793005ULL + rng->inc;
  uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
  uint32_t rot = (uint32_t)(oldstate >> 59u);
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline uint32_t rng_range(RngState *rng, uint32_t max) {
  return rng_next(rng) % max;
}

static inline size_t gen_string(RngState *rng, char *buf, size_t max_len) {
  size_t len = rng_range(rng, (uint32_t)max_len);
  for (size_t i = 0; i < len; i++) {
    uint32_t r = rng_range(rng, 100);
    if (r < 50) {
      buf[i] = 'a' + (char)rng_range(rng, 26);
    } else if (r < 70) {
      buf[i] = 'A' + (char)rng_range(rng, 26);
    } else if (r < 80) {
      buf[i] = '0' + (char)rng_range(rng, 10);
    } else if (r < 90) {
      buf[i] = ' ';
    } else {
      const char punct[] = ".,!?;:'\"-()[]{}@#$%^&*";
      buf[i] = punct[rng_range(rng, sizeof(punct) - 1)];
    }
  }
  buf[len] = '\0';
  return len;
}

static inline size_t gen_utf8_string(RngState *rng, char *buf, size_t max_len) {
  size_t pos = 0;
  while (pos < max_len - 4) {
    uint32_t r = rng_range(rng, 100);
    if (r < 60) {
      buf[pos++] = 'a' + (char)rng_range(rng, 26);
    } else if (r < 70) {
      buf[pos++] = ' ';
    } else if (r < 80) {
      uint32_t cp = 0x80 + rng_range(rng, 0x700);
      buf[pos++] = (char)(0xC0 | (cp >> 6));
      buf[pos++] = (char)(0x80 | (cp & 0x3F));
    } else if (r < 90) {
      uint32_t cp = 0x4E00 + rng_range(rng, 0x5200);
      buf[pos++] = (char)(0xE0 | (cp >> 12));
      buf[pos++] = (char)(0x80 | ((cp >> 6) & 0x3F));
      buf[pos++] = (char)(0x80 | (cp & 0x3F));
    } else {
      uint32_t cp = 0x1F600 + rng_range(rng, 0x50);
      buf[pos++] = (char)(0xF0 | (cp >> 18));
      buf[pos++] = (char)(0x80 | ((cp >> 12) & 0x3F));
      buf[pos++] = (char)(0x80 | ((cp >> 6) & 0x3F));
      buf[pos++] = (char)(0x80 | (cp & 0x3F));
    }
    if (rng_range(rng, 10) == 0)
      break;
  }
  buf[pos] = '\0';
  return pos;
}

static inline size_t gen_bytes(RngState *rng, uint8_t *buf, size_t max_len) {
  size_t len = rng_range(rng, (uint32_t)max_len);
  for (size_t i = 0; i < len; i++) {
    buf[i] = (uint8_t)rng_range(rng, 256);
  }
  return len;
}

#define PROPERTY_CHECK(name, iterations, body)                                 \
  do {                                                                         \
    RngState _rng;                                                             \
    rng_init(&_rng, (uint64_t)time(NULL) ^ __LINE__);                          \
    for (int _i = 0; _i < (iterations); _i++) {                                \
      body                                                                     \
    }                                                                          \
  } while (0)

#endif

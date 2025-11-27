/*
 * Example usage of the tokenizer library with detailed timing.
 *
 * Supports multiple tokenizer formats:
 *   - tiktoken (OpenAI cl100k, o200k)
 *   - gpt2bpe (Qwen, Llama-3, GLM-4, DeepSeek)
 *   - sentencepiece (T5, Gemma, older Llama)
 *
 * To compile (from the project root directory):
 *   cc -O3 -o tokenize examples/tokenize.c \
 *       src/tokenizer/tiktoken.c src/tokenizer/gpt2bpe.c \
 *       src/tokenizer/sentencepiece.c src/tokenizer/simd.c \
 *       src/tokenizer/simd_arm64.S src/tokenizer/unicode_tables.c -Isrc
 *
 * Usage:
 *   ./tokenize --tokenizer <name> [--bench] <text>
 *   ./tokenize --list
 *
 * Examples:
 *   ./tokenize --tokenizer openai "Hello, world!"
 *   ./tokenize --tokenizer llama3 --bench "The quick brown fox"
 */

#include "tokenizer/gpt2bpe.h"
#include "tokenizer/sentencepiece.h"
#include "tokenizer/simd.h"
#include "tokenizer/tiktoken.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __MACH__
#include <mach/mach_time.h>
#endif

typedef enum {
  TOK_TYPE_TIKTOKEN,
  TOK_TYPE_GPT2BPE,
  TOK_TYPE_SENTENCEPIECE
} TokenizerType;

typedef struct {
  const char *name;
  TokenizerType type;
  const char *path1;
  const char *path2;
  const char *description;
} TokenizerInfo;

static const TokenizerInfo TOKENIZERS[] = {
    {"openai", TOK_TYPE_TIKTOKEN, "tokenizers/openai/cl100k_base.tiktoken",
     NULL, "OpenAI cl100k (GPT-4, GPT-3.5)"},
    {"openai-o200k", TOK_TYPE_TIKTOKEN, "tokenizers/openai/o200k_base.tiktoken",
     NULL, "OpenAI o200k (GPT-4o)"},
    {"qwen3", TOK_TYPE_GPT2BPE, "tokenizers/qwen3/vocab.json",
     "tokenizers/qwen3/merges.txt", "Qwen 3 (151k vocab)"},
    {"llama3", TOK_TYPE_GPT2BPE, "tokenizers/llama3/vocab.json",
     "tokenizers/llama3/merges.txt", "Llama 3 / 3.1 (128k vocab)"},
    {"glm4", TOK_TYPE_GPT2BPE, "tokenizers/glm4/vocab.json",
     "tokenizers/glm4/merges.txt", "GLM-4.5 (151k vocab)"},
    {"deepseek", TOK_TYPE_GPT2BPE, "tokenizers/deepseek-r1/vocab.json",
     "tokenizers/deepseek-r1/merges.txt", "DeepSeek R1 (128k vocab)"},
    {NULL, 0, NULL, NULL, NULL}};

typedef struct {
  double simd_init;
  double load;
  double encode;
  double decode_total;
  double cleanup;
  int token_count;
  size_t text_bytes;
  size_t vocab_size;
} TimingStats;

static inline double get_time_ns(void) {
#ifdef __MACH__
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return (double)mach_absolute_time() * tb.numer / tb.denom;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1e9 + ts.tv_nsec;
#endif
}

static const char *format_time(double ns) {
  static char buf[64];
  if (ns < 1000) {
    snprintf(buf, sizeof(buf), "%.1f ns", ns);
  } else if (ns < 1000000) {
    snprintf(buf, sizeof(buf), "%.2f µs", ns / 1000);
  } else if (ns < 1000000000) {
    snprintf(buf, sizeof(buf), "%.2f ms", ns / 1000000);
  } else {
    snprintf(buf, sizeof(buf), "%.2f s", ns / 1000000000);
  }
  return buf;
}

static void print_timing_stats(const TimingStats *stats,
                               const TokenizerInfo *info) {
  double total = stats->simd_init + stats->load + stats->encode +
                 stats->decode_total + stats->cleanup;

  printf("\n");
  printf("⏱  Timing Breakdown\n");
  printf("───────────────────────────────\n");
  printf("  SIMD init:       %s\n", format_time(stats->simd_init));
  printf("  Load tokenizer:  %s\n", format_time(stats->load));
  printf("  Encode text:     %s\n", format_time(stats->encode));
  printf("  Decode tokens:   %s\n", format_time(stats->decode_total));
  printf("  Cleanup:         %s\n", format_time(stats->cleanup));
  printf("───────────────────────────────\n");
  printf("  Total:           %s\n", format_time(total));

  if (stats->token_count > 0) {
    double tokens_per_sec = stats->token_count / (stats->encode / 1e9);
    double bytes_per_sec = stats->text_bytes / (stats->encode / 1e9);
    double ns_per_token = stats->encode / stats->token_count;
    double ns_per_byte = stats->encode / stats->text_bytes;

    printf("\n");
    printf("📊 Performance Stats\n");
    printf("───────────────────────────────\n");
    printf("  Tokenizer:       %s\n", info->name);
    printf("  Vocab size:      %zu\n", stats->vocab_size);
    printf("  Input:           %zu bytes\n", stats->text_bytes);
    printf("  Output:          %d tokens\n", stats->token_count);
    printf("───────────────────────────────\n");
    printf("  Throughput:      %.0f tokens/s\n", tokens_per_sec);
    printf("                   %.2f MB/s\n", bytes_per_sec / 1e6);
    printf("  Latency/token:   %s\n", format_time(ns_per_token));
    printf("  Latency/byte:    %s\n", format_time(ns_per_byte));
  }
}

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s --tokenizer <name> [--bench] <text>\n", prog);
  fprintf(stderr, "  %s --list\n\n", prog);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  --tokenizer, -t  Tokenizer to use (see --list)\n");
  fprintf(stderr, "  --bench, -b      Run benchmark (encode 1000x)\n");
  fprintf(stderr, "  --list, -l       List available tokenizers\n");
}

static void list_tokenizers(void) {
  printf("Available tokenizers:\n\n");
  for (int i = 0; TOKENIZERS[i].name; i++) {
    printf("  %-14s  %s\n", TOKENIZERS[i].name, TOKENIZERS[i].description);
  }
  printf("\n");
}

static const TokenizerInfo *find_tokenizer(const char *name) {
  for (int i = 0; TOKENIZERS[i].name; i++) {
    if (strcmp(TOKENIZERS[i].name, name) == 0) {
      return &TOKENIZERS[i];
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
    list_tokenizers();
    return 0;
  }

  const char *tokenizer_name = NULL;
  int text_start = 1;
  int bench_mode = 0;

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "--tokenizer") == 0 || strcmp(argv[i], "-t") == 0) &&
        i + 1 < argc) {
      tokenizer_name = argv[++i];
      text_start = i + 1;
    } else if (strcmp(argv[i], "--bench") == 0 ||
               strcmp(argv[i], "-b") == 0) {
      bench_mode = 1;
      if (text_start <= i)
        text_start = i + 1;
    }
  }

  if (!tokenizer_name) {
    tokenizer_name = "openai";
  }

  if (text_start >= argc) {
    fprintf(stderr, "Error: No text provided\n\n");
    print_usage(argv[0]);
    return 1;
  }

  const TokenizerInfo *info = find_tokenizer(tokenizer_name);
  if (!info) {
    fprintf(stderr, "Error: Unknown tokenizer '%s'\n", tokenizer_name);
    fprintf(stderr, "Use --list to see available tokenizers\n");
    return 1;
  }

  char *text;
  int needs_text_free = 0;
  if (text_start == argc - 1) {
    text = argv[text_start];
  } else {
    size_t total_len = 0;
    for (int i = text_start; i < argc; i++) {
      total_len += strlen(argv[i]) + 1;
    }
    text = malloc(total_len);
    if (!text) {
      fprintf(stderr, "Memory allocation failed\n");
      return 1;
    }
    text[0] = '\0';
    for (int i = text_start; i < argc; i++) {
      if (i > text_start)
        strcat(text, " ");
      strcat(text, argv[i]);
    }
    needs_text_free = 1;
  }

  TimingStats stats = {0};
  stats.text_bytes = strlen(text);
  double t0;

  t0 = get_time_ns();
  simd_init();
  stats.simd_init = get_time_ns() - t0;

  uint32_t tokens[4096];
  int token_count = 0;

  Tokenizer tiktoken;
  GPT2BPETokenizer gpt2bpe;
  SentencePieceProcessor sp;

  printf("Tokenizer: %s (%s)\n", info->name, info->description);
  printf("Text: \"%s\"\n", text);
  printf("Text length: %zu bytes\n", stats.text_bytes);

  t0 = get_time_ns();
  switch (info->type) {
  case TOK_TYPE_TIKTOKEN:
    tokenizer_init(&tiktoken);
    if (!tokenizer_load_tiktoken(&tiktoken, info->path1)) {
      fprintf(stderr, "Failed to load tokenizer from %s\n", info->path1);
      if (needs_text_free)
        free(text);
      return 1;
    }
    stats.vocab_size = tiktoken.count;
    break;

  case TOK_TYPE_GPT2BPE:
    gpt2_init(&gpt2bpe);
    if (!gpt2_load(&gpt2bpe, info->path1, info->path2)) {
      fprintf(stderr, "Failed to load tokenizer from %s, %s\n", info->path1,
              info->path2);
      if (needs_text_free)
        free(text);
      return 1;
    }
    stats.vocab_size = gpt2bpe.vocab_size;
    break;

  case TOK_TYPE_SENTENCEPIECE:
    sp_init(&sp);
    if (!sp_load(&sp, info->path1)) {
      fprintf(stderr, "Failed to load tokenizer from %s\n", info->path1);
      if (needs_text_free)
        free(text);
      return 1;
    }
    stats.vocab_size = sp.vocab_size;
    break;
  }
  stats.load = get_time_ns() - t0;

  if (bench_mode) {
    printf("\nRunning benchmark (1M iterations)...\n");
    for (int i = 0; i < 1000; i++) {
      switch (info->type) {
      case TOK_TYPE_TIKTOKEN:
        tokenizer_encode(&tiktoken, text, tokens, 4096);
        break;
      case TOK_TYPE_GPT2BPE:
        gpt2_encode(&gpt2bpe, text, tokens, 4096);
        break;
      case TOK_TYPE_SENTENCEPIECE:
        sp_encode(&sp, text, tokens, 4096);
        break;
      }
    }

    t0 = get_time_ns();
    for (int i = 0; i < 1000000; i++) {
      switch (info->type) {
      case TOK_TYPE_TIKTOKEN:
        token_count = tokenizer_encode(&tiktoken, text, tokens, 4096);
        break;
      case TOK_TYPE_GPT2BPE:
        token_count = gpt2_encode(&gpt2bpe, text, tokens, 4096);
        break;
      case TOK_TYPE_SENTENCEPIECE:
        token_count = sp_encode(&sp, text, tokens, 4096);
        break;
      }
    }
    stats.encode = (get_time_ns() - t0) / 1000000.0;
  } else {
    t0 = get_time_ns();
    switch (info->type) {
    case TOK_TYPE_TIKTOKEN:
      token_count = tokenizer_encode(&tiktoken, text, tokens, 4096);
      break;
    case TOK_TYPE_GPT2BPE:
      token_count = gpt2_encode(&gpt2bpe, text, tokens, 4096);
      break;
    case TOK_TYPE_SENTENCEPIECE:
      token_count = sp_encode(&sp, text, tokens, 4096);
      break;
    }
    stats.encode = get_time_ns() - t0;
  }

  stats.token_count = token_count;

  if (token_count < 0) {
    fprintf(stderr, "Tokenization failed\n");
    if (needs_text_free)
      free(text);
    return 1;
  }

  printf("\nToken count: %d\n\n", token_count);

  printf("Tokens: [");
  for (int i = 0; i < token_count; i++) {
    if (i > 0)
      printf(", ");
    printf("%u", tokens[i]);
  }
  printf("]\n\n");

  printf("Decoded tokens:\n");
  t0 = get_time_ns();
  for (int i = 0; i < token_count && i < 20; i++) {
    const char *piece = NULL;
    char *decoded = NULL;

    switch (info->type) {
    case TOK_TYPE_TIKTOKEN: {
      uint32_t single[1] = {tokens[i]};
      decoded = tokenizer_decode(&tiktoken, single, 1);
      piece = decoded;
      break;
    }
    case TOK_TYPE_GPT2BPE:
      piece = gpt2_id_to_token(&gpt2bpe, tokens[i]);
      break;
    case TOK_TYPE_SENTENCEPIECE:
      piece = sp_id_to_piece(&sp, tokens[i]);
      break;
    }

    if (piece) {
      printf("  [%d] %u -> \"", i, tokens[i]);
      for (size_t j = 0; piece[j]; j++) {
        unsigned char c = (unsigned char)piece[j];
        if (c >= 0x20 && c < 0x7f && c != '"' && c != '\\')
          putchar(c);
        else
          printf("\\x%02x", c);
      }
      printf("\"\n");
    }
    if (decoded)
      free(decoded);
  }
  stats.decode_total = get_time_ns() - t0;

  if (token_count > 20) {
    printf("  ... (%d more tokens)\n", token_count - 20);
  }

  t0 = get_time_ns();
  switch (info->type) {
  case TOK_TYPE_TIKTOKEN:
    tokenizer_free(&tiktoken);
    break;
  case TOK_TYPE_GPT2BPE:
    gpt2_free(&gpt2bpe);
    break;
  case TOK_TYPE_SENTENCEPIECE:
    sp_free(&sp);
    break;
  }
  stats.cleanup = get_time_ns() - t0;

  print_timing_stats(&stats, info);

  if (needs_text_free)
    free(text);

  return 0;
}

#include "inference/core/dtype.h"
#include "inference/model/qwen3/qwen3.h"
#include "inference/tokenizer/gpt2bpe.h"
#include "inference/tokenizer/simd.h"
#include "inference/kernels/sampling/sampling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static double get_time_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static int estimate_model_size_mb(const qwen3_model_t *model) {
  long long vocab_embed = (long long)model->config.vocab_size * model->config.hidden_size * 4;
  long long layers = (long long)model->config.num_hidden_layers * (
    model->config.hidden_size * model->config.hidden_size * 4 * 4 +
    model->config.hidden_size * model->config.intermediate_size * 4 * 2 +
    model->config.intermediate_size * model->config.hidden_size * 4
  );
  long long lm_head = (long long)model->config.vocab_size * model->config.hidden_size * 4;
  return (vocab_embed + layers + lm_head) / (1024 * 1024);
}

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [options] <model_dir> [prompt]\n", prog);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  --dtype <f32|f16>  Set compute dtype (default: f16)\n");
  fprintf(stderr, "  --help             Show this help message\n");
  fprintf(stderr, "\nIf no prompt is provided, uses BOS token only\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  dtype_t dtype = DTYPE_F16;
  const char *model_dir = NULL;
  const char *prompt = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--dtype") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: --dtype requires an argument\n");
        return 1;
      }
      i++;
      if (strcmp(argv[i], "f32") == 0) {
        dtype = DTYPE_F32;
      } else if (strcmp(argv[i], "f16") == 0) {
        dtype = DTYPE_F16;
      } else {
        fprintf(stderr, "Error: Invalid dtype '%s'. Use 'f32' or 'f16'\n", argv[i]);
        return 1;
      }
    } else if (strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (model_dir == NULL) {
      model_dir = argv[i];
    } else if (prompt == NULL) {
      prompt = argv[i];
    }
  }

  if (model_dir == NULL) {
    fprintf(stderr, "Error: model_dir is required\n");
    print_usage(argv[0]);
    return 1;
  }

  qwen3_model_t model;

  double load_start = get_time_ms();
  if (!qwen3_model_load(&model, model_dir, dtype)) {
    fprintf(stderr, "Failed to load model\n");
    return 1;
  }
  double load_time = get_time_ms() - load_start;

  int model_size_mb = estimate_model_size_mb(&model);
  const char *dtype_str = (dtype == DTYPE_F16) ? "f16" : "f32";
  printf("Model: Qwen3-%.1fB (L%d, H%d, %dM params, ~%dMB, %s) | Load: %.1fms\n",
         model.config.hidden_size / 1024.0,
         model.config.num_hidden_layers,
         model.config.num_attention_heads,
         (model.config.hidden_size * model.config.num_hidden_layers * 4) / 1000000,
         model_size_mb,
         dtype_str,
         load_time);

  simd_init();
  GPT2BPETokenizer tokenizer;
  gpt2_init(&tokenizer);
  if (!gpt2_load(&tokenizer, "tokenizers/qwen3/vocab.json", "tokenizers/qwen3/merges.txt")) {
    fprintf(stderr, "Error: Failed to load tokenizer\n");
    qwen3_model_free(&model);
    return 1;
  }

  int input_tokens[1024];
  int num_input_tokens = 0;

  if (prompt && strlen(prompt) > 0) {
    uint32_t encoded[1024];
    int encoded_count = gpt2_encode(&tokenizer, prompt, encoded, 1024);
    if (encoded_count <= 0) {
      fprintf(stderr, "Error: Failed to encode prompt\n");
      gpt2_free(&tokenizer);
      qwen3_model_free(&model);
      return 1;
    }
    for (int i = 0; i < encoded_count; i++) {
      input_tokens[i] = (int)encoded[i];
    }
    num_input_tokens = encoded_count;
  } else {
    input_tokens[0] = model.config.bos_token_id;
    num_input_tokens = 1;
  }

  qwen3_model_reset_cache(&model);

  double prefill_start = get_time_ms();
  float *logits = (float *)malloc(model.config.vocab_size * sizeof(float));
  if (!logits) {
    gpt2_free(&tokenizer);
    qwen3_model_free(&model);
    return 1;
  }

  if (!qwen3_forward(&model, logits, input_tokens, num_input_tokens)) {
    free(logits);
    gpt2_free(&tokenizer);
    qwen3_model_free(&model);
    return 1;
  }
  double prefill_time = get_time_ms() - prefill_start;
  double input_tok_per_s = num_input_tokens > 0 ? (num_input_tokens * 1000.0 / prefill_time) : 0.0;

  if (prompt && strlen(prompt) > 0) {
    uint32_t *prompt_token_ids = (uint32_t *)malloc(num_input_tokens * sizeof(uint32_t));
    if (prompt_token_ids) {
      for (int i = 0; i < num_input_tokens; i++) {
        prompt_token_ids[i] = (uint32_t)input_tokens[i];
      }
      char *prompt_decoded = gpt2_decode(&tokenizer, prompt_token_ids, num_input_tokens);
      if (prompt_decoded) {
        printf("Prompt: %s", prompt_decoded);
        fflush(stdout);
        free(prompt_decoded);
      }
      free(prompt_token_ids);
    }
  }

  sampling_rng_t rng;
  sampling_rng_init(&rng, 42);

  int max_tokens = 100;
  int output_tokens[100];
  int num_generated = 0;
  int current_token;

  double decode_start = get_time_ms();
  double first_token_time = 0;
  int is_tty = isatty(fileno(stdout));
  (void)is_tty;

  for (int i = 0; i < max_tokens; i++) {
    current_token = sampling_sample_f32(logits, model.config.vocab_size,
                                        1.0f, 50, 0.9f, 0.0f, &rng);
    output_tokens[i] = current_token;
    num_generated++;

    if (first_token_time == 0) {
      first_token_time = get_time_ms();
    }

    char *decoded = gpt2_decode(&tokenizer, (uint32_t*)&current_token, 1);
    if (decoded) {
      printf("%s", decoded);
      fflush(stdout);
      free(decoded);
    }

    if (current_token == model.config.eos_token_id)
      break;

    int single_token[1] = {current_token};
    if (!qwen3_forward(&model, logits, single_token, 1))
      break;
  }

  double decode_time = get_time_ms() - decode_start;
  double output_tok_per_s = num_generated > 0 ? (num_generated * 1000.0 / decode_time) : 0.0;
  double time_to_first_token = first_token_time > 0 ? (first_token_time - prefill_start) : 0.0;

  printf("\n\n");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("Performance Metrics\n");
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  printf("  Input tokens:     %d\n", num_input_tokens);
  printf("  Output tokens:    %d\n", num_generated);
  printf("  Prefill time:     %.1f ms  (%.1f tok/s)\n", prefill_time, input_tok_per_s);
  printf("  Decode time:      %.1f ms  (%.1f tok/s)\n", decode_time, output_tok_per_s);
  printf("  Time to 1st:      %.1f ms\n", time_to_first_token);
  printf("  Total time:       %.1f ms\n", prefill_time + decode_time);

  long long bytes_processed = 0;
  long long hidden_size_bytes = (long long)model.config.hidden_size * 4;
  long long head_dim_bytes = (long long)model.config.head_dim * 4;

  long long per_layer_weight_bytes =
    (long long)model.config.hidden_size * model.config.hidden_size * 4 * 4 +
    (long long)model.config.hidden_size * model.config.intermediate_size * 4 * 2 +
    (long long)model.config.intermediate_size * model.config.hidden_size * 4;

  long long total_weight_bytes = per_layer_weight_bytes * model.config.num_hidden_layers +
                                 (long long)model.config.vocab_size * model.config.hidden_size * 4 * 2;

  long long activation_bytes = (long long)(num_input_tokens + num_generated) *
                                hidden_size_bytes * model.config.num_hidden_layers * 2;

  long long kv_cache_bytes = (long long)model.config.num_hidden_layers *
                             (num_input_tokens + num_generated) *
                             model.config.num_key_value_heads *
                             head_dim_bytes * 2;

  bytes_processed = total_weight_bytes * (num_input_tokens + num_generated) +
                    activation_bytes + kv_cache_bytes;

  double total_time_s = (prefill_time + decode_time) / 1000.0;
  double memory_bandwidth_gb_s = total_time_s > 0 ?
    (bytes_processed / total_time_s) / (1024.0 * 1024.0 * 1024.0) : 0.0;
  printf("  Est. mem BW:      %.2f GB/s\n", memory_bandwidth_gb_s);
  printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

  free(logits);
  if (tokenizer.loaded) {
    gpt2_free(&tokenizer);
  }
  qwen3_model_free(&model);
  return 0;
}

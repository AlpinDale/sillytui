#include "weights.h"
#include "inference/core/dtype.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAFETENSORS_CPP_IMPLEMENTATION
#include "inference/model_loader/safetensors.hh"

/**
 * Load a tensor from safetensors file and wrap it in tensor_t.
 * Handles dtype conversion and optional transpose.
 */
static tensor_t *load_tensor(const safetensors::safetensors_t *st,
                             const char *tensor_name, dtype_t target_dtype,
                             bool transpose, int rows, int cols) {
  const auto &keys = st->tensors.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == tensor_name) {
      safetensors::tensor_t st_tensor;
      st->tensors.at(i, &st_tensor);

      size_t tensor_size = safetensors::get_shape_size(st_tensor);
      size_t elem_size = dtype_size(target_dtype);

      void *data = malloc(tensor_size * elem_size);
      if (!data)
        return NULL;

      const uint8_t *src = st->databuffer_addr + st_tensor.data_offsets[0];

      /* Determine source dtype */
      dtype_t src_dtype;
      if (st_tensor.dtype == safetensors::dtype::kFLOAT32) {
        src_dtype = DTYPE_F32;
      } else if (st_tensor.dtype == safetensors::dtype::kFLOAT16) {
        src_dtype = DTYPE_F16;
      } else if (st_tensor.dtype == safetensors::dtype::kBFLOAT16) {
        src_dtype = DTYPE_BF16;
      } else {
        free(data);
        return NULL;
      }

      /* Load with optional transpose and dtype conversion */
      if (transpose && rows > 0 && cols > 0) {
        /* Transpose while converting */
        for (int r = 0; r < rows; r++) {
          for (int c = 0; c < cols; c++) {
            size_t src_idx = r * cols + c;
            size_t dst_idx = c * rows + r;

            /* Convert single element */
            float val;
            if (src_dtype == DTYPE_F32) {
              val = ((const float *)src)[src_idx];
            } else if (src_dtype == DTYPE_F16) {
              val = f16_to_f32(((const uint16_t *)src)[src_idx]);
            } else { /* BF16 */
              val = bf16_to_f32(((const uint16_t *)src)[src_idx]);
            }

            if (target_dtype == DTYPE_F32) {
              ((float *)data)[dst_idx] = val;
            } else { /* F16 */
              ((uint16_t *)data)[dst_idx] = f32_to_f16(val);
            }
          }
        }
      } else {
        /* Direct conversion without transpose */
        if (src_dtype == target_dtype) {
          memcpy(data, src, tensor_size * elem_size);
        } else {
          dtype_convert_array(src, src_dtype, data, target_dtype, tensor_size);
        }
      }

      /* Determine shape based on safetensors metadata */
      int ndim = (int)st_tensor.shape.size();
      int64_t shape[TENSOR_MAX_DIMS];
      for (int d = 0; d < ndim && d < TENSOR_MAX_DIMS; d++) {
        shape[d] = st_tensor.shape[d];
      }

      /* If transposed, swap the shape dimensions */
      if (transpose && ndim == 2) {
        int64_t tmp = shape[0];
        shape[0] = shape[1];
        shape[1] = tmp;
      }

      tensor_t *t = tensor_wrap(data, target_dtype, ndim, shape);
      if (!t) {
        free(data);
        return NULL;
      }
      t->owns_data = true; /* Take ownership */
      return t;
    }
  }
  return NULL;
}

static bool load_layer_weights(qwen3_layer_weights_t *layer_weights,
                               const safetensors::safetensors_t *st,
                               int layer_idx, const model_config_t *config,
                               dtype_t dtype) {
  char tensor_name[256];
  int hidden = config->hidden_size;
  int q_dim = config->num_attention_heads * config->head_dim;
  int kv_dim = config->num_key_value_heads * config->head_dim;
  int inter = config->intermediate_size;

  /* For F16, we transpose weight matrices for efficient GEMM */
  bool do_transpose = (dtype == DTYPE_F16);

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.q_proj.weight", layer_idx);
  layer_weights->q_proj =
      load_tensor(st, tensor_name, dtype, do_transpose, q_dim, hidden);
  if (!layer_weights->q_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.k_proj.weight", layer_idx);
  layer_weights->k_proj =
      load_tensor(st, tensor_name, dtype, do_transpose, kv_dim, hidden);
  if (!layer_weights->k_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.v_proj.weight", layer_idx);
  layer_weights->v_proj =
      load_tensor(st, tensor_name, dtype, do_transpose, kv_dim, hidden);
  if (!layer_weights->v_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.o_proj.weight", layer_idx);
  layer_weights->o_proj =
      load_tensor(st, tensor_name, dtype, do_transpose, hidden, q_dim);
  if (!layer_weights->o_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.q_norm.weight", layer_idx);
  layer_weights->q_norm = load_tensor(st, tensor_name, dtype, false, 0, 0);
  if (!layer_weights->q_norm)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.k_norm.weight", layer_idx);
  layer_weights->k_norm = load_tensor(st, tensor_name, dtype, false, 0, 0);
  if (!layer_weights->k_norm)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.mlp.gate_proj.weight", layer_idx);
  layer_weights->gate_proj =
      load_tensor(st, tensor_name, dtype, do_transpose, inter, hidden);
  if (!layer_weights->gate_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.mlp.up_proj.weight", layer_idx);
  layer_weights->up_proj =
      load_tensor(st, tensor_name, dtype, do_transpose, inter, hidden);
  if (!layer_weights->up_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.mlp.down_proj.weight", layer_idx);
  layer_weights->down_proj =
      load_tensor(st, tensor_name, dtype, do_transpose, hidden, inter);
  if (!layer_weights->down_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.input_layernorm.weight", layer_idx);
  layer_weights->input_norm = load_tensor(st, tensor_name, dtype, false, 0, 0);
  if (!layer_weights->input_norm)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.post_attention_layernorm.weight", layer_idx);
  layer_weights->post_attn_norm =
      load_tensor(st, tensor_name, dtype, false, 0, 0);
  if (!layer_weights->post_attn_norm)
    return false;

  return true;
}

bool qwen3_weights_load(qwen3_weights_t *weights, const model_config_t *config,
                        const char *model_path, dtype_t dtype) {
  if (!weights || !config || !model_path)
    return false;

  memset(weights, 0, sizeof(*weights));
  weights->dtype = dtype;

  safetensors::safetensors_t st;
  std::string warn, err;
  bool ret = safetensors::mmap_from_file(model_path, &st, &warn, &err);
  if (!ret) {
    fprintf(stderr, "Failed to load model: %s\n", err.c_str());
    return false;
  }

  if (!safetensors::validate_data_offsets(st, err)) {
    fprintf(stderr, "Invalid safetensors file: %s\n", err.c_str());
    return false;
  }

  int vocab = config->vocab_size;
  int hidden = config->hidden_size;
  bool do_transpose = (dtype == DTYPE_F16);

  weights->embed_tokens =
      load_tensor(&st, "model.embed_tokens.weight", dtype, false, 0, 0);
  if (!weights->embed_tokens) {
    fprintf(stderr, "Failed to load embed_tokens\n");
    return false;
  }

  weights->final_norm =
      load_tensor(&st, "model.norm.weight", dtype, false, 0, 0);
  if (!weights->final_norm) {
    fprintf(stderr, "Failed to load norm\n");
    return false;
  }

  const char *lm_head_name = config->tie_word_embeddings
                                 ? "model.embed_tokens.weight"
                                 : "lm_head.weight";
  weights->lm_head =
      load_tensor(&st, lm_head_name, dtype, do_transpose, vocab, hidden);
  if (!weights->lm_head) {
    if (config->tie_word_embeddings) {
      /* For tied embeddings without transpose, just share the pointer */
      /* But we need a separate tensor_t that doesn't own the data */
      weights->lm_head = tensor_wrap(weights->embed_tokens->data, dtype,
                                     weights->embed_tokens->ndim,
                                     weights->embed_tokens->shape);
      if (!weights->lm_head) {
        fprintf(stderr, "Failed to create lm_head alias\n");
        return false;
      }
    } else {
      fprintf(stderr, "Failed to load lm_head\n");
      return false;
    }
  }

  weights->num_layers = config->num_hidden_layers;
  weights->layers = (qwen3_layer_weights_t *)calloc(
      config->num_hidden_layers, sizeof(qwen3_layer_weights_t));
  if (!weights->layers) {
    fprintf(stderr, "Failed to allocate layer weights\n");
    return false;
  }

  for (int i = 0; i < config->num_hidden_layers; i++) {
    if (!load_layer_weights(&weights->layers[i], &st, i, config, dtype)) {
      fprintf(stderr, "Failed to load layer %d weights\n", i);
      qwen3_weights_free(weights);
      return false;
    }
  }

  return true;
}

void qwen3_weights_free(qwen3_weights_t *weights) {
  if (!weights)
    return;

  tensor_free(weights->embed_tokens);
  tensor_free(weights->final_norm);

  /* Check if lm_head shares data with embed_tokens before freeing */
  if (weights->lm_head &&
      (!weights->embed_tokens ||
       weights->lm_head->data != weights->embed_tokens->data)) {
    tensor_free(weights->lm_head);
  } else if (weights->lm_head) {
    /* Just free the tensor_t struct, not the data */
    free(weights->lm_head);
  }

  if (weights->layers) {
    for (int i = 0; i < weights->num_layers; i++) {
      qwen3_layer_weights_t *layer = &weights->layers[i];
      tensor_free(layer->q_proj);
      tensor_free(layer->k_proj);
      tensor_free(layer->v_proj);
      tensor_free(layer->o_proj);
      tensor_free(layer->q_norm);
      tensor_free(layer->k_norm);
      tensor_free(layer->gate_proj);
      tensor_free(layer->up_proj);
      tensor_free(layer->down_proj);
      tensor_free(layer->input_norm);
      tensor_free(layer->post_attn_norm);
    }
    free(weights->layers);
  }

  memset(weights, 0, sizeof(*weights));
}

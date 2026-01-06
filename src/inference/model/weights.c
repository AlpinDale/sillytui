#include "inference/model/weights.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAFETENSORS_CPP_IMPLEMENTATION
#include "inference/model_loader/safetensors.hh"

static float *load_tensor_f32(const safetensors::safetensors_t *st,
                              const char *tensor_name, size_t *out_size) {
  const auto &keys = st->tensors.keys();
  for (size_t i = 0; i < keys.size(); i++) {
    if (keys[i] == tensor_name) {
      safetensors::tensor_t tensor;
      st->tensors.at(i, &tensor);

      size_t tensor_size = safetensors::get_shape_size(tensor);
      float *data = (float *)malloc(tensor_size * sizeof(float));
      if (!data)
        return NULL;

      const uint8_t *src = st->databuffer_addr + tensor.data_offsets[0];

      if (tensor.dtype == safetensors::dtype::kFLOAT32) {
        memcpy(data, src, tensor_size * sizeof(float));
      } else if (tensor.dtype == safetensors::dtype::kBFLOAT16) {
        const uint16_t *src_bf16 = (const uint16_t *)src;
        for (size_t j = 0; j < tensor_size; j++) {
          data[j] = safetensors::bfloat16_to_float(src_bf16[j]);
        }
      } else if (tensor.dtype == safetensors::dtype::kFLOAT16) {
        const uint16_t *src_f16 = (const uint16_t *)src;
        for (size_t j = 0; j < tensor_size; j++) {
          data[j] = safetensors::fp16_to_float(src_f16[j]);
        }
      } else {
        free(data);
        return NULL;
      }

      if (out_size)
        *out_size = tensor_size;
      return data;
    }
  }
  return NULL;
}

static bool load_layer_weights(qwen3_layer_weights_t *layer_weights,
                               const safetensors::safetensors_t *st,
                               int layer_idx, const qwen3_config_t *config) {
  (void)config;
  char tensor_name[256];
  size_t dummy;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.q_proj.weight", layer_idx);
  layer_weights->q_proj = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->q_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.k_proj.weight", layer_idx);
  layer_weights->k_proj = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->k_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.v_proj.weight", layer_idx);
  layer_weights->v_proj = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->v_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.o_proj.weight", layer_idx);
  layer_weights->o_proj = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->o_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.q_norm.weight", layer_idx);
  layer_weights->q_norm = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->q_norm)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.self_attn.k_norm.weight", layer_idx);
  layer_weights->k_norm = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->k_norm)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.mlp.gate_proj.weight", layer_idx);
  layer_weights->gate_proj = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->gate_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.mlp.up_proj.weight", layer_idx);
  layer_weights->up_proj = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->up_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.mlp.down_proj.weight", layer_idx);
  layer_weights->down_proj = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->down_proj)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.input_layernorm.weight", layer_idx);
  layer_weights->attn_norm = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->attn_norm)
    return false;

  snprintf(tensor_name, sizeof(tensor_name),
           "model.layers.%d.post_attention_layernorm.weight", layer_idx);
  layer_weights->ffn_norm = load_tensor_f32(st, tensor_name, &dummy);
  if (!layer_weights->ffn_norm)
    return false;

  return true;
}

bool qwen3_weights_load(qwen3_weights_t *weights, const qwen3_config_t *config,
                        const char *model_path) {
  if (!weights || !config || !model_path)
    return false;

  memset(weights, 0, sizeof(*weights));

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

  weights->embed_tokens =
      load_tensor_f32(&st, "model.embed_tokens.weight", NULL);
  if (!weights->embed_tokens) {
    fprintf(stderr, "Failed to load embed_tokens\n");
    return false;
  }

  weights->norm = load_tensor_f32(&st, "model.norm.weight", NULL);
  if (!weights->norm) {
    fprintf(stderr, "Failed to load norm\n");
    return false;
  }

  const char *lm_head_name = config->tie_word_embeddings
                                 ? "model.embed_tokens.weight"
                                 : "lm_head.weight";
  weights->lm_head = load_tensor_f32(&st, lm_head_name, NULL);
  if (!weights->lm_head) {
    if (config->tie_word_embeddings) {
      weights->lm_head = weights->embed_tokens;
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
    if (!load_layer_weights(&weights->layers[i], &st, i, config)) {
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

  if (weights->embed_tokens)
    free(weights->embed_tokens);

  if (weights->norm)
    free(weights->norm);

  if (weights->lm_head && weights->lm_head != weights->embed_tokens)
    free(weights->lm_head);

  if (weights->layers) {
    for (int i = 0; i < weights->num_layers; i++) {
      qwen3_layer_weights_t *layer = &weights->layers[i];
      if (layer->q_proj)
        free(layer->q_proj);
      if (layer->k_proj)
        free(layer->k_proj);
      if (layer->v_proj)
        free(layer->v_proj);
      if (layer->o_proj)
        free(layer->o_proj);
      if (layer->q_norm)
        free(layer->q_norm);
      if (layer->k_norm)
        free(layer->k_norm);
      if (layer->gate_proj)
        free(layer->gate_proj);
      if (layer->up_proj)
        free(layer->up_proj);
      if (layer->down_proj)
        free(layer->down_proj);
      if (layer->attn_norm)
        free(layer->attn_norm);
      if (layer->ffn_norm)
        free(layer->ffn_norm);
    }
    free(weights->layers);
  }

  memset(weights, 0, sizeof(*weights));
}

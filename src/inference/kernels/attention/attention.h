#ifndef ATTENTION_H
#define ATTENTION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flash Attention - Memory-efficient attention implementation
 *
 * Implements the Flash Attention algorithm (https://arxiv.org/abs/2205.14135)
 * which computes attention without materializing the full N×N attention matrix.
 *
 * Memory: O(N) instead of O(N²)
 * Compute: Same as standard attention
 */

/*
 * Single-head Flash Attention (FP32)
 *
 * Computes: output = softmax(Q @ K^T / sqrt(head_dim)) @ V
 *
 * Parameters:
 *   output:    [seq_len_q, head_dim] - output tensor
 *   query:     [seq_len_q, head_dim] - query tensor
 *   key:       [seq_len_kv, head_dim] - key tensor
 *   value:     [seq_len_kv, head_dim] - value tensor
 *   seq_len_q: query sequence length
 *   seq_len_kv: key/value sequence length
 *   head_dim:  dimension per head (typically 64, 128, or 256)
 *   scale:     scaling factor (typically 1/sqrt(head_dim))
 *   mask:      [seq_len_q, seq_len_kv] causal mask (optional, NULL for no mask)
 *              Use -INFINITY for masked positions
 */
void flash_attention_f32(float *output, const float *query, const float *key,
                         const float *value, int seq_len_q, int seq_len_kv,
                         int head_dim, float scale, const float *mask);

void flash_attention_bf16(uint16_t *output, const uint16_t *query,
                          const uint16_t *key, const uint16_t *value,
                          int seq_len_q, int seq_len_kv, int head_dim,
                          float scale, const float *mask);

void flash_attention_f16(uint16_t *output, const uint16_t *query,
                         const uint16_t *key, const uint16_t *value,
                         int seq_len_q, int seq_len_kv, int head_dim,
                         float scale, const float *mask);

/*
 * Multi-head Flash Attention (FP32)
 *
 * Computes attention for multiple heads in parallel.
 *
 * Parameters:
 *   output:     [batch, num_heads, seq_len_q, head_dim]
 *   query:      [batch, num_heads, seq_len_q, head_dim]
 *   key:        [batch, num_kv_heads, seq_len_kv, head_dim]
 *   value:      [batch, num_kv_heads, seq_len_kv, head_dim]
 *   batch:      batch size
 *   num_heads:  number of query heads
 *   num_kv_heads: number of key/value heads (for GQA, MQA)
 *   seq_len_q:  query sequence length
 *   seq_len_kv: key/value sequence length
 *   head_dim:   dimension per head
 *   scale:      scaling factor
 *   mask:       causal mask (optional)
 */
void flash_attention_mha_f32(float *output, const float *query,
                             const float *key, const float *value, int batch,
                             int num_heads, int num_kv_heads, int seq_len_q,
                             int seq_len_kv, int head_dim, float scale,
                             const float *mask);

void attention_set_num_threads(int num_threads);
int attention_get_num_threads(void);

#ifdef __cplusplus
}
#endif

#endif

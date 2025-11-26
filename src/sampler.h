#ifndef SAMPLER_H
#define SAMPLER_H

#include "config.h"
#include <stdbool.h>

#define MAX_CUSTOM_SAMPLERS 512
#define CUSTOM_SAMPLER_NAME_LEN 64
#define CUSTOM_SAMPLER_STR_LEN 256
#define MAX_LIST_ITEMS 512
#define MAX_DICT_ITEMS 512
#define DICT_KEY_LEN 32
#define DICT_VAL_LEN 64

typedef enum {
  SAMPLER_TYPE_FLOAT = 0,
  SAMPLER_TYPE_INT,
  SAMPLER_TYPE_STRING,
  SAMPLER_TYPE_BOOL,
  SAMPLER_TYPE_LIST_FLOAT,
  SAMPLER_TYPE_LIST_INT,
  SAMPLER_TYPE_LIST_STRING,
  SAMPLER_TYPE_DICT
} SamplerValueType;

typedef struct {
  char key[DICT_KEY_LEN];
  char str_val[DICT_VAL_LEN];
  double num_val;
  bool is_string;
} DictEntry;

typedef struct {
  char name[CUSTOM_SAMPLER_NAME_LEN];
  SamplerValueType type;
  double value;
  char str_value[CUSTOM_SAMPLER_STR_LEN];
  double min_val;
  double max_val;
  double step;
  double list_values[MAX_LIST_ITEMS];
  char list_strings[MAX_LIST_ITEMS][64];
  int list_count;
  DictEntry dict_entries[MAX_DICT_ITEMS];
  int dict_count;
} CustomSampler;

typedef struct {
  double temperature;
  double top_p;
  int top_k;
  double min_p;
  double frequency_penalty;
  double presence_penalty;
  double repetition_penalty;
  double typical_p;
  double tfs;
  double top_a;
  double smoothing_factor;
  int min_tokens;
  int max_tokens;

  double dynatemp_min;
  double dynatemp_max;
  double dynatemp_exponent;

  int mirostat_mode;
  double mirostat_tau;
  double mirostat_eta;

  double dry_multiplier;
  double dry_base;
  int dry_allowed_length;
  int dry_range;

  double xtc_threshold;
  double xtc_probability;

  double nsigma;
  double skew;

  CustomSampler custom[MAX_CUSTOM_SAMPLERS];
  int custom_count;
} SamplerSettings;

void sampler_init_defaults(SamplerSettings *s);
bool sampler_load(SamplerSettings *s, ApiType api_type);
bool sampler_save(const SamplerSettings *s, ApiType api_type);
bool sampler_add_custom(SamplerSettings *s, const char *name,
                        SamplerValueType type, double value,
                        const char *str_value, double min_val, double max_val,
                        double step);
bool sampler_remove_custom(SamplerSettings *s, int index);

#endif

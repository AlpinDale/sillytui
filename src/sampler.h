#ifndef SAMPLER_H
#define SAMPLER_H

#include "config.h"
#include <stdbool.h>

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
} SamplerSettings;

void sampler_init_defaults(SamplerSettings *s);
bool sampler_load(SamplerSettings *s, ApiType api_type);
bool sampler_save(const SamplerSettings *s, ApiType api_type);

#endif

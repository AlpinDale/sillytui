#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_MODELS 32
#define MAX_NAME_LEN 64
#define MAX_URL_LEN 256
#define MAX_KEY_LEN 256
#define MAX_MODEL_ID_LEN 128

typedef struct {
  char name[MAX_NAME_LEN];
  char base_url[MAX_URL_LEN];
  char api_key[MAX_KEY_LEN];
  char model_id[MAX_MODEL_ID_LEN];
} ModelConfig;

typedef struct {
  ModelConfig models[MAX_MODELS];
  size_t count;
  int active_index;
} ModelsFile;

typedef struct {
  bool skip_exit_confirm;
} AppSettings;

bool config_ensure_dir(void);
bool config_load_models(ModelsFile *mf);
bool config_save_models(const ModelsFile *mf);
bool config_add_model(ModelsFile *mf, const ModelConfig *model);
bool config_remove_model(ModelsFile *mf, size_t index);
ModelConfig *config_get_active(ModelsFile *mf);
void config_set_active(ModelsFile *mf, size_t index);

bool config_load_settings(AppSettings *settings);
bool config_save_settings(const AppSettings *settings);

#endif

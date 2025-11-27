#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <dirent.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char g_test_home[512] = {0};
static char g_original_home[512] = {0};

__attribute__((unused)) static int remove_callback(const char *fpath,
                                                   const struct stat *sb,
                                                   int typeflag,
                                                   struct FTW *ftwbuf) {
  (void)sb;
  (void)typeflag;
  (void)ftwbuf;
  return remove(fpath);
}

__attribute__((unused)) static void
remove_directory_recursive(const char *path) {
  nftw(path, remove_callback, 64, FTW_DEPTH | FTW_PHYS);
}

__attribute__((unused)) static void setup_test_environment(void) {
  const char *home = getenv("HOME");
  if (home) {
    strncpy(g_original_home, home, sizeof(g_original_home) - 1);
  }
  snprintf(g_test_home, sizeof(g_test_home), "/tmp/sillytui_test_%d", getpid());
  mkdir(g_test_home, 0755);
  char config_dir[600];
  snprintf(config_dir, sizeof(config_dir), "%s/.config/sillytui", g_test_home);
  char cmd[700];
  snprintf(cmd, sizeof(cmd), "mkdir -p %s", config_dir);
  system(cmd);
  snprintf(cmd, sizeof(cmd), "mkdir -p %s/.config/sillytui/chats", g_test_home);
  system(cmd);
  setenv("HOME", g_test_home, 1);
}

__attribute__((unused)) static void teardown_test_environment(void) {
  if (g_original_home[0]) {
    setenv("HOME", g_original_home, 1);
  }
  if (g_test_home[0]) {
    remove_directory_recursive(g_test_home);
  }
}

__attribute__((unused)) static void create_test_file(const char *path,
                                                     const char *content) {
  FILE *f = fopen(path, "w");
  if (f) {
    fputs(content, f);
    fclose(f);
  }
}

__attribute__((unused)) static char *read_test_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc(len + 1);
  if (buf) {
    size_t read_len = fread(buf, 1, len, f);
    buf[read_len] = '\0';
  }
  fclose(f);
  return buf;
}

__attribute__((unused)) static int file_exists(const char *path) {
  return access(path, F_OK) == 0;
}

#endif

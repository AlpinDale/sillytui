#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <windows.h>
#define unlink _unlink
#define mkdir(path, mode) _mkdir(path)
#define strcasecmp _stricmp
#define stat _stat
#define usleep(us) Sleep((us) / 1000)
#define getpid _getpid
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"

struct dirent {
  char d_name[MAX_PATH];
};

typedef struct {
  HANDLE handle;
  WIN32_FIND_DATAA find_data;
  struct dirent current;
  int first;
} DIR;

static inline DIR *opendir(const char *path) {
  DIR *dir = (DIR *)malloc(sizeof(DIR));
  if (!dir)
    return NULL;

  char search_path[MAX_PATH];
  snprintf(search_path, sizeof(search_path), "%s\\*", path);

  dir->handle = FindFirstFileA(search_path, &dir->find_data);
  if (dir->handle == INVALID_HANDLE_VALUE) {
    free(dir);
    return NULL;
  }

  dir->first = 1;
  return dir;
}

static inline struct dirent *readdir(DIR *dir) {
  if (!dir)
    return NULL;

  if (dir->first) {
    dir->first = 0;
  } else {
    if (!FindNextFileA(dir->handle, &dir->find_data))
      return NULL;
  }

  strncpy(dir->current.d_name, dir->find_data.cFileName, MAX_PATH - 1);
  dir->current.d_name[MAX_PATH - 1] = '\0';
  return &dir->current;
}

static inline int closedir(DIR *dir) {
  if (!dir)
    return -1;
  if (dir->handle != INVALID_HANDLE_VALUE)
    FindClose(dir->handle);
  free(dir);
  return 0;
}

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

#else
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

static inline const char *get_home_dir(void) {
#ifdef _WIN32
  const char *home = getenv("USERPROFILE");
  if (!home)
    home = getenv("HOMEDRIVE");
  return home;
#else
  return getenv("HOME");
#endif
}

#endif

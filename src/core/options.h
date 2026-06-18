#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>

typedef struct {
    int log_level;
    bool save_log;
    bool show_log;
    bool show_debug_info;
    unsigned short fps;
} CwOptions;

void options_load(CwOptions *opts, const char *path);
bool options_save(CwOptions *opts, const char *path);

#endif

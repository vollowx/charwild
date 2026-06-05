#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "core/options.h"
#include "core/common.h"

void options_load(CwOptions *opts, const char *path) {
    FILE *fp = fopen(path, "r");

    if (!fp) {
        warn("[options] %s not found, falling back to default options",
             path);
        return;
    }
    char key[32];
    int val;
    while (fscanf(fp, "%31[^=]=%d\n", key, &val) == 2) {
        if (strcmp(key, "log_level") == 0)
            opts->log_level = val;
        else if (strcmp(key, "save_log") == 0)
            opts->save_log = (bool)val;
        else if (strcmp(key, "show_log") == 0)
            opts->show_log = (bool)val;
        else if (strcmp(key, "fps") == 0)
            opts->fps = (unsigned short)val;
    }
    fclose(fp);

    info("[options] loaded from %s", path);
}

bool options_save(CwOptions *opts, const char *path) {
    bool ret = true;
    FILE *fp = fopen(path, "w");
    if (!fp)
        do_defer_and_return(false);
    fprintf(fp, "log_level=%d\n",      opts->log_level);
    fprintf(fp, "save_log=%d\n", (int)opts->save_log);
    fprintf(fp, "show_log=%d\n", (int)opts->show_log);
    fprintf(fp, "fps=%d\n",      opts->fps);

defer:
    if (fp)  fclose(fp);
    if (ret) info("[options] saved to %s", path);
    return ret;
}

#include <stdio.h>

#include "core/log.h"
#include "core/options.h"
#include "core/common.h"

void options_load(CwOptions *opts, const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        warn("[options] %s not found, falling back to default options", path);
        return;
    }

    char buf[256];
    CwLine line;
    while (cw_next_line(fp, buf, sizeof(buf), &line)) {
        if (line.kind != CW_LINE_KV)
            continue;
        if      (sv_eq_cstr(line.tag, "log_level")) opts->log_level = sv_to_int(line.val);
        else if (sv_eq_cstr(line.tag, "save_log"))  opts->save_log  = (bool)sv_to_int(line.val);
        else if (sv_eq_cstr(line.tag, "show_log"))  opts->show_log  = (bool)sv_to_int(line.val);
        else if (sv_eq_cstr(line.tag, "fps"))       opts->fps       = (unsigned short)sv_to_int(line.val);
    }
    fclose(fp);

    info("[options] loaded from %s", path);
}

bool options_save(CwOptions *opts, const char *path) {
    bool ret = true;
    FILE *fp = fopen(path, "w");
    if (!fp)
        do_defer_and_return(false);

    fprintf(fp, "fps       = %d\n", opts->fps);
    fprintf(fp, "log_level = %d\n", opts->log_level);
    fprintf(fp, "show_log  = %d\n", (int)opts->show_log);
    fprintf(fp, "save_log  = %d\n", (int)opts->save_log);

defer:
    if (fp)  fclose(fp);
    if (ret) info("[options] saved to %s", path);
    return ret;
}


#include "core/log.h"
#include "core/common.h"
#include "core/options.h"
#include "tui/fcp.h"
#include "tui/tui_context.h"

static WINDOW *win = NULL;
static short log_color_pairs[3];

void log_init(CwTui *ctx)
{
    info("[tui] overlay += log");

    log_color_pairs[CW_LOG_INFO]    = fcp_get(COLOR_BLUE, -1);
    log_color_pairs[CW_LOG_WARNING] = fcp_get(COLOR_YELLOW, -1);
    log_color_pairs[CW_LOG_ERROR]   = fcp_get(COLOR_RED, -1);

    int height = LOG_UI_CAPACITY + 1;
    win = newwin(height, COLS, LINES - height, 0);
}

void log_deinit(CwTui *ctx)
{
    werase(win);
    wnoutrefresh(win);
    delwin(win);
}

void log_frame(CwTui *ctx)
{
    static bool was_showing = false;

    if (!ctx->core->options.show_log) {
        if (was_showing) {
            werase(win);
            wnoutrefresh(win);
            was_showing = false;
        }
        return;
    }

    was_showing = true;

    werase(win);
    wattron(win, COLOR_PAIR(fcp_get(COLOR_MAGENTA, -1)));
    mvwhline(win, 0, 0, ACS_HLINE, COLS);
    wattroff(win, COLOR_PAIR(fcp_get(COLOR_MAGENTA, -1)));

    size_t line = LOG_UI_CAPACITY;
    size_t i = logs.count;

    while (i > 0 && line > 0) {
        const Log *log = &logs.items[--i];

        if ((int)log->level >= ctx->core->options.log_level) {
            short cp = log_color_pairs[log->level];
            wattron(win, COLOR_PAIR(cp));
            mvwprintw(win, line--, 0, "%s", log->msg);
            wattroff(win, COLOR_PAIR(cp));
        }
    }

    wnoutrefresh(win);
}

void log_resize(CwTui *ctx)
{
    int height = LOG_UI_CAPACITY + 1;
    wresize(win, height, COLS);
    mvwin(win, LINES - height, 0);
}

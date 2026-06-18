#include "core/common.h"
#include "core/log.h"
#include "tui/fcp.h"
#include "tui/tui_context.h"

#define ABOUT_HEIGHT 9
#define ABOUT_WIDTH 60

static WINDOW *win;

void about_init(CwTui *ctx)
{
    info("[tui] screen = about");

    win = newwin(ABOUT_HEIGHT, ABOUT_WIDTH, (LINES - ABOUT_HEIGHT) / 2,
                   (COLS - ABOUT_WIDTH) / 2);

    keypad(win, TRUE);
}

void about_deinit(CwTui *ctx)
{
    werase(win);
    wnoutrefresh(win);
    delwin(win);
    win = NULL;
}

void about_input(CwTui *ctx)
{
    if (ctx->ch == 'q')
        ctx->next_state = TUI_STATE_MAIN_MENU;
}

void about_frame(CwTui *ctx)
{
    draw_win_frame(win, "About", COLOR_BLUE);
    mvwprintw(win, 2, 4, "charwild v%d.%d.%d", CW_VERSION_MAJOR,
              CW_VERSION_MINOR, CW_VERSION_PATCH);
    mvwprintw(win, 3, 4, "    developed by Lucas X. Zhao");
    mvwprintw(win, 4, 4, "    licensed under Apache-2.0");
    mvwprintw(win, 6, 4, "charwild is a survival sandbox game in TUI.");
    wnoutrefresh(win);
}

void about_resize(CwTui *ctx)
{
    mvwin(win, (LINES - ABOUT_HEIGHT) / 2, (COLS - ABOUT_WIDTH) / 2);
}

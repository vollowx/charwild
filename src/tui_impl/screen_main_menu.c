#include <curses.h>
#include <menu.h>
#include "core/common.h"
#include "core/log.h"
#include "core/world.h"
#include "tui/fcp.h"
#include "tui/tui_common.h"
#include "tui/tui_context.h"

#define MAIN_MENU_HEIGHT 9
#define MAIN_MENU_WIDTH 32
#define MAIN_MENU_N_ITEMS 4

static ITEM **items;
static MENU *menu;
static WINDOW *win;
static WINDOW *map_win;

static World splash_world = {0};

void main_menu_init(CwTui *ctx)
{
    info("[tui] screen = main_menu");

    char *labels[] = {
        "Start Game",
        "Options",
        "About",
        "Quit",
        (char *)NULL,
    };

    items = (ITEM **)calloc(MAIN_MENU_N_ITEMS + 1, sizeof(ITEM *));
    for (int i = 0; i < MAIN_MENU_N_ITEMS; ++i)
        items[i] = new_item(labels[i], "");

    menu = new_menu(items);
    win =
        newwin(MAIN_MENU_HEIGHT, MAIN_MENU_WIDTH,
               (LINES - MAIN_MENU_HEIGHT) / 2, (COLS - MAIN_MENU_WIDTH) / 2);
    // TODO: o dear! The layout calculation is a garbage dump
    // TODO: handle resizing
    map_win =
        newwin(7, 14,
               (LINES - MAIN_MENU_HEIGHT) / 2 + 1, (COLS - MAIN_MENU_WIDTH) / 2 + 16);

    keypad(win, TRUE);

    set_menu_win(menu, win);
    set_menu_sub(
        menu, derwin(win, MAIN_MENU_HEIGHT - 4, MAIN_MENU_WIDTH - 4, 2, 1));
    set_menu_mark(menu, " > ");
    set_menu_fore(menu,
                  COLOR_PAIR(fcp_get(COLOR_BLUE, -1)) | A_BOLD | A_REVERSE);
    post_menu(menu);

    if (!splash_world.player)
        world_init(&splash_world, 50, 50, ctx->core);
    // Not exactly 7x7 for a bigger chance to find a ground
}

void main_menu_deinit(CwTui *ctx)
{
    werase(win);
    wnoutrefresh(win);
    free_menu_ctx(win, menu, items, MAIN_MENU_N_ITEMS, false);
    werase(map_win);
    wnoutrefresh(map_win);
    delwin(map_win);
}

void main_menu_input(CwTui *ctx)
{
    switch (ctx->ch) {
    case 'j':
        menu_driver(menu, REQ_DOWN_ITEM);
        break;
    case 'k':
        menu_driver(menu, REQ_UP_ITEM);
        break;
    case 'q':
        ctx->next_state = TUI_STATE_QUIT;
        break;
    case 10: {
        int index = item_index(current_item(menu));
        switch (index) {
        case 0:
            ctx->next_state = TUI_STATE_SAVES;
            break;
        case 1:
            ctx->next_state = TUI_STATE_OPTIONS;
            break;
        case 2:
            ctx->next_state = TUI_STATE_ABOUT;
            break;
        case 3:
            ctx->next_state = TUI_STATE_QUIT;
            break;
        }
        break;
    }
    }
}

void main_menu_frame(CwTui *ctx)
{
    draw_win_frame(win, "charwild", COLOR_BLUE);
    wnoutrefresh(win);
    draw_world(map_win, &splash_world, ctx);
}

void main_menu_resize(CwTui *ctx)
{
    mvwin(win, (LINES - MAIN_MENU_HEIGHT) / 2, (COLS - MAIN_MENU_WIDTH) / 2);
    mvwin(map_win, (LINES - MAIN_MENU_HEIGHT) / 2 + 1,
                   (COLS - MAIN_MENU_WIDTH) / 2 + 16);
}

#include <menu.h>
#include "core/common.h"
#include "core/log.h"
#include "core/save.h"
#include "tui/fcp.h"
#include "tui/tui_context.h"

// menu height = 3, padding = 1 * 2, border = 1 * 2
#define SAVES_HEIGHT 7
#define SAVES_WIDTH 48
#define PREVIEW_HEIGHT 24
#define PREVIEW_WIDTH 56

static ITEM **items = NULL;
static MENU *menu = NULL;
static WINDOW *win = NULL;
static WINDOW *preview_win = NULL;
static SavePreview previews[3];

void rebuild_saves_menu(CwTui *ctx)
{
    if (menu && items) { // Not first run
        unpost_menu(menu);
        free_menu(menu);

        for (int i = 0; i < 3; i++) {
            free((void *)item_name(items[i]));
            free_item(items[i]);
        }
        free(items);
        items = NULL;
    }

    items = (ITEM **)calloc(4, sizeof(ITEM *));

    for (int i = 0; i < 3; i++) {
        char label[SAVES_WIDTH - 6];

        previews[i] = save_preview(i);
        snprintf(label, sizeof(label), "Slot %d: %33s", i,
                 previews[i].exists ? previews[i].header.player_name
                                    : "<Empty>");

        items[i] = new_item(strdup(label), "");
    }
    items[3] = NULL;

    menu = new_menu(items);
    set_menu_win(menu, win);
    set_menu_sub(menu,
                 derwin(win, SAVES_HEIGHT - 4, SAVES_WIDTH - 4, 2, 1));
    set_menu_mark(menu, " > ");
    set_menu_fore(menu,
                  COLOR_PAIR(fcp_get(COLOR_BLUE, -1)) | A_BOLD | A_REVERSE);
    set_current_item(menu, items[ctx->core->current_slot]);

    post_menu(menu);
}

void saves_init(CwTui *ctx)
{
    info("[tui] screen = saves");

    //                          gap
    int total_w = SAVES_WIDTH + 1 + PREVIEW_WIDTH;
    int start_x = (COLS - total_w) / 2;
    int start_y = (LINES - PREVIEW_HEIGHT) / 2;

    win = newwin(SAVES_HEIGHT, SAVES_WIDTH, start_y, start_x);
    preview_win = newwin(PREVIEW_HEIGHT, PREVIEW_WIDTH, start_y,
                   start_x + SAVES_WIDTH + 1);

    keypad(win, TRUE);
    rebuild_saves_menu(ctx);
}

void saves_deinit(CwTui *ctx)
{
    werase(win);
    werase(preview_win);
    wnoutrefresh(win);
    wnoutrefresh(preview_win);

    free_menu_ctx(win, menu, items, 3, true);

    delwin(preview_win);
    preview_win = NULL;
}

void saves_input(CwTui *ctx)
{
    int slot = item_index(current_item(menu));

    ITEM *cur = current_item(menu);
    if (cur) {
        ctx->core->current_slot = item_index(cur);
    }

    switch (ctx->ch) {
    case KEY_DOWN:
    case 'j':
        menu_driver(menu, REQ_DOWN_ITEM);
        break;
    case KEY_UP:
    case 'k':
        menu_driver(menu, REQ_UP_ITEM);
        break;
    case 'q':
        ctx->next_state = TUI_STATE_MAIN_MENU;
        break;

    case 10:
    case 'o':
        if (previews[slot].exists) {
            ctx->next_state = TUI_STATE_GAMEPLAY;
        } else {
            World world = {0};
            world_init(&world, ctx->core);
            world_save(&world, slot);
            world_free(&world);

            rebuild_saves_menu(ctx);
        }

        break;

    case 'x':
        if (!previews[slot].exists)
            break;
        // next_state = STATE_SAVE_DELETE
        save_delete(slot);
        rebuild_saves_menu(ctx);
        break;
    case 'r':
        if (!previews[slot].exists)
            break;

        echo();
        curs_set(1);

        char new_name[32];

        mvwprintw(preview_win, 2, 22, "%32s", "");
        wmove(preview_win, 2, 22);
        wgetnstr(preview_win, new_name, 31);

        noecho();
        curs_set(0);

        if (strlen(new_name) > 0) {
            World world = {0};
            if (world_load(&world, slot, ctx->core) == SAVE_OK) {
                strcpy(world.player->name, new_name);
                if (world_save(&world, slot) == SAVE_OK) {
                    info("[save] Renamed slot %d to %s", slot, new_name);
                } else {
                    error("[save] Failed to rename slot saving save");
                }
            } else {
                error("[save] Failed to rename slot loading save");
            }
            world_free(&world);
        }

        rebuild_saves_menu(ctx);
        break;
    }
}

void saves_frame(CwTui *ctx)
{
    draw_win_frame(win, "Select Save", COLOR_BLUE);
    wnoutrefresh(win);

    werase(preview_win);
    draw_win_frame(preview_win, "Preview", COLOR_CYAN);

    int idx = item_index(current_item(menu));
    if (previews[idx].exists) {
        wattron(preview_win, COLOR_PAIR(1));
        mvwprintw(preview_win, 2, 2, "%16s", "player");
        wattroff(preview_win, COLOR_PAIR(1));
        mvwprintw(preview_win, 2, 22, "%s", previews[idx].header.player_name);

        wattron(preview_win, A_DIM);
        mvwhline(preview_win, 3, 22, ACS_HLINE, 31);
        wattroff(preview_win, A_DIM);

        mvwprintw(preview_win, 4, 2, "%16s", "version");
        mvwprintw(preview_win, 4, 22, "%d", previews[idx].header.version);
    } else {
        wattron(preview_win, A_DIM);
        mvwprintw(preview_win, 2, 4, "Empty Slot");
        mvwprintw(preview_win, 3, 4, "No data available.");
        wattroff(preview_win, A_DIM);
    }
    wnoutrefresh(preview_win);
}

void saves_resize(CwTui *ctx)
{
    int total_w = SAVES_WIDTH + 1 + PREVIEW_WIDTH;
    int start_x = (COLS - total_w) / 2;
    int start_y = (LINES - PREVIEW_HEIGHT) / 2;

    mvwin(win, start_y, start_x);
    mvwin(preview_win, start_y, start_x + SAVES_WIDTH + 1);

    rebuild_saves_menu(ctx);
}

#include <menu.h>

#include "core/common.h"
#include "core/log.h"
#include "core/save.h"
#include "ui/fcp.h"
#include "ui/tui_context.h"

// menu height = 3, padding = 1 * 2, border = 1 * 2
#define SAVES_HEIGHT 7
#define SAVES_WIDTH 48
#define PREVIEW_HEIGHT 24
#define PREVIEW_WIDTH 56

static ITEM **s_items = NULL;
static MENU *s_menu = NULL;
static WINDOW *s_win = NULL;
static WINDOW *s_pre = NULL;
static SavePreview previews[3];

void rebuild_saves_menu(CwTui *ctx) {
    if (s_menu && s_items) { // Not first run
        unpost_menu(s_menu);
        free_menu(s_menu);

        for (int i = 0; i < 3; i++) {
            free((void *)item_name(s_items[i]));
            free_item(s_items[i]);
        }
        free(s_items);
        s_items = NULL;
    }

    s_items = (ITEM **)calloc(4, sizeof(ITEM *));

    for (int i = 0; i < 3; i++) {
        char label[SAVES_WIDTH - 6];

        previews[i] = save_preview(i);
        snprintf(label, sizeof(label), "Slot %d: %33s", i,
                 previews[i].exists ? previews[i].header.player_name
                                    : "<Empty>");

        s_items[i] = new_item(strdup(label), "");
    }
    s_items[3] = NULL;

    s_menu = new_menu(s_items);
    set_menu_win(s_menu, s_win);
    set_menu_sub(s_menu,
                 derwin(s_win, SAVES_HEIGHT - 4, SAVES_WIDTH - 4, 2, 1));
    set_menu_mark(s_menu, " > ");
    set_menu_fore(s_menu,
                  COLOR_PAIR(fcp_get(COLOR_BLUE, -1)) | A_BOLD | A_REVERSE);
    set_current_item(s_menu, s_items[ctx->core->current_slot]);

    post_menu(s_menu);
}

void saves_init(CwTui *ctx) {
    info("[tui] screen = saves");

    //                          gap
    int total_w = SAVES_WIDTH + 1 + PREVIEW_WIDTH;
    int start_x = (COLS - total_w) / 2;
    int start_y = (LINES - PREVIEW_HEIGHT) / 2;

    s_win = newwin(SAVES_HEIGHT, SAVES_WIDTH, start_y, start_x);
    s_pre = newwin(PREVIEW_HEIGHT, PREVIEW_WIDTH, start_y,
                   start_x + SAVES_WIDTH + 1);

    keypad(s_win, TRUE);
    rebuild_saves_menu(ctx);
}

void saves_deinit(CwTui *ctx) {
    werase(s_win);
    werase(s_pre);
    wnoutrefresh(s_win);
    wnoutrefresh(s_pre);

    free_menu_ctx(s_win, s_menu, s_items, 3, true);

    delwin(s_pre);
    s_pre = NULL;
}

void saves_input(CwTui *ctx) {
    int slot = item_index(current_item(s_menu));

    ITEM *cur = current_item(s_menu);
    if (cur) {
        ctx->core->current_slot = item_index(cur);
    }

    switch (ctx->ch) {
    case KEY_DOWN:
    case 'j':
        menu_driver(s_menu, REQ_DOWN_ITEM);
        break;
    case KEY_UP:
    case 'k':
        menu_driver(s_menu, REQ_UP_ITEM);
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

        mvwprintw(s_pre, 2, 22, "%32s", "");
        wmove(s_pre, 2, 22);
        wgetnstr(s_pre, new_name, 31);

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

void saves_frame(CwTui *ctx) {
    draw_win_frame(s_win, "Select Save", COLOR_BLUE);
    wnoutrefresh(s_win);

    werase(s_pre);
    draw_win_frame(s_pre, "Preview", COLOR_CYAN);

    int idx = item_index(current_item(s_menu));
    if (previews[idx].exists) {
        wattron(s_pre, COLOR_PAIR(1));
        mvwprintw(s_pre, 2, 2, "%16s", "player");
        wattroff(s_pre, COLOR_PAIR(1));
        mvwprintw(s_pre, 2, 22, "%s", previews[idx].header.player_name);

        wattron(s_pre, A_DIM);
        mvwhline(s_pre, 3, 22, ACS_HLINE, 31);
        wattroff(s_pre, A_DIM);

        mvwprintw(s_pre, 4, 2, "%16s", "version");
        mvwprintw(s_pre, 4, 22, "%d", previews[idx].header.version);
    } else {
        wattron(s_pre, A_DIM);
        mvwprintw(s_pre, 2, 4, "Empty Slot");
        mvwprintw(s_pre, 3, 4, "No data available.");
        wattroff(s_pre, A_DIM);
    }
    wnoutrefresh(s_pre);
}

void saves_resize(CwTui *ctx) {
    int total_w = SAVES_WIDTH + 1 + PREVIEW_WIDTH;
    int start_x = (COLS - total_w) / 2;
    int start_y = (LINES - PREVIEW_HEIGHT) / 2;

    mvwin(s_win, start_y, start_x);
    mvwin(s_pre, start_y, start_x + SAVES_WIDTH + 1);

    rebuild_saves_menu(ctx);
}

#include "core/log.h"
#include "core/save.h"
#include "core/common.h"
#include "core/definitions.h"
#include "core/world.h"
#include "tui/tui_common.h"
#include "tui/tui_context.h"

static WINDOW *win = NULL;
static WINDOW *inventory_win = NULL;
static WINDOW *debug_info_win = NULL;
static bool needs_redraw = true;
static int item_selected = 1; // [1, 9]

void gameplay_init(CwTui *ctx)
{
    info("[tui] screen = gameplay");

    if (world_load(&ctx->core->current_world, ctx->core->current_slot, ctx->core) != SAVE_OK) {
        ctx->next_state = TUI_STATE_MAIN_MENU;
        return;
    }

    win            = newwin(LINES, COLS, 0, 0);
    inventory_win  = newwin(11, 8, 1, 2);
    debug_info_win = newwin(4, 32, 1, COLS - 34);
    keypad(win, TRUE);
}

void gameplay_deinit(CwTui *ctx)
{
    world_save(&ctx->core->current_world, ctx->core->current_slot);
    world_free(&ctx->core->current_world);
    ctx->core->current_world = (World){0};

    werase(win);
    wnoutrefresh(win);
    delwin(win);
    win = NULL;
    delwin(inventory_win);
    inventory_win = NULL;
    delwin(debug_info_win);
    debug_info_win = NULL;
    needs_redraw = true;
}

void gameplay_input(CwTui *ctx)
{
    Map *map = ctx->core->current_world.map;
    Entity *player = ctx->core->current_world.player;

    // TASK(20260227-142821): Redesign player movement, consider add into
    // world_tick and add velocity
    switch (ctx->ch) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        item_selected = ctx->ch - '0';
        break;
    case 'k':
        needs_redraw = entity_move(player, map, 0, -1);
        break;
    case 'j':
        needs_redraw = entity_move(player, map, 0, 1);
        break;
    case 'h':
        needs_redraw = entity_move(player, map, -1, 0);
        break;
    case 'l':
        needs_redraw = entity_move(player, map, 1, 0);
        break;
        // TASK(20260226-155803): Add object related functions
    case 'K':
        cell_ref(map, player->y - 1, player->x)->object_id = 0;
        needs_redraw = true;
        break;
    case 'J':
        cell_ref(map, player->y + 1, player->x)->object_id = 0;
        needs_redraw = true;
        break;
    case 'H':
        cell_ref(map, player->y, player->x - 1)->object_id = 0;
        needs_redraw = true;
        break;
    case 'L':
        cell_ref(map, player->y, player->x + 1)->object_id = 0;
        needs_redraw = true;
        break;
    case '':
        needs_redraw = entity_place_object(player, map, 10000, 0, -1);
        break;
    case 10: // Vim not inputting ^J somehow
        needs_redraw = entity_place_object(player, map, 10000, 0, 1);
        break;
    case '':
        needs_redraw = entity_place_object(player, map, 10000, -1, 0);
        break;
    case '':
        needs_redraw = entity_place_object(player, map, 10000, 1, 0);
        break;
    case 'q':
        ctx->next_state = TUI_STATE_SAVES;
        break;
    }
}

void gameplay_frame(CwTui *ctx)
{
    static double tick_accumulator = 0;
    const double tick_rate = 1.0 / CW_TPS;

    tick_accumulator += ctx->frame_time;
    while (tick_accumulator >= tick_rate) {
        // game_tick should be called 20 times a sec
        if (world_tick(&ctx->core->current_world, tick_rate)) {
            needs_redraw = true;
        }
        tick_accumulator -= tick_rate;
    }

    if (!needs_redraw)
        return;

    Map *map = ctx->core->current_world.map;
    Entity *player = ctx->core->current_world.player;

    draw_world(win, &ctx->core->current_world, ctx);

    werase(inventory_win);
    wattrset(inventory_win, A_NORMAL);
    draw_win_frame(inventory_win, NULL, COLOR_CYAN);
    int i = 1;
    da_foreach(Item, item, &player->inventory) {
        short attr = COLOR_PAIR(fcp_get(item->def->fg, item->def->bg));
        wattrset(inventory_win, attr);
        wmove(   inventory_win, i, 1);
        waddch(  inventory_win, item->def->symbol[0]);
        waddch(  inventory_win, item->def->symbol[1]);
        wattrset(inventory_win, i == item_selected ? A_STANDOUT : A_NORMAL);
        wprintw( inventory_win, " %3d", item->stack ? item->stack : item->durability);
        i += 1;
    }
    wnoutrefresh(inventory_win);

    static bool was_showing = false;
    if (!ctx->core->options.show_debug_info) {
        if (was_showing) {
            werase(debug_info_win);
            wnoutrefresh(debug_info_win);
            was_showing = false;
        }
        needs_redraw = false;
        return;
    }
    was_showing = true;

    werase(debug_info_win);
    draw_win_frame(debug_info_win, "de Buggin'", COLOR_RED);
    mvwprintw(debug_info_win, 1, 1, "x, y, z: %zu, %zu, %d", player->x, player->y,
              cell_ref(map, player->y, player->x)->elevation);
    mvwprintw(debug_info_win, 2, 1, "entities: %zu", ctx->core->current_world.entities.count);
    wnoutrefresh(debug_info_win);

    needs_redraw = false;
}

void gameplay_resize(CwTui *ctx)
{
    wresize(win, LINES, COLS);
    mvwin(win, 0, 0);
    wresize(inventory_win, 11, 8);
    mvwin(inventory_win, 1, 2);
    wresize(debug_info_win, 4, 32);
    mvwin(debug_info_win, 1, COLS - 34);
    needs_redraw = true;
}

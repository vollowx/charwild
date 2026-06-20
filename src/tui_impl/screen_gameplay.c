#include "core/log.h"
#include "core/save.h"
#include "core/common.h"
#include "core/definitions.h"
#include "core/world.h"
#include "tui/fcp.h"
#include "tui/tui_context.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

typedef struct {
    char symbol;
    short fg;
    short bg;

    short cp; // Registered color pair at runtime
} CellVisualDef;

static CellVisualDef cell_visual_defs[] = {
    [ELEV_NONE]       = { '?', COLOR_BLACK, COLOR_MAGENTA},
    [ELEV_DEEP_WATER] = { '~', COLOR_BLACK, COLOR_BLUE},
    [ELEV_WATER]      = { ' ', COLOR_WHITE, COLOR_BLUE},
    [ELEV_GROUND]     = { ' ', COLOR_BLACK, COLOR_GREEN},
    [ELEV_HILL]       = { '^', COLOR_BLACK, COLOR_GREEN},
    [ELEV_MOUNTAIN]   = { '*', COLOR_BLACK, COLOR_GREEN},
};

static WINDOW *win = NULL;
static WINDOW *inventory_win = NULL;
static WINDOW *debug_info_win = NULL;
static bool needs_redraw = true;
static int item_selected = 1; // [1, 9]

void gameplay_init(CwTui *ctx)
{
    info("[tui] screen = gameplay");

    static bool is_first_run = true;
    if (is_first_run) {
        is_first_run = false;

        for (int i = 0; i < _elevation_count; ++i)
            cell_visual_defs[i].cp =
                fcp_get(cell_visual_defs[i].fg, cell_visual_defs[i].bg);
    }

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
    case KEY_UP:
    case 'k':
        needs_redraw = entity_move(player, map, 0, -1);
        break;
    case KEY_DOWN:
    case 'j':
        needs_redraw = entity_move(player, map, 0, 1);
        break;
    case KEY_LEFT:
    case 'h':
        needs_redraw = entity_move(player, map, -1, 0);
        break;
    case KEY_RIGHT:
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

    int scr_w, scr_h;
    getmaxyx(win, scr_h, scr_w);
    int scr_w_cells = scr_w / 2;

    int draw_h = MIN((int)map->h, scr_h);
    int draw_w = MIN((int)map->w, scr_w_cells);

    int max_scroll_y = MAX(0, (int)map->h - scr_h);
    int max_scroll_x = MAX(0, (int)map->w - scr_w_cells);

    size_t map_y1 = CLAMP((int)player->y - scr_h / 2,       0, max_scroll_y);
    size_t map_x1 = CLAMP((int)player->x - scr_w_cells / 2, 0, max_scroll_x);
    int screen_y1 = (scr_h - draw_h) / 2;
    int screen_x1 = (scr_w - (draw_w * 2)) / 2; // * 2 accounts for 2-char wide cells

    werase(win);
    attr_t current_attrs = A_NORMAL;
    wattrset(win, current_attrs);

    for (int y = 0; y < draw_h; ++y) {
        wmove(win, screen_y1 + y, screen_x1);

        for (int x = 0; x < draw_w; ++x) {
            Cell *cell = cell_ref(map, map_y1 + y, map_x1 + x);

            const CellVisualDef *cell_def = &cell_visual_defs[cell->elevation];
            char symbol[2] = {cell_def->symbol, cell_def->symbol};
            short fg = cell_def->fg, bg = cell_def->bg;
            short cp = cell_def->cp;
            int attr = A_NORMAL;

            if (cell->object_id) {
                ObjectDef *def = object_def_lookup(ctx->core->object_defs, cell->object_id);
                symbol[0] = def->symbol[0];
                symbol[1] = def->symbol[1];
                if (def->fg != -1)
                    fg = def->fg;
                if (def->bg != -1)
                    bg = def->bg;
                cp = fcp_get(fg, bg);

                if (def->attr)
                    attr = def->attr;
            }
            if (cell->entity) {
                const EntityDef *def = cell->entity->def;
                symbol[0] = def->symbol[0];
                symbol[1] = def->symbol[1];
                if (def->fg != -1)
                    fg = def->fg;
                if (def->bg != -1)
                    bg = def->bg;
                cp = fcp_get(fg, bg);

                if (def->attr)
                    attr = def->attr;
            }

            attr_t new_attrs = COLOR_PAIR(cp) | attr;

            if (new_attrs != current_attrs) {
                wattrset(win, new_attrs);
                current_attrs = new_attrs;
            }

            waddch(win, symbol[0]);
            waddch(win, symbol[1]);
        }
    }

    wnoutrefresh(win);

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

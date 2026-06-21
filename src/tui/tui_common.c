#include <assert.h>
#include <stdio.h>
#include <ncurses.h>
#include "core/definitions.h"
#include "tui/tui_common.h"
#include "tui/tui_context.h"
#include "tui/fcp.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

void wprintwattr(WINDOW *window, attr_t attr, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    wattron(window, attr);
    vw_printw(window, format, args);
    wattroff(window, attr);
    va_end(args);
}

void mvwprintwattr(WINDOW *window, int y, int x, attr_t attr, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    wmove(window, y, x);
    wattron(window, attr);
    vw_printw(window, format, args);
    wattroff(window, attr);
    va_end(args);
}

static CellVisualDef cell_visual_defs[] = {
    [ELEV_NONE]       = { '?', COLOR_BLACK, COLOR_MAGENTA},
    [ELEV_DEEP_WATER] = { '~', COLOR_BLACK, COLOR_BLUE},
    [ELEV_WATER]      = { ' ', COLOR_WHITE, COLOR_BLUE},
    [ELEV_GROUND]     = { ' ', COLOR_BLACK, COLOR_GREEN},
    [ELEV_HILL]       = { '^', COLOR_BLACK, COLOR_GREEN},
    [ELEV_MOUNTAIN]   = { '*', COLOR_BLACK, COLOR_GREEN},
};

void init_cell_color_pairs(void)
{
    for (int i = 0; i < _elevation_count; ++i)
        cell_visual_defs[i].cp =
            fcp_get(cell_visual_defs[i].fg, cell_visual_defs[i].bg);
}

void draw_world(WINDOW *win, World *world, CwTui *ctx)
{
    Map *map = world->map;
    Entity *player = world->player;

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
    attr_t current_attr = A_NORMAL;
    wattrset(win, current_attr);

    for (int y = 0; y < draw_h; ++y) {
        wmove(win, screen_y1 + y, screen_x1);

        for (int x = 0; x < draw_w; ++x) {
            Cell *cell = cell_ref(map, map_y1 + y, map_x1 + x);

            // TODO: assert(def is found)
            const CellVisualDef *cell_def = &cell_visual_defs[cell->elevation];
            char symbol[2] = {cell_def->symbol, cell_def->symbol};
            short fg = cell_def->fg, bg = cell_def->bg;
            short cp = cell_def->cp;
            int attr = A_NORMAL;

            if (cell->object_id) {
                ObjectDef *def = object_def_lookup(ctx->core->object_defs, cell->object_id);
                assert(def);
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

            if (new_attrs != current_attr) {
                wattrset(win, new_attrs);
                current_attr = new_attrs;
            }

            waddch(win, symbol[0]);
            waddch(win, symbol[1]);
        }
    }

    wnoutrefresh(win);
}

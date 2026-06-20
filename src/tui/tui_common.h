// Renderers and utilities for TUI

#ifndef TUI_COMMON_H
#define TUI_COMMON_H

#include <ncurses.h>
#include "tui/fcp.h"
#include "tui/tui_context.h"

void wprintwattr(WINDOW *, attr_t, const char *, ...);

#define draw_win_frame(win, title, color)                                      \
    do {                                                                       \
        wattron(win, COLOR_PAIR(fcp_get(color, -1)));                          \
        box(win, 0, 0);                                                        \
        wattron(win, A_BOLD);                                                  \
        if (title != NULL)                                                     \
            mvwprintw(win, 0, 3, " %s ", title);                               \
        wattroff(win, COLOR_PAIR(fcp_get(color, -1)) | A_BOLD);                \
    } while (0)

#define free_menu_ctx(win, menu, items, n_items, owns_labels)                  \
    do {                                                                       \
        if (menu) {                                                            \
            unpost_menu(menu);                                                 \
            WINDOW *sub = menu_sub(menu);                                      \
            if (sub)                                                           \
                delwin(sub);                                                   \
            free_menu(menu);                                                   \
            menu = NULL;                                                       \
        }                                                                      \
        if (items) {                                                           \
            for (int i = 0; i < n_items; i++) {                                \
                if (owns_labels)                                               \
                    free((void *)item_name(items[i]));                         \
                free_item(items[i]);                                           \
            }                                                                  \
            free(items);                                                       \
            items = NULL;                                                      \
        }                                                                      \
        delwin(win);                                                           \
        win = NULL;                                                            \
    } while (0)

typedef struct {
    char symbol;
    short fg;
    short bg;

    short cp; // Registered color pair at runtime
} CellVisualDef;

void init_cell_color_pairs(void);
void draw_world(WINDOW *win, World *world, CwTui *ctx);

#endif

#include <menu.h>
#include "core/common.h"
#include "core/log.h"
#include "core/options.h"
#include "tui/fcp.h"
#include "tui/tui_context.h"

#define OPTIONS_HEIGHT 32
#define OPTIONS_WIDTH 56

static ITEM **items = NULL;
static MENU *menu = NULL;
static WINDOW *win = NULL;

ITEM *new_caption(const char *title)
{
    ITEM *item = new_item(title, "");
    item_opts_off(item, O_SELECTABLE);
    return item;
}

void format_menu_item(char *dest, size_t size, const char *label,
                      const char *value)
{
    int label_width = 16;
    int total_width = OPTIONS_WIDTH - 8;
    snprintf(dest, size, "%-*s %*s", label_width, label,
             (total_width - label_width), value);
}

void rebuild_options_menu(CwTui *ctx)
{
    int current_idx = 0;

    if (menu && items) { // Not first run
        ITEM *cur = current_item(menu);
        if (cur) {
            current_idx = item_index(cur);
        }
        unpost_menu(menu);
        free_menu(menu);
        menu = NULL;

        for (int i = 0; i < OPTIONS_HEIGHT - 4 + 1; i++) {
            free_item(items[i]);
        }
        free(items);
        items = NULL;
    }

    items = (ITEM **)calloc(OPTIONS_HEIGHT - 4 + 1, sizeof(ITEM *));
    int i = 0;

    items[i++] = new_caption("  Game");
    items[i++] = new_item("Foo", "");
    items[i++] = new_item("Bar", "");
    items[i++] = new_item("Baz", "");

    items[i++] = new_caption("  Display");
    items[i++] = new_item("Zoom", "");

    items[i++] = new_caption("  Other");

    static char log_level_line[64], log_show_line[64], log_save_line[64];
    const char *lvls[] = {"<Information>", "<Warning>", "<Error>"};

    format_menu_item(log_level_line, sizeof(log_level_line), "Show log level",
                     lvls[ctx->core->options.log_level]);
    format_menu_item(log_show_line, sizeof(log_show_line), "Show log window",
                     ctx->core->options.show_log ? "[x]" : "[ ]");
    format_menu_item(log_save_line, sizeof(log_save_line), "Save log locally",
                     ctx->core->options.save_log ? "[x]" : "[ ]");

    items[i++] = new_item(log_level_line, "");
    items[i++] = new_item(log_show_line, "");
    items[i++] = new_item(log_save_line, "");
    items[i++] = new_item("Clear logs", "");
    items[i++] = new_item("Clear local logs", "");

    items[i++] = new_caption(" ");
    items[i++] = new_item("Save", "");
    items[i++] = new_item("Cancel", "");
    items[i++] = NULL;

    menu = new_menu(items);
    set_menu_win(menu, win);
    set_menu_sub(menu,
                 derwin(win, OPTIONS_HEIGHT - 4, OPTIONS_WIDTH - 4, 2, 1));
    set_menu_mark(menu, " > ");
    set_menu_fore(menu,
                  COLOR_PAIR(fcp_get(COLOR_BLUE, -1)) | A_BOLD | A_REVERSE);
    set_current_item(menu, items[current_idx]);

    post_menu(menu);
}

void options_init(CwTui *ctx)
{
    info("[tui] screen = options");

    win = newwin(OPTIONS_HEIGHT, OPTIONS_WIDTH, (LINES - OPTIONS_HEIGHT) / 2,
                   (COLS - OPTIONS_WIDTH) / 2);
    keypad(win, TRUE);
    rebuild_options_menu(ctx);
}

void options_deinit(CwTui *ctx)
{
    werase(win);
    wnoutrefresh(win);
    free_menu_ctx(win, menu, items, OPTIONS_HEIGHT - 4, false);
}

void options_input(CwTui *ctx)
{
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
        options_load(&ctx->core->options, CW_OPTIONS_PATH);
        ctx->next_state = TUI_STATE_MAIN_MENU;
        break;
    case 10: {
        ITEM *cur = current_item(menu);
        const char *name = item_name(cur);

        if (strstr(name, "Show log level")) {
            ctx->core->options.log_level = (ctx->core->options.log_level + 1) % 3;
            rebuild_options_menu(ctx);
        } else if (strstr(name, "Show log window")) {
            ctx->core->options.show_log = !ctx->core->options.show_log;
            rebuild_options_menu(ctx);
        } else if (strcmp(name, "Save") == 0) {
            options_save(&ctx->core->options, CW_OPTIONS_PATH);
            ctx->next_state = TUI_STATE_MAIN_MENU;
        } else if (strcmp(name, "Cancel") == 0) {
            options_load(&ctx->core->options, CW_OPTIONS_PATH);
            ctx->next_state = TUI_STATE_MAIN_MENU;
        }
        break;
    }
    }
}

void options_frame(CwTui *ctx)
{
    draw_win_frame(win, "Options", COLOR_BLUE);
    wnoutrefresh(win);
}

void options_resize(CwTui *ctx)
{
    mvwin(win, (LINES - OPTIONS_HEIGHT) / 2, (COLS - OPTIONS_WIDTH) / 2);
}

#include <time.h>

#include "core/common.h"
#include "core/log.h"
#include "core/options.h"
#include "core/world_defs.h"
#include "ui/fcp.h"
#include "ui/tui_context.h"

int main(int argc, char *argv[]) {
    Cw core_ctx = {
        .options = {
            .log_level = 0,
            .show_log = true,
        },
        .cur_slot = 0,
    };
    CwTui ctx = {
        .cur_state = (CwTuiState)-1,
        .next_state = TUI_STATE_MAIN_MENU,
        .core = &core_ctx,
    };

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (!has_colors()) {
        printw("Your terminal does not support color. Press any key to quit.");
        getch();
        return 1;
    }

    start_color();
    use_default_colors();
    fcp_init();

    definitions_load(
        "base.cwdef",
        &ctx.core->item_defs,
        &ctx.core->entity_defs,
        &ctx.core->object_defs
    );
    options_load(&ctx.core->options, CW_OPTIONS_PATH);

    log_init(&ctx);

    struct timespec last_frame, current_frame;
    clock_gettime(CLOCK_MONOTONIC, &last_frame);

    while (ctx.next_state != TUI_STATE_QUIT) {
        clock_gettime(CLOCK_MONOTONIC, &current_frame);
        ctx.frame_time = (current_frame.tv_sec - last_frame.tv_sec) +
                         (current_frame.tv_nsec - last_frame.tv_nsec) / 1e9;
        last_frame = current_frame;

        if (ctx.next_state != ctx.cur_state) {
            if (ctx.cur_screen) {
                ctx.cur_screen->deinit();
            }

            ctx.cur_state = ctx.next_state;
            ctx.cur_screen = CW_SCREENS[ctx.cur_state];

            ctx.cur_screen->init(&ctx);
        }

        ctx.ch = getch();
        if (ctx.ch == KEY_RESIZE) {
            erase();
            ctx.cur_screen->resize(&ctx);
            log_resize(&ctx);
            refresh();
        } else {
            ctx.cur_screen->input(&ctx);
        }

        ctx.cur_screen->frame(&ctx);
        log_frame(&ctx);

        doupdate();
        napms(1000 / CW_FPS);
    }

    ctx.cur_screen->deinit();
    log_deinit();
    endwin();

    free_logs();

    da_free(core_ctx.item_defs);
    da_free(core_ctx.entity_defs);
    da_free(core_ctx.object_defs);

    return 0;
}

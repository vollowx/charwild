#include <time.h>
#include <unistd.h>
#include "core/common.h"
#include "core/log.h"
#include "core/definitions.h"
#include "core/options.h"
#include "tui/tui_context.h"

#ifdef CW_USE_ASAN
void __asan_on_error(void)
{
    if (!isendwin())
        endwin();
    log_print_all(stderr);
}
#else
#include <signal.h>
static void sig_handler(int sig)
{
    if (!isendwin())
        endwin();
    log_print_all(stderr);

    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

int main(int argc, char *argv[])
{
#ifndef CW_USE_ASAN
    signal(SIGSEGV, sig_handler);
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGABRT, sig_handler);
#endif

    srand((unsigned int) time(NULL));

    Cw core_ctx = {
        .options = {
            .fps = 60,
            .log_level = 0,
            .show_log = true,
        },
        .current_slot = 0,
    };
    CwTui ctx = {
        .current_state = (CwTuiState)-1,
        .next_state = TUI_STATE_MAIN_MENU,
        .core = &core_ctx,
    };

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    // nodelay(stdscr, TRUE);
    timeout(1000 / core_ctx.options.fps);
    curs_set(0);

    if (!has_colors()) {
        endwin();
        printf("Your terminal does not support color, quitting...\n");
        return 30;
    }

    start_color();
    use_default_colors();

    definitions_load(
        CW_DEFINITIONS_PATH,
        &ctx.core->item_defs,
        &ctx.core->entity_defs,
        &ctx.core->object_defs
    );
    options_load(&ctx.core->options, CW_OPTIONS_PATH);

    log_init(&ctx);

    struct timespec last_frame;
    clock_gettime(CLOCK_MONOTONIC, &last_frame);

    while (ctx.next_state != TUI_STATE_QUIT) {
        if (ctx.next_state != ctx.current_state) {
            if (ctx.current_screen)
                ctx.current_screen->deinit(&ctx);

            ctx.current_state = ctx.next_state;
            ctx.current_screen = CW_SCREENS[ctx.current_state];

            ctx.current_screen->init(&ctx);
        }

        ctx.ch = getch();
        ctx.frame_time = calculate_frame_time(&last_frame);

        if (ctx.ch == KEY_RESIZE) {
            erase();
            ctx.current_screen->resize(&ctx);
            log_resize(&ctx);
            refresh();
        } else {
            ctx.current_screen->input(&ctx);
        }

        ctx.current_screen->frame(&ctx);
        log_frame(&ctx);

        doupdate();
    }

    ctx.current_screen->deinit(&ctx);
    log_deinit(&ctx);
    endwin();

    log_print_all(stderr);
    log_free_all();
    da_free(core_ctx.item_defs);
    da_free(core_ctx.entity_defs);
    da_free(core_ctx.object_defs);

    return 0;
}

#define AIDE_IMPLEMENTATION
#include "aide.h"

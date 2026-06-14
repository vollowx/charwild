#ifndef TUI_CONTEXT_H
#define TUI_CONTEXT_H

#include "core/context.h"

typedef struct CwTui CwTui;

#define OVERLAY_MAP(X) X(log)

// X(state, name)
#define SCREEN_MAP(X)                                                          \
    X(MAIN_MENU, main_menu)                                                    \
    X(SAVES,     saves)                                                        \
    X(GAMEPLAY,  gameplay)                                                     \
    X(OPTIONS,   options)                                                      \
    X(ABOUT,     about)

typedef struct {
    void (*init)(CwTui *ctx);
    void (*deinit)(CwTui *ctx);
    void (*frame)(CwTui *ctx);
    void (*resize)(CwTui *ctx);
} CwTuiOverlay;

typedef struct {
    void (*init)(CwTui *ctx);
    void (*deinit)(CwTui *ctx);
    void (*frame)(CwTui *ctx);
    void (*resize)(CwTui *ctx);
    void (*input)(CwTui *ctx);
} CwTuiScreen;

#define DECLARE_VIEW_FUNCTIONS(name)                                           \
    void name##_init(CwTui *ctx);                                              \
    void name##_deinit(CwTui *ctx);                                            \
    void name##_frame(CwTui *ctx);                                             \
    void name##_resize(CwTui *ctx);                                            \

#define X(name)                                                                \
    DECLARE_VIEW_FUNCTIONS(name)                                               \
    extern CwTuiOverlay overlay_##name;
OVERLAY_MAP(X)
#undef X

#define X(state, name)                                                         \
    DECLARE_VIEW_FUNCTIONS(name)                                               \
    void name##_input(CwTui *ctx);                                             \
    extern CwTuiScreen screen_##name;
SCREEN_MAP(X)
#undef X

#undef DECLARE_VIEW_FUNCTIONS

static CwTuiScreen *const CW_SCREENS[] = {
#define X(state, name) &screen_##name,
    SCREEN_MAP(X)
#undef X
    NULL, // TUI_STATE_QUIT
};

typedef enum {
#define X(state, name) TUI_STATE_##state,
    SCREEN_MAP(X)
#undef X
    TUI_STATE_QUIT,
    __tui_state_count,
} CwTuiState;

// charwild TUI context
struct CwTui {
    int ch;
    double frame_time;
    CwTuiState current_state;
    CwTuiState next_state;
    CwTuiScreen *current_screen;
    Cw *core;
};

double calculate_frame_time(struct timespec *last_frame);

#endif // TUI_CONTEXT_H

#include "ui/tui_context.h"

#define X(name)                                                                \
    CwTuiOverlay overlay_##name = {name##_init, name##_deinit, name##_frame,   \
                                   name##_resize};
OVERLAY_MAP(X)
#undef X

#define X(state, name)                                                         \
    CwTuiScreen screen_##name = {name##_init, name##_deinit, name##_frame,     \
                                 name##_resize, name##_input};
SCREEN_MAP(X)
#undef X

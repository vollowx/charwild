#include <time.h>
#include "tui/tui_context.h"

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

double calculate_frame_time(struct timespec *last_frame)
{
    struct timespec current_frame;
    clock_gettime(CLOCK_MONOTONIC, &current_frame);

    double frame_time = (current_frame.tv_sec - last_frame->tv_sec) +
                        (current_frame.tv_nsec - last_frame->tv_nsec) / 1e9;

    *last_frame = current_frame;
    return frame_time;
}

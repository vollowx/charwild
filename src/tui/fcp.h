// Flexible color pairs
//
// Defines color pairs and cache the bg/fg combination for future usage.
// All functions should be called after `start_color()`.

#ifndef FCP_H
#define FCP_H

void fcp_reset(void);
short fcp_get(short fg, short bg);

#endif

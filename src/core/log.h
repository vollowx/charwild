#ifndef LOG_H
#define LOG_H

#include <ncurses.h>

#define LOG_MAX_LENGTH 128
#define LOG_UI_CAPACITY 8

#define info(fmt, ...)  log_add(CW_LOG_INFO,    fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  log_add(CW_LOG_WARNING, fmt, ##__VA_ARGS__)
#define error(fmt, ...) log_add(CW_LOG_ERROR,   fmt, ##__VA_ARGS__)

typedef enum {
    CW_LOG_INFO = 0,
    CW_LOG_WARNING,
    CW_LOG_ERROR,
} LogLevel;

typedef struct {
    LogLevel level;
    char *msg;
} Log;

typedef struct {
    Log *items;
    size_t capacity;
    size_t count;
} Logs;

void log_add(LogLevel level, const char *fmt, ...);
void log_free_all(void);

extern Logs logs;

#endif

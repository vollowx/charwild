#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "core/common.h"
#include "core/log.h"

Logs logs = {0};

static const char* log_labels[3] = {
    "info",
    "warn",
    " err",
};

void log_add(LogLevel level, const char *fmt, ...)
{
    char msg[LOG_MAX_LENGTH + 1];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, LOG_MAX_LENGTH, fmt, args);
    va_end(args);

    Log log = {0};
    log.level = level;
    log.msg = strdup(msg);

    da_append(&logs, log);
}

void log_print_all(FILE *stream)
{
    da_foreach(Log, it, &logs)
        if (it->level > CW_LOG_INFO)
            fprintf(stream, "%s: %s\n", log_labels[it->level], it->msg);
}

void log_free_all(void)
{
    da_foreach(Log, it, &logs)
        free(it->msg);
    da_free(logs);
    logs.count = 0;
    logs.capacity = 0;
}

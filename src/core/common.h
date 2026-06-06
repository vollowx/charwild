#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aide.h"
#include "core/log.h"

#define CW_VERSION_MAJOR 0
#define CW_VERSION_MINOR 0
#define CW_VERSION_PATCH 0

#define CW_DEFINITIONS_PATH "definitions.txt"
#define CW_OPTIONS_PATH     "options.txt"
#define CW_SAVES_PATH       "save%d.txt"

// World ticks per second
#define CW_TPS 20.0

typedef enum {
    CW_LINE_SECTION, // [name]         ->  tag = "name"
    CW_LINE_KV,      // key = value    ->  tag = key,  val = trimmed value
    CW_LINE_STRUCT,  // key : a b ...  ->  tag = key,  val = trimmed args
    CW_LINE_APPEND,  // key + a b ...  ->  tag = key,  val = trimmed args
} CwLineKind;

typedef struct {
    CwLineKind kind;
    Sv         tag; // key / section name — points into caller's buf
    Sv         val; // value / args       — points into caller's buf
} CwLine;

// Reads the next meaningful line from fp into buf and parses it into *out.
// Returns false on EOF.
static inline bool cw_next_line(FILE *fp, char *buf, size_t bufsz, CwLine *out) {
    while (fgets(buf, (int)bufsz, fp)) {
        // Strip // comment
        char *comment = strstr(buf, "//");
        if (comment)
            *comment = '\0';

        // Trim leading whitespace
        char *s = buf;
        while (*s == ' ' || *s == '\t')
            ++s;

        // Trim trailing whitespace
        size_t len = strlen(s);
        while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                           s[len - 1] == '\n' || s[len - 1] == '\r'))
            s[--len] = '\0';

        if (*s == '\0')
            continue; // blank or comment-only

        // [section]
        if (s[0] == '[') {
            char *close = strchr(s, ']');
            if (!close)
                continue;
            *close    = '\0';
            out->tag  = (Sv){s + 1, (size_t)(close - s - 1)};
            out->val  = (Sv){close + 1, 0};
            out->kind = CW_LINE_SECTION;
            return true;
        }

        // Scan key: stop at operator (= : +) or whitespace
        char *key_start = s;
        char *p = s;
        while (*p && *p != '=' && *p != ':' && *p != '+' && *p != ' ' && *p != '\t')
            ++p;
        size_t key_len = (size_t)(p - key_start);

        // Skip whitespace between key and operator
        while (*p == ' ' || *p == '\t')
            ++p;

        char op = *p;
        if (op != '=' && op != ':' && op != '+')
            continue; // unrecognized line, skip

        // Key as a view into buf
        out->tag = (Sv){key_start, key_len};

        // Val: skip operator and leading whitespace
        char *v = p + 1;
        while (*v == ' ' || *v == '\t')
            ++v;
        out->val = (Sv){v, strlen(v)};

        if      (op == '=') out->kind = CW_LINE_KV;
        else if (op == ':') out->kind = CW_LINE_STRUCT;
        else                out->kind = CW_LINE_APPEND;
        return true;
    }
    return false;
}

#define draw_win_frame(win, title, color)                                      \
    do {                                                                       \
        wattron(win, COLOR_PAIR(fcp_get(color, -1)));                          \
        box(win, 0, 0);                                                        \
        wattron(win, A_BOLD);                                                  \
        mvwprintw(win, 0, 3, " %s ", title);                                   \
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

#endif

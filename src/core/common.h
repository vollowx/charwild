#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/log.h"

#define CW_VERSION_MAJOR 0
#define CW_VERSION_MINOR 0
#define CW_VERSION_PATCH 0

#define CW_DEFINITIONS_PATH "definitions.txt"
#define CW_OPTIONS_PATH     "options.txt"
#define CW_SAVES_PATH       "save%d.txt"

// World ticks per second
#define CW_TPS 20.0

extern Logs logs;

typedef struct {
    const char *ptr;
    size_t      len;
} Sv;

static inline Sv   sv_from_cstr(const char *s)         { return (Sv){s, strlen(s)}; }
static inline bool sv_eq(Sv a, Sv b)                   { return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0; }
static inline bool sv_eq_cstr(Sv a, const char *b)     { return strncmp(a.ptr, b, a.len) == 0 && b[a.len] == '\0'; }
static inline int  sv_to_int(Sv s)                     { return (int)strtol(s.ptr, NULL, 10); }
static inline long sv_to_long(Sv s)                    { return strtol(s.ptr, NULL, 10); }
static inline unsigned long sv_to_ulong(Sv s)          { return strtoul(s.ptr, NULL, 10); }

// Copy at most n-1 bytes of s into dst and null-terminate.
static inline void sv_to_buf(Sv s, char *dst, size_t n) {
    if (n == 0) return;
    size_t copy = s.len < n - 1 ? s.len : n - 1;
    memcpy(dst, s.ptr, copy);
    dst[copy] = '\0';
}

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

#define do_defer_and_return(value)                                             \
    do {                                                                       \
        ret = (value);                                                         \
        goto defer;                                                            \
    } while (0)

#define UNUSED (void)

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
        if (win) {                                                             \
            delwin(win);                                                       \
            win = NULL;                                                        \
        }                                                                      \
    } while (0)

#define NOB_REALLOC realloc
#define NOB_FREE free
#define NOB_ASSERT assert
#define NOB_DA_INIT_CAP 128
#define NOB_DECLTYPE_CAST(x)

#define nob_da_reserve(da, expected_capacity)                                  \
    do {                                                                       \
        if ((expected_capacity) > (da)->capacity) {                            \
            if ((da)->capacity == 0) {                                         \
                (da)->capacity = NOB_DA_INIT_CAP;                              \
            }                                                                  \
            while ((expected_capacity) > (da)->capacity) {                     \
                (da)->capacity *= 2;                                           \
            }                                                                  \
            (da)->items = NOB_DECLTYPE_CAST((da)->items) NOB_REALLOC(          \
                (da)->items, (da)->capacity * sizeof(*(da)->items));           \
            NOB_ASSERT((da)->items != NULL && "Buy more RAM lol");             \
        }                                                                      \
    } while (0)

// Append an item to a dynamic array
#define nob_da_append(da, item)                                                \
    do {                                                                       \
        nob_da_reserve((da), (da)->count + 1);                                 \
        (da)->items[(da)->count++] = (item);                                   \
    } while (0)

#define nob_da_free(da) NOB_FREE((da).items)

// Append several items to a dynamic array
#define nob_da_append_many(da, new_items, new_items_count)                     \
    do {                                                                       \
        nob_da_reserve((da), (da)->count + (new_items_count));                 \
        memcpy((da)->items + (da)->count, (new_items),                         \
               (new_items_count) * sizeof(*(da)->items));                      \
        (da)->count += (new_items_count);                                      \
    } while (0)

#define nob_da_resize(da, new_size)                                            \
    do {                                                                       \
        nob_da_reserve((da), new_size);                                        \
        (da)->count = (new_size);                                              \
    } while (0)

#define nob_da_last(da)                                                        \
    (da)->items[(NOB_ASSERT((da)->count > 0), (da)->count - 1)]
#define nob_da_remove_unordered(da, i)                                         \
    do {                                                                       \
        size_t j = (i);                                                        \
        NOB_ASSERT(j < (da)->count);                                           \
        (da)->items[j] = (da)->items[--(da)->count];                           \
    } while (0)

#define nob_da_foreach(Type, it, da)                                           \
    for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

#define da_append nob_da_append
#define da_free nob_da_free
#define da_append_many nob_da_append_many
#define da_resize nob_da_resize
#define da_reserve nob_da_reserve
#define da_last nob_da_last
#define da_remove_unordered nob_da_remove_unordered
#define da_foreach nob_da_foreach

#endif

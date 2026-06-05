/*
 * Dynamic array macros are from nob.h (https://github.com/tsoding/nob.h)
 * Author: tsoding (https://github.com/tsoding)
 * License: MIT
 *
 * Copyright (c) 2024 Alexey Kutepov
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions: The above copyright
 * notice and this permission notice shall be included in all copies or
 * substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HELPERS_H
#define HELPERS_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "core/log.h"
#include "core/options.h"
#include "ui/fcp.h"

#define CW_DEFINITIONS_PATH "definitions.txt"
#define CW_OPTIONS_PATH     "options.txt"
#define CW_SAVES_PATH       "save%d.txt"
#define CW_FPS 60
#define CW_VERSION_MAJOR 0
#define CW_VERSION_MINOR 0
#define CW_VERSION_PATCH 0

extern Logs logs;

static inline char *cw_trim(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        ++s;

    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                       s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }

    return s;
}

static inline int cw_is_ignored_line(const char *s) {
    return s[0] == '\0' || (s[0] == '/' && s[1] == '/');
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

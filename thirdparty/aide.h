#ifndef AIDE_H
#define AIDE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED (void)

#define do_defer_and_return(value)                                             \
    do {                                                                       \
        ret = (value);                                                         \
        goto defer;                                                            \
    } while (0)

#define NOB_REALLOC realloc
#define NOB_FREE free
#define NOB_ASSERT assert
#define NOB_DA_INIT_CAP 128
#define NOB_DECLTYPE_CAST(x)

#define da_reserve(da, expected_capacity)                                      \
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
#define da_append(da, item)                                                    \
    do {                                                                       \
        da_reserve((da), (da)->count + 1);                                     \
        (da)->items[(da)->count++] = (item);                                   \
    } while (0)

#define da_free(da) NOB_FREE((da).items)

// Append several items to a dynamic array
#define da_append_many(da, new_items, new_items_count)                         \
    do {                                                                       \
        da_reserve((da), (da)->count + (new_items_count));                     \
        memcpy((da)->items + (da)->count, (new_items),                         \
               (new_items_count) * sizeof(*(da)->items));                      \
        (da)->count += (new_items_count);                                      \
    } while (0)

#define da_resize(da, new_size)                                                \
    do {                                                                       \
        da_reserve((da), new_size);                                            \
        (da)->count = (new_size);                                              \
    } while (0)

#define da_last(da)                                                            \
    (da)->items[(NOB_ASSERT((da)->count > 0), (da)->count - 1)]
#define da_remove_unordered(da, i)                                             \
    do {                                                                       \
        size_t j = (i);                                                        \
        NOB_ASSERT(j < (da)->count);                                           \
        (da)->items[j] = (da)->items[--(da)->count];                           \
    } while (0)

#define da_foreach(Type, it, da)                                               \
    for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

typedef struct {
    const char *ptr;
    size_t len;
} Sv;

static inline Sv sv(const char *cstr) { return (Sv){ cstr, strlen(cstr) }; }
static inline void sv_to_cstr(Sv s, char *dst, size_t n) {
    if (n == 0) return;
    size_t copy = s.len < n - 1 ? s.len : n - 1;
    memcpy(dst, s.ptr, copy);
    dst[copy] = '\0';
}

static inline bool sv_eq(Sv a, Sv b) {
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}
static inline bool sv_eq_cstr(Sv a, const char *b) {
    return strncmp(a.ptr, b, a.len) == 0 && b[a.len] == '\0';
}

static inline int  sv_to_int(Sv s) {
    return (int)strtol(s.ptr, NULL, 10);
}
static inline long sv_to_long(Sv s) {
    return strtol(s.ptr, NULL, 10);
}
static inline unsigned long sv_to_ulong(Sv s) {
    return strtoul(s.ptr, NULL, 10);
}

#endif // AIDE_H

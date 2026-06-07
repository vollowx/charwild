// aide - Public Domain
// Helpers including dynamic array and string operations.

#ifndef AIDE_H
#define AIDE_H

#include <stdbool.h>
#include <string.h>

#ifndef AIDEDEF
#define AIDEDEF
#endif

#ifndef AIDE_ASSERT
#include "assert.h"
#define AIDE_ASSERT assert
#endif

#ifndef AIDE_REALLOC
#include <stdlib.h>
#define AIDE_REALLOC realloc
#endif

#ifndef AIDE_FREE
#include <stdlib.h>
#define AIDE_FREE free
#endif

#ifndef AIDE_DA_INIT_CAP
#define AIDE_DA_INIT_CAP 256
#endif

#ifdef __cplusplus
#define AIDE_DECLTYPE_CAST(T) (decltype(T))
#else
#define AIDE_DECLTYPE_CAST(T)
#endif // __cplusplus

#define UNUSED (void)

#define do_defer_and_return(value)                                             \
    do {                                                                       \
        ret = (value);                                                         \
        goto defer;                                                            \
    } while (0)

#define da_reserve(da, expected_capacity)                                      \
    do {                                                                       \
        if ((expected_capacity) > (da)->capacity) {                            \
            if ((da)->capacity == 0) {                                         \
                (da)->capacity = AIDE_DA_INIT_CAP;                             \
            }                                                                  \
            while ((expected_capacity) > (da)->capacity) {                     \
                (da)->capacity *= 2;                                           \
            }                                                                  \
            (da)->items = AIDE_DECLTYPE_CAST((da)->items) AIDE_REALLOC(        \
                (da)->items, (da)->capacity * sizeof(*(da)->items));           \
            AIDE_ASSERT((da)->items != NULL && "da_reserve: out of memory");   \
        }                                                                      \
    } while (0)

// Append an item to a dynamic array
#define da_append(da, item)                                                    \
    do {                                                                       \
        da_reserve((da), (da)->count + 1);                                     \
        (da)->items[(da)->count++] = (item);                                   \
    } while (0)

#define da_free(da) AIDE_FREE((da).items)

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
    (da)->items[(AIDE_ASSERT((da)->count > 0), (da)->count - 1)]
#define da_remove_unordered(da, i)                                             \
    do {                                                                       \
        size_t j = (i);                                                        \
        AIDE_ASSERT(j < (da)->count);                                          \
        (da)->items[j] = (da)->items[--(da)->count];                           \
    } while (0)

#define da_foreach(Type, it, da)                                               \
    for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

typedef struct {
    const char *ptr;
    size_t len;
} Sv;

AIDEDEF Sv sv(const char *cstr);
AIDEDEF void sv_to_cstr(Sv s, char *dst, size_t n);

AIDEDEF bool sv_eq(Sv a, Sv b);
AIDEDEF bool sv_eq_cstr(Sv a, const char *cstr);

AIDEDEF Sv sv_from_data(const char *ptr, size_t len);
AIDEDEF Sv sv_trim_left(Sv);
AIDEDEF Sv sv_trim_right(Sv);
AIDEDEF Sv sv_trim(Sv);

AIDEDEF int sv_to_int(Sv);
AIDEDEF long sv_to_long(Sv);
AIDEDEF unsigned long sv_to_ulong(Sv);

#endif // AIDE_H

#ifdef AIDE_IMPLEMENTATION

#include <ctype.h>

Sv sv(const char *cstr)
{
    return (Sv){ cstr, strlen(cstr) };
}
void sv_to_cstr(Sv s, char *dst, size_t n)
{
    if (n == 0) return;
    size_t copy = s.len < n - 1 ? s.len : n - 1;
    memcpy(dst, s.ptr, copy);
    dst[copy] = '\0';
}

bool sv_eq(Sv a, Sv b)
{
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}
bool sv_eq_cstr(Sv a, const char *b)
{
    return strncmp(a.ptr, b, a.len) == 0 && b[a.len] == '\0';
}

Sv sv_from_data(const char *ptr, size_t len)
{
    Sv sv;
    sv.ptr = ptr;
    sv.len = len;
    return sv;
}

Sv sv_trim_left(Sv sv)
{
    size_t i = 0;
    while (i < sv.len && isspace(sv.ptr[i])) {
        i += 1;
    }

    return sv_from_data(sv.ptr + i, sv.len - i);
}

Sv sv_trim_right(Sv sv)
{
    size_t i = 0;
    while (i < sv.len && isspace(sv.ptr[sv.len - 1 - i])) {
        i += 1;
    }

    return sv_from_data(sv.ptr, sv.len - i);
}

Sv sv_trim(Sv sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

int sv_to_int(Sv sv)
{
    return (int)strtol(sv.ptr, NULL, 10);
}
long sv_to_long(Sv sv)
{
    return strtol(sv.ptr, NULL, 10);
}
unsigned long sv_to_ulong(Sv sv)
{
    return strtoul(sv.ptr, NULL, 10);
}

#endif // AIDE_IMPLEMENTATION

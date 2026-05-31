#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdlib.h>

#include "core/options.h"
#include "core/world_defs.h"

// charwild context
typedef struct {
    CwOptions options;

    size_t cur_slot;

    ItemDefs item_defs;
    EntityDefs entity_defs;
    ObjectDefs object_defs;
} Cw;

#endif

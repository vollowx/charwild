#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdlib.h>

#include "core/options.h"
#include "core/world_defs.h"

// charwild context
typedef struct {
    size_t cur_slot;

    CwOptions *options;

    ItemDefs item_defs;
    EntityDefs entity_defs;
    ObjectDefs object_defs;
} Cw;

#endif

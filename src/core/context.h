#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdlib.h>
#include "core/options.h"
#include "core/definitions.h"
#include "core/world.h"

// charwild context
struct Cw {
    CwOptions options;

    size_t current_slot;
    World current_world;

    ItemDefs item_defs;
    EntityDefs entity_defs;
    ObjectDefs object_defs;
};

#endif

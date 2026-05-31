#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdlib.h>

#include "core/options.h"
#include "core/save.h"
#include "core/world.h"
#include "core/world_defs.h"

// charwild context
struct Cw {
    CwOptions options;

    size_t current_slot;
    World current_world;
    Save current_save;

    ItemDefs item_defs;
    EntityDefs entity_defs;
    ObjectDefs object_defs;
};

#endif

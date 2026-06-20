// World definitions loading and querying

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <stdint.h>
#include <ncurses.h>

// Definition arrays have a maximum size of `uint16_t`, that is limited by the
// id of a definition.

typedef enum {
    ITEM_RESOURCE,   // Ore, wood
    ITEM_PLACEABLE,  // Seeds, furniture
    ITEM_CONSUMABLE, // Food, potions
    ITEM_EQUIPMENT,  // Tools, armor
} ItemKind;

typedef enum {
    ENTITY_PLAYER,
    ENTITY_NPC,
    ENTITY_ENEMY,
    ENTITY_ANIMAL,
    ENTITY_ITEM,
} EntityKind;

typedef struct {
    uint16_t id;
    ItemKind kind;
    char name[32];
    union {
        bool stackable;
        int  stack_max;
    };

    char symbol[2];
    short fg, bg;
    attr_t attr;
} ItemDef;

typedef struct {
    uint16_t id;
    EntityKind kind;
    char name[32];
    int max_health;
    bool is_passable;

    char symbol[2];
    short fg, bg;
    attr_t attr;
} EntityDef;

typedef struct {
    uint16_t id;
    char name[32];
    int max_health;
    bool is_passable;

    char symbol[2];
    short fg, bg;
    attr_t attr;
} ObjectDef;

typedef struct {
    ItemDef *items;
    size_t count;
    size_t capacity;
} ItemDefs;

typedef struct {
    EntityDef *items;
    size_t count;
    size_t capacity;
} EntityDefs;

typedef struct {
    ObjectDef *items;
    size_t count;
    size_t capacity;
} ObjectDefs;

bool definitions_load(
    const char *path,
    ItemDefs   *item_defs,
    EntityDefs *entity_defs,
    ObjectDefs *object_defs
);

ItemDef     *item_def_lookup(  ItemDefs defs, uint16_t id);
EntityDef *entity_def_lookup(EntityDefs defs, uint16_t id);
ObjectDef *object_def_lookup(ObjectDefs defs, uint16_t id);

#endif // DEFINITIONS_H

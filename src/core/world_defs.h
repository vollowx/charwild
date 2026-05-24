#ifndef WORLD_DEFS_H
#define WORLD_DEFS_H

#include <stdint.h>

#include <ncurses.h>

typedef enum {
    ITEM_RESOURCE,   // Ore, wood
    ITEM_PLACEABLE,  // Seeds, furniture
    ITEM_CONSUMABLE, // Food, potions
    ITEM_EQUIPMENT,  // Tools, armor
} ItemType;

typedef enum {
    ENTITY_PLAYER,
    ENTITY_NPC,
    ENTITY_ENEMY,
    ENTITY_ANIMAL,
    ENTITY_ITEM,
} EntityType;

typedef struct {
    uint16_t id;
    ItemType type;
    char name[32];
    int max_stack;

    char symbol[2];
    short fg, bg;
    attr_t attr;
} ItemDef;

typedef struct {
    uint16_t id;
    EntityType type;
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

ObjectDef *object_def_lookup(ObjectDefs defs, uint16_t id);

#endif // WORLD_DEFS_H

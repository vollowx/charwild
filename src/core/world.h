#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ncurses.h>

#include "core/context.h"
#include "world_defs.h"

// Order:
// 1. item
// 2. entity
// 3. object
// 4. map
// 5. game

typedef enum {
    ELEV_NONE = 0,
    ELEV_DEEP_WATER,
    ELEV_WATER,
    ELEV_GROUND,
    ELEV_HILL,
    ELEV_MOUNTAIN,
    _elevation_count,
} Elevation;

typedef struct {
    ItemDef *def;
    int quantity;
    int durability; // Only used if def->type == ITEM_EQUIPMENT
} ItemStack;

typedef struct {
    ItemStack *items;
    size_t count;
    size_t capacity;
} ItemStacks;

typedef struct {
    EntityDef *def;
    char name[32];
    int health;
    size_t x, y;

    ItemStacks inventory;
} Entity;

typedef struct {
    Entity *items;
    size_t count;
    size_t capacity;
} Entities;

typedef struct {
    Elevation elevation;
    uint16_t object_id;
    int object_health;
    Entity *entity;
} MapCell;

typedef struct {
    size_t w, h;
    MapCell **cells;
} Map;

typedef struct {
    uint32_t seed;

    Map *map;
    Entity *player;
    Entities entities;
} World;

bool entity_move(Entity *e, Map *map, int dx, int dy);
bool entity_place_object(Entity *e, Map *map, uint16_t object_id, int dx,
                         int dy);

Map *new_map(size_t height, size_t width);
void free_map(Map *map);

void free_world(World *);
void world_init(World *, Cw *ctx);
void world_gen_area(World *, size_t y1, size_t x1, size_t y2, size_t x2, Cw *ctx);
bool world_tick(World *, double dt);
bool world_tick_animals(World *, double dt);
bool world_tick_world(World *, double dt);

#endif

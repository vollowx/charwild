#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <ncurses.h>

#include "core/definitions.h"

typedef struct Cw Cw;

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
    uint64_t x, y;

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
} Cell;

typedef struct {
    uint64_t w, h;
    Cell *cells;
} Map;

typedef struct {
    uint32_t seed;
    uint32_t timestamp;

    Map *map;
    Entity *player;
    Entities entities;
} World;

// `dy` for delta x, so as `dx`
bool entity_move(Entity *e, Map *map, int dx, int dy);
bool entity_place_object(Entity *e, Map *map, uint16_t object_id, int dx,
                         int dy);

Map *map_alloc(size_t height, size_t width);
void map_free(Map *map);
Cell *cell_ref(Map *map, uint64_t y, uint64_t x);

void world_free(World *);
void world_init(World *, Cw *ctx);
void world_gen_area(World *, size_t y1, size_t x1, size_t y2, size_t x2, Cw *ctx);
// Returns the player or `NULL` when not found
Entity *world_link_entities_to_cells(World *w, size_t start);
bool world_tick(World *, double dt);

#endif

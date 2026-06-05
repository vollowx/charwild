#include <assert.h>
#include <time.h>

#include <ncurses.h>
#include <simplexnoise1234.h>

#include "core/context.h"
#include "core/common.h"
#include "core/world.h"

bool entity_move(Entity *e, Map *map, int dx, int dy) {
    if (!e || !map)
        return false;

    if (dx < 0 && e->x < (size_t)abs(dx))
        return false;
    if (dy < 0 && e->y < (size_t)abs(dy))
        return false;

    size_t new_x = e->x + dx;
    size_t new_y = e->y + dy;

    if (new_x >= map->w || new_y >= map->h)
        return false;

    MapCell *target = &map->cells[new_y][new_x];

    if (target->object_id)
        return false;
    if (target->entity != NULL)
        return false;
    if (target->elevation == ELEV_DEEP_WATER || target->elevation == ELEV_WATER)
        return false;

    map->cells[e->y][e->x].entity = NULL;

    e->x = new_x;
    e->y = new_y;

    target->entity = e;

    return true;
}

bool entity_place_object(Entity *e, Map *map, uint16_t object_id, int dx,
                         int dy) {
    assert(e && map && map->cells);

    int tx = (int)e->x + dx;
    int ty = (int)e->y + dy;

    if (tx < 0 || ty < 0 || (size_t)tx >= map->w || (size_t)ty >= map->h) {
        error("entity_place_object: Target (%d, %d) out of bounds", tx, ty);
        return false;
    }

    MapCell *target_cell = &map->cells[ty][tx];

    if (target_cell->object_id != 0 && object_id != 0)
        return false;
    if (target_cell->entity != NULL)
        return false;

    target_cell->object_id = object_id;

    return true;
}

Map *map_alloc(size_t height, size_t width) {
    // 1. Allocate the Map container
    Map *map = malloc(sizeof(Map));
    if (!map)
        return NULL;

    map->w = width;
    map->h = height;

    // 2. Allocate the array of row pointers
    map->cells = malloc(sizeof(MapCell *) * height);
    if (!map->cells) {
        free(map);
        return NULL;
    }

    // 3. Allocate each row
    for (size_t y = 0; y < height; ++y) {
        // Use calloc to automatically zero out all members (Elevation 0, NULL
        // pointers)
        map->cells[y] = calloc(width, sizeof(MapCell));

        if (!map->cells[y]) {
            // Cleanup previous rows on failure
            for (size_t i = 0; i < y; ++i)
                free(map->cells[i]);
            free(map->cells);
            free(map);
            return NULL;
        }
    }

    return map;
}

void map_free(Map *map) {
    if (!map)
        return;

    for (size_t y = 0; y < map->h; ++y) {
        free(map->cells[y]);
    }
    free(map->cells);
    free(map);
}

// TASK(20260223-173936): Chunk-ize game both in struct and file
void world_init(World *w, Cw *ctx) {
    w->map = map_alloc(2048, 2048);

    w->entities.items = NULL;
    w->entities.count = 0;
    w->entities.capacity = 0;

    Entity player = {
        .def = &ctx->entity_defs.items[0],
        .health = 100,
    };
    strncpy(player.name, "Guy", sizeof(player.name) - 1);

    player.inventory.count = 0;
    player.inventory.capacity = 8;
    player.inventory.items =
        malloc(sizeof(ItemStack) * player.inventory.capacity);

    da_append(&w->entities, player);

    srand((unsigned int)time(NULL));
    w->seed = ((uint32_t)rand() << 16) | (uint32_t)rand();

    size_t py = 1024, px = 1024, y_offset = 0;
    world_gen_area(w, py - 256, px - 256, py + 256, px + 256, ctx);

    w->player = &w->entities.items[0];

    // TODO: Fails in a rather small possibility
    while (w->map->cells[py + y_offset][px].elevation != ELEV_GROUND &&
           y_offset < 256) {
        ++y_offset;
    }
    py += y_offset;

    w->player->y = py;
    w->player->x = px;
    w->map->cells[w->player->y][w->player->x].entity = w->player;
}

void world_free(World *w) {
    assert(w != NULL);

    map_free(w->map);
    da_foreach(Entity, it, &w->entities)
        free(it->inventory.items);
    da_free(w->entities);
}

void world_gen_area(World *w, size_t y1, size_t x1, size_t y2, size_t x2, Cw *ctx) {
    assert(w && w->map);

    Map *map = w->map;

    float seed_ox = (float)(w->seed % 100000);
    float seed_oy = (float)((w->seed / 100) % 100000);

    size_t pre_count = w->entities.count;

    for (size_t y = y1; y < y2 && y < map->h; y++) {
        for (size_t x = x1; x < x2 && x < map->w; x++) {
            MapCell *cell = &map->cells[y][x];
            if (cell->elevation != ELEV_NONE) continue;

            float scale = 0.07f;
            float raw_noise = snoise2((float)x * scale + seed_ox,
                                      (float)y * scale + seed_oy);
            float val = (raw_noise + 1.0f) * 0.5f;

            if (val > 0.9f) {
                cell->elevation = ELEV_MOUNTAIN;
            } else if (val > 0.85f) {
                cell->elevation = ELEV_HILL;
            } else if (val > 0.4f) {
                cell->elevation = ELEV_GROUND;
            } else if (val > 0.3f) {
                cell->elevation = ELEV_WATER;
            } else {
                cell->elevation = ELEV_DEEP_WATER;
            }

            if (cell->elevation != ELEV_GROUND &&
                cell->elevation != ELEV_HILL) continue;

            uint32_t res_h = w->seed ^ ((uint32_t)x * 123456789U) ^
                             ((uint32_t)y * 987654321U);

            uint32_t spawn_threshold =
                (cell->elevation == ELEV_HILL) ? 7U : 4U;

            if ((res_h % 100) < spawn_threshold) {
                Entity animal = {
                    .x = x,
                    .y = y,
                };

                int pick = (res_h >> 8) % 3 + 2;
                animal.def = &ctx->entity_defs.items[pick];
                strncpy(animal.name, ctx->entity_defs.items[pick].name,
                        sizeof(animal.name) - 1);

                da_append(&w->entities, animal);
            }
        }
    }

    for (size_t i = pre_count; i < w->entities.count; ++i) {
        Entity *e = &w->entities.items[i];
        map->cells[e->y][e->x].entity = e;
    }
}

bool world_tick(World *w, double dt) {
    bool updated = false;

    // What a typhoon
    // da_foreach(Entity *, e, &game->entities) {
    //   if (entity_move(*e, game->map, 1, 0)) {
    //     updated = true;
    //   }
    // }

    return updated;
}

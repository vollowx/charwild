#include <assert.h>
#include <time.h>
#include <ncurses.h>
#include <simplexnoise1234.h>
#include "core/context.h"
#include "core/world.h"
#include "core/common.h"

Item *entity_inventory_lookup(Entity *e, uint16_t def_id)
{
    assert(e && e->inventory.items);
    da_foreach(Item, item, &e->inventory)
        if (item->def && item->def->id == def_id)
            return item;

    return NULL;
}

bool entity_move(Entity *e, Map *m, int dx, int dy)
{
    assert(e && m);

    if (dx < 0 && e->x < (size_t)abs(dx))
        return false;
    if (dy < 0 && e->y < (size_t)abs(dy))
        return false;

    size_t dest_x = e->x + dx, dest_y = e->y + dy;

    if (dest_x >= m->w || dest_y >= m->h)
        return false;

    Cell *dest = cell_ref(m, dest_y, dest_x);

    // 10000 is special, standing for the Floating Bridge
    if (dest->object_id > 10000)
        return false;
    if (dest->elevation <= ELEV_WATER && !(dest->object_id == 10000))
        return false;

    if (dest->entity != NULL){
        // Chance to push it away :)
        if ((rand() % 100) < 3) {
            if (!entity_move(dest->entity, m, dx, dy))
                return false;
        } else return false;
    }

    cell_ref(m, e->y, e->x)->entity = NULL;
    e->x = dest_x;
    e->y = dest_y;
    dest->entity = e;

    return true;
}

bool entity_place_object(Entity *e, Map *m, uint16_t def_id, int dx, int dy)
{
    assert(e && m && m->cells);

    int tx = (int)e->x + dx;
    int ty = (int)e->y + dy;
    if (tx < 0 || ty < 0 || (size_t)tx >= m->w || (size_t)ty >= m->h) {
        return false;
    }

    Cell *target_cell = cell_ref(m, ty, tx);
    if (target_cell->object_id != 0 && def_id != 0)
        return false;
    if (target_cell->entity != NULL)
        return false;

    Item *item = entity_inventory_lookup(e, def_id);
    if (!item || item->stack <= 0)
        return false;
    target_cell->object_id = def_id;
    item->stack -= 1;
    // TODO: bool entity_remove_item(Entity *e, uint16_t def_id, int n)

    return true;
}

void entity_acquire_item(Entity *e, uint16_t def_id, int stack, Cw *ctx)
{
    assert(e);
    Item item = {
        .def = item_def_lookup(ctx->item_defs, def_id),
        .stack = stack,
        0,
    };
    da_append(&e->inventory, item);
}

Map *map_alloc(size_t height, size_t width)
{
    Map *map = malloc(sizeof(Map));
    if (!map)
        return NULL;

    map->w = width;
    map->h = height;

    map->cells = malloc(sizeof(Cell) * width * height);
    if (!map->cells) {
        free(map);
        return NULL;
    }

    return map;
}

void map_free(Map *map)
{
    assert(map && map->cells);

    free(map->cells);
    free(map);
}

Cell *cell_ref(Map *map, uint64_t y, uint64_t x)
{
    assert(y < map->h && x < map->w);
    return &map->cells[y * map->w + x];
}

// TASK(20260223-173936): Chunk-ize game both in struct and file
void world_init(World *w, Cw *ctx)
{
    w->map = map_alloc(512, 512);

    w->entities.items = NULL;
    w->entities.count = 0;
    w->entities.capacity = 0;

    Entity player = {
        .def = &ctx->entity_defs.items[0],
        .health = 100,
    };
    strncpy(player.name, "Guy", sizeof(player.name) - 1);

    player.inventory.count = 0;
    player.inventory.capacity = 32;
    player.inventory.items = malloc(sizeof(Item) * player.inventory.capacity);

    da_append(&w->entities, player);

    srand((unsigned int)time(NULL));
    w->seed = ((uint32_t)rand() << 16) | (uint32_t)rand();

    size_t py = 256, px = 256, y_offset = 0;
    world_gen_area(w, py - 256, px - 256, py + 256, px + 256, ctx);

    w->player = &w->entities.items[0];

    // TODO: Fails in a rather small possibility
    while (cell_ref(w->map, py + y_offset, px)->elevation != ELEV_GROUND &&
           y_offset < 256) {
        ++y_offset;
    }
    py += y_offset;

    w->player->y = py;
    w->player->x = px;
    cell_ref(w->map, w->player->y, w->player->x)->entity = w->player;
}

void world_free(World *w)
{
    assert(w != NULL);

    map_free(w->map);
    da_foreach(Entity, it, &w->entities)
        free(it->inventory.items);
    da_free(w->entities);
}

void world_gen_area(World *w, size_t y1, size_t x1, size_t y2, size_t x2, Cw *ctx)
{
    assert(w && w->map);

    float scale = 0.07f;
    float seed_bias_x = (float)(w->seed % 100000);
    float seed_bias_y = (float)((w->seed / 100) % 100000);

    size_t original_entity_count = w->entities.count;

    for (size_t y = y1; y < y2 && y < w->map->h; y++) {
        for (size_t x = x1; x < x2 && x < w->map->w; x++) {
            Cell *cell = cell_ref(w->map, y, x);
            if (cell->elevation != ELEV_NONE) continue;

            float noise = snoise2((float)x * scale + seed_bias_x,
                                  (float)y * scale + seed_bias_y);

            if (noise > 0.9f) {
                cell->elevation = ELEV_MOUNTAIN;
            } else if (noise > 0.8f) {
                cell->elevation = ELEV_HILL;
            } else if (noise > -0.1f) {
                cell->elevation = ELEV_GROUND;
            } else if (noise > -0.5f) {
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

                int pick = (res_h >> 8) % 3 + 1;
                animal.def = &ctx->entity_defs.items[pick];
                // TODO: Reconsider entity naming strategy, currently none mean
                //       the default

                da_append(&w->entities, animal);
            }
        }
    }

    world_link_entities_to_cells(w, original_entity_count);
}

Entity *world_link_entities_to_cells(World *w, size_t start)
{
    Entity *player = NULL;
    for (size_t i = start; i < w->entities.count; ++i) {
        Entity *entity = &w->entities.items[i];
        cell_ref(w->map, entity->y, entity->x)->entity = entity;
        if (entity->def->type == ENTITY_PLAYER)
            player = entity;
    }
    return player;
}

bool world_tick(World *w, double dt)
{
    bool updated = false;

    // TODO: dedicated behavior functions for each kind of animal
    da_foreach(Entity, ent, &w->entities) {
        if (ent->def->type != ENTITY_ANIMAL)
            continue;

        if ((rand() % 100) < 3) {
            int dx = (rand() % 3) - 1, dy = (rand() % 3) - 1;
            if (dx == 0 && dy == 0)
                continue;

            bool moved = entity_move(ent, w->map, dx, dy);
            updated = moved || updated;
        }
    }

    return updated;
}

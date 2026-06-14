#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/save.h"
#include "core/common.h"
#include "core/context.h"
#include "core/log.h"
#include "core/definitions.h"
#include "core/world.h"

static char save_path[128];

static const char *get_save_path(int slot)
{
    snprintf(save_path, sizeof(save_path), CW_SAVES_PATH, slot);
    return save_path;
}

SaveResult world_save(const World *w, int slot)
{
    info("[save] saving slot %d", slot);

    assert(w && w->map && w->player);

    SaveResult ret = SAVE_OK;
    FILE *fp = fopen(get_save_path(slot), "w");
    if (!fp)
        do_defer_and_return(SAVE_ERR_OPEN);

    fprintf(fp, "[header]\n");
    fprintf(fp, "version = %d\n",  SAVE_VERSION);
    fprintf(fp, "slot    = %d\n",  slot);
    fprintf(fp, "player  = %s\n",  w->player->name[0] ? w->player->name : "0");

    fprintf(fp, "\n[world]\n");
    fprintf(fp, "seed = %u\n", w->seed);

    fprintf(fp, "\n[map]\n");
    fprintf(fp, "width  = %zu\n", w->map->w);
    fprintf(fp, "height = %zu\n", w->map->h);
    for (size_t y = 0; y < w->map->h; ++y) {
        fprintf(fp, "row :");
        for (size_t x = 0; x < w->map->w; ++x)
            fprintf(fp, " %d", cell_ref(w->map, y, x)->elevation);
        fputc('\n', fp);
    }

    fprintf(fp, "\n[objects]\n");
    for (size_t y = 0; y < w->map->h; ++y) {
        for (size_t x = 0; x < w->map->w; ++x) {
            Cell *cell = cell_ref(w->map, y, x);
            if (!cell->object_id)
                continue;
            fprintf(fp, "object : %u %zu %zu %d\n",
                    cell->object_id, x, y, cell->object_health);
        }
    }

    fprintf(fp, "\n[entities]\n");
    da_foreach(Entity, ent, &w->entities) {
        assert(ent && ent->def);
        fprintf(fp, "entity : %u %zu %zu %d %s\n",
                ent->def->id, ent->x, ent->y, ent->health,
                ent->name[0] ? ent->name : "0");
        da_foreach(Item, item, &ent->inventory) {
            if (item->def->id >= 30000)
                fprintf(fp, "entity + %u %d %d\n",
                        item->def->id, item->stack, item->durability);
            else
                fprintf(fp, "entity + %u %d\n", item->def->id, item->stack);
        }
    }

defer:
    if (fp)
        fclose(fp);
    return ret;
}

SaveResult world_load(World *w, int slot, Cw *ctx)
{
    info("[save] loading slot %d", slot);
    assert(w && ctx);
    SaveResult ret  = SAVE_OK;
    FILE *fp = fopen(get_save_path(slot), "r");
    if (!fp)
        do_defer_and_return(SAVE_ERR_OPEN);

    da_reserve(&w->entities, 512);

    char   buf[8192];
    CwLine line;
    char   section[64] = {0};
    size_t map_w = 0, map_h = 0, row = 0;
    Entity *current_entity = NULL;

    while (cw_next_line(fp, buf, sizeof(buf), &line)) {
        if (line.kind == CW_LINE_SECTION) {
            sv_to_cstr(line.tag, section, sizeof(section));
            continue;
        }

        if (strcmp(section, "world") == 0) {
            if (line.kind == CW_LINE_KV && sv_eq_cstr(line.tag, "seed"))
                w->seed = (uint32_t)sv_to_ulong(line.val);

        } else if (strcmp(section, "map") == 0) {
            if (line.kind == CW_LINE_KV) {
                if      (sv_eq_cstr(line.tag, "width"))  map_w = (size_t)sv_to_ulong(line.val);
                else if (sv_eq_cstr(line.tag, "height")) map_h = (size_t)sv_to_ulong(line.val);
                if (map_w && map_h && !w->map) {
                    w->map = map_alloc(map_h, map_w);
                    if (!w->map)
                        do_defer_and_return(SAVE_ERR_READ);
                }
            } else if (line.kind == CW_LINE_STRUCT && sv_eq_cstr(line.tag, "row")) {
                if (!w->map || row >= w->map->h)
                    do_defer_and_return(SAVE_ERR_READ);
                const char *p = line.val.ptr;
                for (size_t x = 0; x < w->map->w; ++x) {
                    char *end;
                    long v = strtol(p, &end, 10);
                    if (p == end)
                        do_defer_and_return(SAVE_ERR_READ);
                    cell_ref(w->map, row, x)->elevation = (Elevation)v;
                    p = end;
                }
                row++;
            }

        } else if (strcmp(section, "objects") == 0) {
            if (line.kind != CW_LINE_STRUCT || !sv_eq_cstr(line.tag, "object"))
                continue;
            uint16_t def_id = 0;
            uint64_t x = 0, y = 0;
            int health = 0;
            if (sscanf(line.val.ptr, "%hu %zu %zu %d", &def_id, &x, &y, &health) != 4)
                do_defer_and_return(SAVE_ERR_READ);
            if (!w->map || x >= w->map->w || y >= w->map->h)
                do_defer_and_return(SAVE_ERR_READ);
            cell_ref(w->map, y, x)->object_id     = def_id;
            cell_ref(w->map, y, x)->object_health = health;

        } else if (strcmp(section, "entities") == 0) {
            if (line.kind == CW_LINE_STRUCT && sv_eq_cstr(line.tag, "entity")) {
                Entity entity = {0};
                uint16_t def_id = 0;
                char name[32] = {0};
                if (sscanf(line.val.ptr, "%hu %zu %zu %d %31[^\n]",
                           &def_id, &entity.x, &entity.y, &entity.health, name) != 5)
                    do_defer_and_return(SAVE_ERR_READ);
                entity.def = entity_def_lookup(ctx->entity_defs, def_id);
                if (!entity.def)
                    do_defer_and_return(SAVE_ERR_READ);
                if (strcmp(name, "0") != 0)
                    snprintf(entity.name, sizeof(entity.name), "%s", name);
                da_append(&w->entities, entity);
                current_entity = &w->entities.items[w->entities.count - 1];
            } else if (line.kind == CW_LINE_APPEND && sv_eq_cstr(line.tag, "entity")) {
                if (!current_entity)
                    do_defer_and_return(SAVE_ERR_READ);

                uint16_t item_id = 0;
                int stack = 0;
                int durability = 0;

                int parsed = sscanf(line.val.ptr, "%hu %d %d", &item_id, &stack, &durability);
                if (parsed < 2)
                    do_defer_and_return(SAVE_ERR_READ);

                ItemDef *idef = item_def_lookup(ctx->item_defs, item_id);
                if (idef) {
                    Item it = {
                        .def = idef,
                        .stack = stack,
                        .durability = (parsed == 3) ? durability : 0
                    };
                    da_append(&current_entity->inventory, it);
                }
            }
        }
    }

    if (!w->map || row != w->map->h)
        do_defer_and_return(SAVE_ERR_READ);

    w->player = world_link_entities_to_cells(w, 0);

defer:
    if (fp)
        fclose(fp);
    if (ret != SAVE_OK) {
        world_free(w);
        memset(w, 0, sizeof(*w));
        error("[save] failed to load slot %d", slot);
    }
    return ret;
}

SaveResult save_delete(int slot)
{
    if (remove(get_save_path(slot)) != 0)
        return SAVE_ERR_WRITE;
    return SAVE_OK;
}

SavePreview save_preview(int slot)
{
    SavePreview p = {0};
    FILE *fp = fopen(get_save_path(slot), "r");
    if (!fp)
        return p;

    char   buf[256];
    CwLine line;
    char   section[64] = {0};

    while (cw_next_line(fp, buf, sizeof(buf), &line)) {
        if (line.kind == CW_LINE_SECTION) {
            sv_to_cstr(line.tag, section, sizeof(section));
            if (strcmp(section, "header") != 0)
                break; // past the header section, stop
            continue;
        }
        if (strcmp(section, "header") != 0 || line.kind != CW_LINE_KV)
            continue;
        if (sv_eq_cstr(line.tag, "version"))
            p.header.version = (uint32_t)sv_to_ulong(line.val);
        else if (sv_eq_cstr(line.tag, "slot"))
            p.header.slot = sv_to_int(line.val);
        else if (sv_eq_cstr(line.tag, "player"))
            sv_to_cstr(line.val, p.header.player_name, sizeof(p.header.player_name));
    }

    p.exists = p.header.version != 0;
    fclose(fp);
    return p;
}


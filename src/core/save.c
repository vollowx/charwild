#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/save.h"
#include "core/common.h"
#include "core/context.h"
#include "core/log.h"
#include "core/world_defs.h"

char save_path[128] = {0};

static char *get_save_path(int slot) {
    snprintf(save_path, 128, CW_SAVES_PATH, slot);
    return save_path;
}

static void clear_world(World *w) {
    world_free(w);
    memset(w, 0, sizeof(*w));
}

static void copy_name(char *dst, size_t n, const char *src) {
    snprintf(dst, n, "%s", src ? src : "");
}

SaveResult world_save(const World *w, int slot) {
    info("[save] saving slot %d", slot);

    SaveResult ret = SAVE_OK;
    char *path = get_save_path(slot);
    FILE *fp = NULL;

    if (!w || !w->map || !path)
        return SAVE_ERR_WRITE;

    fp = fopen(path, "w");
    if (!fp)
        do_defer_and_return(SAVE_ERR_OPEN);

    fprintf(fp, "@header\n");
    fprintf(fp, "slot = %d\n", slot);
    // fprintf(fp, "version = %u\n", self->header.version);
    // fprintf(fp, "player_name = %s\n\n", self->header.player_name);

    fprintf(fp, "@world\n");
    fprintf(fp, "seed = %u\n", w->seed);
    // fprintf(fp, "timestamp = %u\n\n", self->header.timestamp);

    fprintf(fp, "@map %zu %zu\n", w->map->w, w->map->h);
    for (size_t y = 0; y < w->map->h; ++y) {
        fprintf(fp, "@row");
        for (size_t x = 0; x < w->map->w; ++x)
            fprintf(fp, " %d", w->map->cells[y][x].elevation);
        fputc('\n', fp);
    }
    fputc('\n', fp);

    for (size_t y = 0; y < w->map->h; ++y) {
        for (size_t x = 0; x < w->map->w; ++x) {
            MapCell *cell = &w->map->cells[y][x];
            if (!cell->object_id)
                continue;
            fprintf(fp, "@object %u %zu %zu %d\n", cell->object_id, x, y,
                    cell->object_health);
        }
    }

    da_foreach(Entity, ent, &w->entities) {
        assert(ent && ent->def);

        fprintf(fp, "@entity %u %zu %zu %d %s\n", ent->def->id, ent->x, ent->y,
                ent->health, ent->name[0] ? ent->name : "0");

        da_foreach(ItemStack, item, &ent->inventory) {
            if (item->def->id >= 30000)
                fprintf(fp, "+ %u %d %d\n", item->def->id, item->quantity,
                        item->durability);
            else
                fprintf(fp, "+ %u %d\n", item->def->id, item->quantity);
        }
    }

defer:
    if (fp)
        fclose(fp);
    return ret;
}

SaveResult world_load(World *w, int slot, Cw *ctx) {
    info("[save] loading slot %d", slot);
    SaveResult ret = SAVE_OK;
    char *path = get_save_path(slot);
    FILE *fp = NULL;
    char raw[8192];
    size_t row = 0;
    Entity *cur_ent = NULL;

    if (!w || !path)
        return SAVE_ERR_READ;

    fp = fopen(path, "r");
    if (!fp)
        do_defer_and_return(SAVE_ERR_OPEN);

    da_reserve(&w->entities, 512);

    while (fgets(raw, sizeof(raw), fp)) {
        char *line = cw_trim(raw);
        if (cw_is_ignored_line(line))
            continue;

        if (strcmp(line, "@header") == 0 || strcmp(line, "@world") == 0) {
            continue;
        } else if (strncmp(line, "slot =", 6) == 0) {
            // self->header.slot = atoi(cw_trim(line + 6));
        } else if (strncmp(line, "version =", 9) == 0) {
            // self->header.version = (uint32_t)strtoul(cw_trim(line + 9), NULL, 10);
        } else if (strncmp(line, "player_name =", 13) == 0) {
            // copy_name(self->header.player_name,
            //           sizeof(self->header.player_name), cw_trim(line + 13));
        } else if (strncmp(line, "seed =", 6) == 0) {
            w->seed = (uint32_t)strtoul(cw_trim(line + 6), NULL, 10);
        } else if (strncmp(line, "timestamp =", 11) == 0) {
            // self->header.timestamp =
            //     (uint32_t)strtoul(cw_trim(line + 11), NULL, 10);
        } else if (strncmp(line, "@map ", 5) == 0) {
            size_t width = 0, height = 0;
            if (sscanf(line, "@map %zu %zu", &width, &height) != 2 || !width || !height)
                do_defer_and_return(SAVE_ERR_READ);
            w->map = map_alloc(height, width);
            if (!w->map)
                do_defer_and_return(SAVE_ERR_READ);
            row = 0;
        } else if (strncmp(line, "@row", 4) == 0) {
            if (!w->map || row >= w->map->h)
                do_defer_and_return(SAVE_ERR_READ);

            char *p = line + 4;
            for (size_t x = 0; x < w->map->w; ++x) {
                char *end;
                long v;

                while (*p == ' ' || *p == '\t')
                    ++p;
                v = strtol(p, &end, 10);
                if (p == end)
                    do_defer_and_return(SAVE_ERR_READ);

                w->map->cells[row][x].elevation = (Elevation)v;
                p = end;
            }
            row++;
        } else if (strncmp(line, "@object ", 8) == 0) {
            unsigned int def = 0;
            size_t x = 0, y = 0;
            int hp = 0;

            if (!w->map ||
                sscanf(line, "@object %u %zu %zu %d", &def, &x, &y, &hp) != 4)
                do_defer_and_return(SAVE_ERR_READ);
            if (x >= w->map->w || y >= w->map->h)
                do_defer_and_return(SAVE_ERR_READ);

            w->map->cells[y][x].object_id = (uint16_t)def;
            w->map->cells[y][x].object_health = hp;
        } else if (strncmp(line, "@entity ", 8) == 0) {
            unsigned int def_id = 0;
            size_t x = 0, y = 0;
            int hp = 0;
            char name[64] = {0};

            if (sscanf(line, "@entity %u %zu %zu %d %63[^\n]", &def_id, &x, &y,
                       &hp, name) != 5)
                do_defer_and_return(SAVE_ERR_READ);

            Entity new_ent = {0};
            new_ent.def = entity_def_lookup(ctx->entity_defs, def_id);
            new_ent.x = x;
            new_ent.y = y;
            new_ent.health = hp;

            if (!new_ent.def)
                do_defer_and_return(SAVE_ERR_READ);
            if (strcmp(name, "0") != 0)
                copy_name(new_ent.name, sizeof(new_ent.name), name);

            da_append(&w->entities, new_ent);
            cur_ent = &w->entities.items[w->entities.count - 1];
        } else if (strncmp(line, "+ ", 2) == 0) {
            // TODO: To be redone
            //
            // unsigned int def_id = 0;
            // int qty = 0, dur = 0;
            // int n;
            //
            // if (!cur_ent)
            //     do_defer_and_return(SAVE_ERR_READ);
            //
            // n = sscanf(line, "+ %u %d %d", &def_id, &qty, &dur);
            // if (n < 2)
            //     do_defer_and_return(SAVE_ERR_READ);
            //
            // ItemStack item = {0};
            // item.def = item_def_lookup(ctx->item_defs, def_id);
            // item.quantity = qty;
            // if (n >= 3)
            //     item.durability = dur;
            //
            // if (!item.def)
            //     do_defer_and_return(SAVE_ERR_READ);
            //
            // da_append(&cur_ent->inventory, item);
        } else {
            do_defer_and_return(SAVE_ERR_READ);
        }
    }

    // if (self->header.version != SAVE_VERSION)
    //     do_defer_and_return(SAVE_ERR_VERSION);
    if (!w->map || row != w->map->h)
        do_defer_and_return(SAVE_ERR_READ);

    w->player = NULL;
    for (size_t i = 0; i < w->entities.count; ++i) {
        Entity *ent = &w->entities.items[i];
        if (ent->x >= w->map->w || ent->y >= w->map->h)
            do_defer_and_return(SAVE_ERR_READ);

        w->map->cells[ent->y][ent->x].entity = ent;
        if (ent->def->type == ENTITY_PLAYER)
            w->player = ent;
    }

defer:
    if (fp)
        fclose(fp);
    if (ret != SAVE_OK) {
        clear_world(w);
        error("[save] failed to load save %d", slot);
    }
    return ret;
}

SaveResult save_delete(int slot) {
    char *path = get_save_path(slot);
    SaveResult ret = SAVE_OK;

    if (!path)
        return SAVE_ERR_WRITE;
    if (remove(path) != 0)
        ret = SAVE_ERR_WRITE;

    return ret;
}

SavePreview get_save_preview(int slot) {
    SavePreview p = {0};
    char *path = get_save_path(slot);
    FILE *fp = NULL;
    char raw[256];

    if (!path)
        return p;

    fp = fopen(path, "r");
    if (!fp) {
        return p;
    }

    while (fgets(raw, sizeof(raw), fp)) {
        char *line = cw_trim(raw);
        if (cw_is_ignored_line(line))
            continue;

        if (strncmp(line, "slot =", 6) == 0)
            p.header.slot = atoi(cw_trim(line + 6));
        else if (strncmp(line, "version =", 9) == 0)
            p.header.version = (uint32_t)strtoul(cw_trim(line + 9), NULL, 10);
        else if (strncmp(line, "player_name =", 13) == 0)
            copy_name(p.header.player_name, sizeof(p.header.player_name),
                      cw_trim(line + 13));
        else if (line[0] == '@' && strcmp(line, "@header") != 0)
            break;
    }

    p.exists = p.header.version != 0;

    fclose(fp);
    return p;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/context.h"
#include "core/common.h"
#include "core/log.h"
#include "core/world_defs.h"
#include "core/save.h"

static char *get_save_path(int slot) {
    char *buf = malloc(128);
    if (!buf)
        return NULL;
    snprintf(buf, 128, "%d.cwsave", slot);
    return buf;
}

static void clear_world(World *w) {
    free_world(w);
    memset(w, 0, sizeof(*w));
}

static void copy_name(char *dst, size_t n, const char *src) {
    snprintf(dst, n, "%s", src ? src : "");
}

SaveResult save_save(const Save *self, int slot) {
    info("[save] saving slot %d", slot);

    SaveResult ret = SAVE_OK;
    char *path = get_save_path(slot);
    FILE *fp = NULL;

    if (!self || !self->world || !self->world->map || !path)
        return SAVE_ERR_WRITE;

    fp = fopen(path, "w");
    if (!fp)
        do_defer_and_return(SAVE_ERR_OPEN);

    fprintf(fp, "@header\n");
    fprintf(fp, "slot = %d\n", slot);
    fprintf(fp, "version = %u\n", self->header.version);
    fprintf(fp, "player_name = %s\n\n", self->header.player_name);

    fprintf(fp, "@world\n");
    fprintf(fp, "seed = %u\n", self->world->seed);
    fprintf(fp, "timestamp = %u\n\n", self->header.timestamp);

    fprintf(fp, "@map %zu %zu\n", self->world->map->w, self->world->map->h);
    for (size_t y = 0; y < self->world->map->h; ++y) {
        fprintf(fp, "@row");
        for (size_t x = 0; x < self->world->map->w; ++x)
            fprintf(fp, " %d", self->world->map->cells[y][x].elevation);
        fputc('\n', fp);
    }
    fputc('\n', fp);

    for (size_t y = 0; y < self->world->map->h; ++y) {
        for (size_t x = 0; x < self->world->map->w; ++x) {
            MapCell *cell = &self->world->map->cells[y][x];
            if (!cell->object_id)
                continue;
            fprintf(fp, "@object %u %zu %zu %d\n", cell->object_id, x, y,
                    cell->object_health);
        }
    }

    da_foreach(Entity, ent, &self->world->entities) {
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
    free(path);
    return ret;
}

SaveResult save_load(Save *self, int slot, Cw *ctx) {
    info("[save] loading to slot %d", slot);
    SaveResult ret = SAVE_OK;
    char *path = get_save_path(slot);
    FILE *fp = NULL;
    char raw[8192];
    size_t row = 0;
    Entity *cur_ent = NULL;

    if (!self || !self->world || !path)
        return SAVE_ERR_READ;

    fp = fopen(path, "r");
    if (!fp)
        do_defer_and_return(SAVE_ERR_OPEN);

    memset(&self->header, 0, sizeof(self->header));
    clear_world(self->world);
    da_reserve(&self->world->entities, 512);

    while (fgets(raw, sizeof(raw), fp)) {
        char *line = cw_trim(raw);
        if (cw_is_ignored_line(line))
            continue;

        if (strcmp(line, "@header") == 0 || strcmp(line, "@world") == 0) {
            continue;
        } else if (strncmp(line, "slot =", 6) == 0) {
            self->header.slot = atoi(cw_trim(line + 6));
        } else if (strncmp(line, "version =", 9) == 0) {
            self->header.version = (uint32_t)strtoul(cw_trim(line + 9), NULL, 10);
        } else if (strncmp(line, "player_name =", 13) == 0) {
            copy_name(self->header.player_name,
                      sizeof(self->header.player_name), cw_trim(line + 13));
        } else if (strncmp(line, "seed =", 6) == 0) {
            self->world->seed = (uint32_t)strtoul(cw_trim(line + 6), NULL, 10);
        } else if (strncmp(line, "timestamp =", 11) == 0) {
            self->header.timestamp =
                (uint32_t)strtoul(cw_trim(line + 11), NULL, 10);
        } else if (strncmp(line, "@map ", 5) == 0) {
            size_t w = 0, h = 0;
            if (sscanf(line, "@map %zu %zu", &w, &h) != 2 || !w || !h)
                do_defer_and_return(SAVE_ERR_READ);
            self->world->map = new_map(h, w);
            if (!self->world->map)
                do_defer_and_return(SAVE_ERR_READ);
            row = 0;
        } else if (strncmp(line, "@row", 4) == 0) {
            if (!self->world->map || row >= self->world->map->h)
                do_defer_and_return(SAVE_ERR_READ);

            char *p = line + 4;
            for (size_t x = 0; x < self->world->map->w; ++x) {
                char *end;
                long v;

                while (*p == ' ' || *p == '\t')
                    ++p;
                v = strtol(p, &end, 10);
                if (p == end)
                    do_defer_and_return(SAVE_ERR_READ);

                self->world->map->cells[row][x].elevation = (Elevation)v;
                p = end;
            }
            row++;
        } else if (strncmp(line, "@object ", 8) == 0) {
            unsigned int def = 0;
            size_t x = 0, y = 0;
            int hp = 0;

            if (!self->world->map ||
                sscanf(line, "@object %u %zu %zu %d", &def, &x, &y, &hp) != 4)
                do_defer_and_return(SAVE_ERR_READ);
            if (x >= self->world->map->w || y >= self->world->map->h)
                do_defer_and_return(SAVE_ERR_READ);

            self->world->map->cells[y][x].object_id = (uint16_t)def;
            self->world->map->cells[y][x].object_health = hp;
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

            da_append(&self->world->entities, new_ent);
            cur_ent = &self->world->entities.items[self->world->entities.count - 1];
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

    if (self->header.version != SAVE_VERSION)
        do_defer_and_return(SAVE_ERR_VERSION);
    if (!self->world->map || row != self->world->map->h)
        do_defer_and_return(SAVE_ERR_READ);

    self->world->player = NULL;
    for (size_t i = 0; i < self->world->entities.count; ++i) {
        Entity *ent = &self->world->entities.items[i];
        if (ent->x >= self->world->map->w || ent->y >= self->world->map->h)
            do_defer_and_return(SAVE_ERR_READ);

        self->world->map->cells[ent->y][ent->x].entity = ent;
        if (ent->def->type == ENTITY_PLAYER)
            self->world->player = ent;
    }

defer:
    if (fp)
        fclose(fp);
    if (ret != SAVE_OK) {
        clear_world(self->world);
        error("[save] failed to load save");
    }
    free(path);
    return ret;
}

SaveResult save_delete(int slot) {
    char *path = get_save_path(slot);
    SaveResult ret = SAVE_OK;

    if (!path)
        return SAVE_ERR_WRITE;
    if (remove(path) != 0)
        ret = SAVE_ERR_WRITE;

    free(path);
    return ret;
}

void save_init(Save *self) {
    if (!self)
        return;
    self->header.slot = 0;
    self->header.version = SAVE_VERSION;
    self->header.timestamp = (uint32_t)time(NULL);
    self->header.player_name[0] = '\0';
}

SavePreview get_slot_preview(int slot) {
    SavePreview p = {0};
    char *path = get_save_path(slot);
    FILE *fp = NULL;
    char raw[256];

    if (!path)
        return p;

    fp = fopen(path, "r");
    if (!fp) {
        free(path);
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
    free(path);
    return p;
}

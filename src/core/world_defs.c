#include "core/helpers.h"
#include "core/world_defs.h"

static const struct { const char *name; short val; } COLOR_TABLE[] = {
    {"none",    -1},
    {"black",   COLOR_BLACK},
    {"red",     COLOR_RED},
    {"green",   COLOR_GREEN},
    {"yellow",  COLOR_YELLOW},
    {"blue",    COLOR_BLUE},
    {"magenta", COLOR_MAGENTA},
    {"cyan",    COLOR_CYAN},
    {"white",   COLOR_WHITE},
};

static short parse_color(const char *s) {
    for (size_t i = 0; i < sizeof(COLOR_TABLE) / sizeof(*COLOR_TABLE); ++i)
        if (strcmp(s, COLOR_TABLE[i].name) == 0)
            return COLOR_TABLE[i].val;
    return 0;
}

static bool parse_bool(const char *s, bool *out) {
    if (strcmp(s, "0") == 0) {
        *out = false;
        return true;
    }
    if (strcmp(s, "1") == 0) {
        *out = true;
        return true;
    }
    return false;
}

bool definitions_load(
    const char *path,
    ItemDefs   *item_defs,
    EntityDefs *entity_defs,
    ObjectDefs *object_defs
) {
    bool ret = 0;
    ItemDefs   items    = {0};
    EntityDefs entities = {0};
    ObjectDefs objects  = {0};

    FILE *fp = fopen(path, "r");
    if (!fp)
        do_defer_and_return(false);

    char line_buf[512];
    while (fgets(line_buf, sizeof(line_buf), fp)) {
        char *line = cw_trim(line_buf);
        if (cw_is_ignored_line(line)) continue;

        if (strncmp(line, "@resource ", 10) == 0 ||
            strncmp(line, "@placeable ", 11) == 0 ||
            strncmp(line, "@consumable ", 12) == 0 ||
            strncmp(line, "@equipment ", 11) == 0) {
            ItemDef def = {0};

            char kind[32] = {0};
            char symbol[8] = {0};
            char fg_name[32] = {0};
            char bg_name[32] = {0};
            char name[32] = {0};
            int n = 0;

            n = sscanf(line, "@%31s %hu %d %7s %31s %31s %31[^\n]",
                       kind, &def.id, &def.max_stack, symbol,
                       fg_name, bg_name, name);

            if (n != 7)
                do_defer_and_return(false);

            if (strcmp(kind, "resource") == 0)
                def.type = ITEM_RESOURCE;
            else if (strcmp(kind, "placeable") == 0)
                def.type = ITEM_PLACEABLE;
            else if (strcmp(kind, "consumable") == 0)
                def.type = ITEM_CONSUMABLE;
            else if (strcmp(kind, "equipment") == 0)
                def.type = ITEM_EQUIPMENT;

            def.symbol[0] = symbol[0];
            def.symbol[1] = symbol[1];
            def.fg = parse_color(fg_name);
            def.bg = parse_color(bg_name);
            def.attr = 0;
            snprintf(def.name, sizeof(def.name), "%s", name);

            da_append(&items, def);
        } else if (strncmp(line, "@player ", 8) == 0 ||
                   strncmp(line, "@animal ", 8) == 0) {
            EntityDef def = {0};
            char kind[32] = {0};
            char passable_str[8] = {0};
            char symbol[8] = {0};
            char fg_name[32] = {0};
            char bg_name[32] = {0};
            char name[32] = {0};

            if (sscanf(line, "@%31s %hu %d %7s %7s %31s %31s %31[^\n]",
                       kind, &def.id, &def.max_health, passable_str, symbol,
                       fg_name, bg_name, name) != 8) {
                do_defer_and_return(false);
            }

            if (strcmp(kind, "player") == 0)
                def.type = ENTITY_PLAYER;
            else if (strcmp(kind, "animal") == 0)
                def.type = ENTITY_ANIMAL;
            else
                do_defer_and_return(false);

            if (!parse_bool(passable_str, &def.is_passable))
                do_defer_and_return(false);
            if (strlen(symbol) != 2)
                do_defer_and_return(false);

            def.symbol[0] = symbol[0];
            def.symbol[1] = symbol[1];
            def.fg = parse_color(fg_name);
            def.bg = parse_color(bg_name);
            def.attr = 0;
            snprintf(def.name, sizeof(def.name), "%s", name);

            da_append(&entities, def);
        } else if (strncmp(line, "@object ", 8) == 0) {
            ObjectDef def = {0};
            char passable_str[8] = {0};
            char symbol[8] = {0};
            char fg_name[32] = {0};
            char bg_name[32] = {0};
            char name[32] = {0};

            if (sscanf(line, "@object %hu %d %7s %7s %31s %31s %31[^\n]",
                       &def.id, &def.max_health, passable_str, symbol,
                       fg_name, bg_name, name) != 7) {
                do_defer_and_return(false);
            }

            if (!parse_bool(passable_str, &def.is_passable))
                do_defer_and_return(false);

            def.symbol[0] = symbol[0];
            def.symbol[1] = symbol[1];
            def.fg = parse_color(fg_name);
            def.bg = parse_color(bg_name);
            def.attr = 0;
            snprintf(def.name, sizeof(def.name), "%s", name);

            da_append(&objects, def);
        }
    }

    *item_defs = items;
    *entity_defs = entities;
    *object_defs = objects;
    items = (ItemDefs){0};
    entities = (EntityDefs){0};
    objects = (ObjectDefs){0};

defer:
    if (fp)
        fclose(fp);
    if (ret != true) {
        da_free(items);
        da_free(entities);
        da_free(objects);
    }
    return ret;
}

ItemDef *item_def_lookup(ItemDefs defs, uint16_t id) {
    da_foreach(ItemDef, def, &defs)
        if (def->id == id)
            return def;
    return NULL;
}

EntityDef *entity_def_lookup(EntityDefs defs, uint16_t id) {
    da_foreach(EntityDef, def, &defs)
        if (def->id == id)
            return def;
    return NULL;
}

ObjectDef *object_def_lookup(ObjectDefs defs, uint16_t id) {
    da_foreach(ObjectDef, def, &defs)
        if (def->id == id)
            return def;
    return NULL;
}

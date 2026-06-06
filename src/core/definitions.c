#include "core/common.h"
#include "core/definitions.h"

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
    if (strcmp(s, "0") == 0) { *out = false; return true; }
    if (strcmp(s, "1") == 0) { *out = true;  return true; }
    return false;
}

bool definitions_load(
    const char *path,
    ItemDefs   *item_defs,
    EntityDefs *entity_defs,
    ObjectDefs *object_defs
) {
    bool ret = true;
    ItemDefs   items    = {0};
    EntityDefs entities = {0};
    ObjectDefs objects  = {0};

    FILE *fp = fopen(path, "r");
    if (!fp)
        do_defer_and_return(false);

    char buf[512];
    CwLine line;
    char section[64] = {0};

    while (cw_next_line(fp, buf, sizeof(buf), &line)) {
        if (line.kind == CW_LINE_SECTION) {
            sv_to_cstr(line.tag, section, sizeof(section));
            continue;
        }
        if (line.kind != CW_LINE_STRUCT)
            continue;

        if (strcmp(section, "items") == 0) {
            ItemDef def = {0};
            char fg_name[32] = {0}, bg_name[32] = {0}, name[32] = {0};
            if (sscanf(line.val.ptr, "%hu %d %7s %31s %31s %31[^\n]",
                       &def.id, &def.max_stack, def.symbol,
                       fg_name, bg_name, name) != 6)
                do_defer_and_return(false);
            if      (sv_eq_cstr(line.tag, "resource"))   def.type = ITEM_RESOURCE;
            else if (sv_eq_cstr(line.tag, "placeable"))  def.type = ITEM_PLACEABLE;
            else if (sv_eq_cstr(line.tag, "consumable")) def.type = ITEM_CONSUMABLE;
            else if (sv_eq_cstr(line.tag, "equipment"))  def.type = ITEM_EQUIPMENT;
            else do_defer_and_return(false);
            def.fg = parse_color(fg_name);
            def.bg = parse_color(bg_name);
            snprintf(def.name, sizeof(def.name), "%s", name);
            da_append(&items, def);
        } else if (strcmp(section, "entities") == 0) {
            EntityDef def = {0};
            char passable[8] = {0}, fg_name[32] = {0}, bg_name[32] = {0}, name[32] = {0};
            if (sscanf(line.val.ptr, "%hu %d %7s %7s %31s %31s %31[^\n]",
                       &def.id, &def.max_health, passable, def.symbol,
                       fg_name, bg_name, name) != 7)
                do_defer_and_return(false);
            if      (sv_eq_cstr(line.tag, "player")) def.type = ENTITY_PLAYER;
            else if (sv_eq_cstr(line.tag, "animal")) def.type = ENTITY_ANIMAL;
            else do_defer_and_return(false);
            if (!parse_bool(passable, &def.is_passable)) do_defer_and_return(false);
            def.fg = parse_color(fg_name);
            def.bg = parse_color(bg_name);
            snprintf(def.name, sizeof(def.name), "%s", name);
            da_append(&entities, def);
        } else if (strcmp(section, "objects") == 0) {
            ObjectDef def = {0};
            char passable[8] = {0}, fg_name[32] = {0}, bg_name[32] = {0},
                 name[32] = {0};
            if (!sv_eq_cstr(line.tag, "object")) do_defer_and_return(false);
            if (sscanf(line.val.ptr, "%hu %d %7s %7s %31s %31s %31[^\n]",
                       &def.id, &def.max_health, passable, def.symbol,
                       fg_name, bg_name, name) != 7)
                do_defer_and_return(false);
            if (!parse_bool(passable, &def.is_passable)) do_defer_and_return(false);
            def.fg = parse_color(fg_name);
            def.bg = parse_color(bg_name);
            snprintf(def.name, sizeof(def.name), "%s", name);
            da_append(&objects, def);
        }
    }

    *item_defs   = items;
    *entity_defs = entities;
    *object_defs = objects;
    items    = (ItemDefs){0};
    entities = (EntityDefs){0};
    objects  = (ObjectDefs){0};

defer:
    if (fp)
        fclose(fp);
    if (ret != true) {
        da_free(items);
        da_free(entities);
        da_free(objects);
        error("[definitions] failed to load from %s", path);
    } else {
        info("[definitions] loaded from %s", path);
    }
    return ret;
}

// TODO: Optimize definition searching
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


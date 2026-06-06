#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

#include "core/world.h"

typedef struct Cw Cw;

#define SAVE_VERSION 1
#define MAX_N_SLOTS 3

typedef enum {
    SAVE_OK,
    SAVE_ERR_OPEN,
    SAVE_ERR_READ,
    SAVE_ERR_WRITE,
    SAVE_ERR_VERSION,
    SAVE_ERR_FORMAT
} SaveResult;

typedef struct {
    int slot;
    uint32_t version;
    uint32_t timestamp;
    char player_name[32];
} SaveHeader;

typedef struct {
    SaveHeader header;
    World *world;
} Save;

typedef struct {
    SaveHeader header;
    bool exists;
} SavePreview;

SaveResult world_load(World *w, int slot, Cw *ctx);
SaveResult world_save(const World *w, int slot);
SaveResult save_delete(int slot);

SavePreview save_preview(int slot);

#endif

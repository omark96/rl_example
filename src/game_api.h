#pragma once
#include "raylib.h"
#include "umka_full.h"
#include <stdlib.h>

#define H_TYPE_SHIFT 58
#define H_OWNER_SHIFT 51
#define H_OGEN_SHIFT 47
#define H_SLOT_SHIFT 31
#define H_SGEN_SHIFT 0

#define H_TYPE_MASK 0x1FULL
#define H_OWNER_MASK 0x7FULL
#define H_OGEN_MASK 0x0FULL
#define H_SLOT_MASK 0xFFFFULL
#define H_SGEN_MASK 0x7FFFFFFFULL

#define HANDLE_NULL 0ULL

#define MAX_GAMES 10
#define MAX_TEXTURES 256

typedef uint64_t Handle;

typedef enum {
    RES_NONE = 0,
    RES_GAME,
    RES_TEXTURE,
    RES_RENDER_TEXTURE,
    RES_TYPE_COUNT
} ResType;

static_assert(RES_TYPE_COUNT <= 32, "type field overflow");

static inline uint8_t handleType(Handle h) { return (h >> H_TYPE_SHIFT) & H_TYPE_MASK; }

static inline uint8_t handleOwnerId(Handle h) { return (h >> H_OWNER_SHIFT) & H_OWNER_MASK; }

static inline uint8_t handleOwnerGen(Handle h) { return (h >> H_OGEN_SHIFT) & H_OGEN_MASK; }

static inline uint16_t handleSlot(Handle h) { return (h >> H_SLOT_SHIFT) & H_SLOT_MASK; }

static inline uint32_t handleSlotGen(Handle h) { return h & H_SGEN_MASK; }

static inline Handle makeHandle(uint8_t type, uint8_t gameId, uint8_t gameGen, uint16_t slot,
                                uint32_t slotGen) {
    return ((Handle)(type & H_TYPE_MASK) << H_TYPE_SHIFT)
           | ((Handle)(gameId & H_OWNER_MASK) << H_OWNER_SHIFT)
           | ((Handle)(gameGen & H_OGEN_MASK) << H_OGEN_SHIFT)
           | ((Handle)(slot & H_SLOT_MASK) << H_SLOT_SHIFT) | (Handle)(slotGen & H_SGEN_MASK);
}

typedef struct {
    uint8_t id;
    uint8_t gen;
} GameRef;

static inline GameRef handleGameRef(Handle h) {
    return (GameRef){handleOwnerId(h), handleOwnerGen(h)};
}

static inline Handle makeGameHandle(GameRef g) { return makeHandle(RES_GAME, g.id, g.gen, 0, 0); }

#define POOL_INIT_SIZE 4

typedef struct TextureSlot {
    Texture item;
    uint32_t generation;
    uint32_t nextFree;
} TextureSlot;

typedef struct TexturePool {
    TextureSlot *items;
    uint32_t count;
    uint32_t cap;
    uint32_t firstFree;
    uint8_t ownerId;
    uint8_t ownerGen;
} TexturePool;

void texturePoolInit(TexturePool *pool, uint8_t ownerId, uint8_t ownerGen) {
    pool->items = malloc(sizeof(TextureSlot) * POOL_INIT_SIZE);
    pool->count = 0;
    pool->cap = POOL_INIT_SIZE;
    pool->firstFree = 0;
    pool->ownerId = ownerId;
    pool->ownerGen = ownerGen;
}

bool texturePoolGrow(TexturePool *pool) { return true; }

Handle texturePoolAdd(TexturePool *pool, Texture item) {
    uint32_t slotId = pool->firstFree;
    if (slotId != 0) {
        pool->firstFree = pool->items[slotId].nextFree;
    } else {
        pool->count += 1;
        if (pool->count == pool->cap) {
            bool ok = texturePoolGrow(pool);
            assert(ok);
        }
    }

    TextureSlot *slot = &pool->items[slotId];
    slot->nextFree = 0;
    slot->item = item;

    return makeHandle(RES_TEXTURE, pool->ownerId, pool->ownerGen, slotId, slot->generation);
}

bool texturePoolResolve(TexturePool *pool, Handle handle, uint16_t *outSlotId) {
    if (handleType(handle) != RES_TEXTURE) {
        return false;
    }
    if (handleOwnerId(handle) != pool->ownerId) {
        return false;
    }
    if (handleOwnerGen(handle) != pool->ownerGen) {
        return false;
    }
    uint16_t slotId = handleSlot(handle);
    if (slotId == 0 || slotId > pool->count) {
        return false;
    }
    if (pool->items[slotId].generation != handleSlotGen(handle)) {
        return false;
    }
    *outSlotId = slotId;
    return true;
}

bool texturePoolRemove(TexturePool *pool, Handle handle) {
    uint16_t slotId;
    if (!texturePoolResolve(pool, handle, &slotId)) {
        return false;
    }
    TextureSlot *slot = &pool->items[slotId];
    slot->generation += 1;
    slot->item = (Texture){0};
    slot->nextFree = pool->firstFree;
    pool->firstFree = slotId;
    return true;
}

// Texture *texturePoolGet(TexturePool *pool, Handle handle) {
//     uint16_t slotId;
//     if (!texturePoolResolve(pool, handle, &slotId)) {
//         return &pool->items[0];
//     }
//     return &pool->items[slotId];
// }

typedef struct GameApi {
    char *name;
    Umka *umka;
    bool active;

    TexturePool textures;

    UmkaFuncContext update;
    UmkaFuncContext init;
    UmkaFuncContext hotReload;
} GameApi;

// TODO: Refactor to using static array of GameSlot instead.
typedef struct GameSlot {
    GameApi game;
    uint32_t generation;
} GameSlot;

extern GameSlot games[MAX_GAMES];

void gamesInit() {
    for (int i = 0; i < MAX_GAMES; i++) {
        games[i] = (GameSlot){0};
        games[i].generation = 1;
    }
}

void initGame(GameApi *gameApi, char *name);

uint8_t gameAdd(char *name) {
    for (uint8_t i = 1; 1 < MAX_GAMES; i++) {
        if (games[i].game.umka != NULL)
            continue;

        GameApi game = {0};
        initGame(&game, name);
        texturePoolInit(&game.textures, i, games[i].generation);
        games[i].game = game;
        return i;
    }
    return 0;
}

static inline GameApi *gameFromRef(GameRef g) { GameApi *game = &games[g.id].game; }

GameSlot *slotFromUmka(Umka *umka) {
    for (uint8_t i = 1; i < MAX_GAMES; i++) {
        if (games[i].game.umka == umka) {
            return &games[i];
        }
    }
    return NULL;
}
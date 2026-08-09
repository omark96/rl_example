#pragma once
#include "umka_full.h"

#define H_TYPE_SHIFT 58
#define H_GAME_SHIFT 51
#define H_GGEN_SHIFT 47
#define H_SLOT_SHIFT 31
#define H_SGEN_SHIFT 0

#define H_TYPE_MASK 0x1FULL
#define H_GAME_MASK 0x7FULL
#define H_GGEN_MASK 0x0FULL
#define H_SLOT_MASK 0xFFFFULL
#define H_SGEN_MASK 0x7FFFFFFFULL

#define MAX_GAMES 128
#define HANDLE_NULL 0ULL

typedef struct GameApi {
    char *name;
    Umka *umka;
    bool active;
    UmkaFuncContext update;
    UmkaFuncContext init;
    UmkaFuncContext hotReload;
} GameApi;

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

static inline uint8_t handleGameId(Handle h) { return (h >> H_GAME_SHIFT) & H_GAME_MASK; }

static inline uint8_t handleGameGen(Handle h) { return (h >> H_GGEN_SHIFT) & H_GGEN_MASK; }

static inline uint16_t handleSlot(Handle h) { return (h >> H_SLOT_SHIFT) & H_SLOT_MASK; }

static inline uint32_t handleSlotGen(Handle h) { return h & H_SGEN_MASK; }

static inline Handle makeHandle(uint8_t type, uint8_t gameId, uint8_t gameGen, uint16_t slot,
                                uint32_t slotGen) {
    return ((Handle)(type & H_TYPE_MASK) << H_TYPE_SHIFT)
           | ((Handle)(gameId & H_GAME_MASK) << H_GAME_SHIFT)
           | ((Handle)(gameGen & H_GGEN_MASK) << H_GGEN_SHIFT)
           | ((Handle)(slot & H_SLOT_MASK) << H_SLOT_SHIFT) | (Handle)(slotGen & H_SGEN_MASK);
}

typedef struct {
    uint8_t id;
    uint8_t gen;
} GameRef;

static inline GameRef handleGameRef(Handle h) {
    return (GameRef){handleGameId(h), handleGameGen(h)};
}

static inline Handle makeGameHandle(GameRef g) { return makeHandle(RES_GAME, g.id, g.gen, 0, 0); }

extern GameApi *games[MAX_GAMES];

static inline GameApi *gameFromRef(GameRef g) { GameApi *game = games[g.id]; }

GameApi *gameFromUmka(Umka *umka) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i] == NULL) {
            continue;
        }
        if (games[i]->umka == umka) {
            return games[i];
        }
    }
    return NULL;
}
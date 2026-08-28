#pragma once
#include "handle.h"
#include "raylib.h"
#include "umka_full.h"
#include <stdlib.h>

#define MAX_GAMES 10
#define MAX_TEXTURES 256

#define T Texture
#define F_PREFIX texture
#define POOL_RES_TYPE RES_TEXTURE
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_TEXTURES
#include "pool.h"

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

void initGame(GameApi *gameApi, char *name, uint8_t slot, uint8_t gen);

uint8_t gameAdd(char *name) {
    for (uint8_t i = 1; 1 < MAX_GAMES; i++) {
        if (games[i].game.umka != NULL)
            continue;

        GameApi game = {0};
        initGame(&game, name, i, 1);
        texturePoolInit(&game.textures, i, games[i].generation);
        games[i].game = game;
        return i;
    }
    return 0;
}

// static inline GameApi *gameFromRef(GameRef g) { GameApi *game = &games[g.id].game; }

GameApi *gameFromUmka(Umka *umka) {
    for (uint8_t i = 0; i < MAX_GAMES; i++) {
        if (games[i].game.umka == umka) {
            return &games[i].game;
        }
    }
    return NULL;
}
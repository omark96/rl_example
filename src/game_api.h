#pragma once
#include "handle.h"
#include "raylib.h"
#include "umka_full.h"
#include <stdlib.h>

#define MAX_GAMES 10
#define MAX_TEXTURES 256
#define MAX_RENDER_TEXTURES 8

#define T Texture
#define F_PREFIX texture
#define POOL_RES_TYPE RES_TEXTURE
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_TEXTURES
#include "pool.h"

#define T RenderTexture2D
#define F_PREFIX renderTexture2D
#define POOL_RES_TYPE RES_RENDER_TEXTURE
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_RENDER_TEXTURES
#include "pool.h"

// TODO: Rewrite this into a single shared pool for each of the resources. Add a "screen"-field to
// the game struct with the default render texture of the game.

typedef struct Game {
    char *name;
    Umka *umka;
    bool active;
    long lastModified;

    TexturePool textures;
    RenderTexture2DPool renderTextures;

    UmkaFuncContext update;
    UmkaFuncContext init;
    UmkaFuncContext hotReload;
} Game;

#define T Game
#define F_PREFIX game
#define POOL_RES_TYPE RES_GAME
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_GAMES
#include "pool.h"

extern GamePool games;

void gamesInit() {
    for (int i = 0; i < MAX_GAMES; i++) {
        games.items[i] = (GameSlot){0};
        games.items[i].generation = 1;
    }
}

bool initUmka(Game *gameApi);

uint8_t gameAdd(char *name) {
    for (uint8_t i = 1; 1 < MAX_GAMES; i++) {
        if (games.items[i].item.umka != NULL)
            continue;

        Game game = {0};
        initUmka(&game);
        texturePoolInit(&game.textures, LoadTexture("defaultAssets/default_texture.png"), i,
                        games.items[i].generation);
        renderTexture2DPoolInit(&game.renderTextures,
                                LoadRenderTexture(GetScreenWidth(), GetScreenHeight()), i,
                                games.items[i].generation);
        games.items[i].item = game;
        return i;
    }
    return 0;
}

// static inline GameApi *gameFromRef(GameRef g) { GameApi *game = &games[g.id].game; }

Game *gameFromUmka(Umka *umka) {
    for (uint8_t i = 0; i < MAX_GAMES; i++) {
        if (games.items[i].item.umka == umka) {
            return &games.items[i].item;
        }
    }
    return NULL;
}
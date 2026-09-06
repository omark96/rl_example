#pragma once
#include "raylib.h"
#include "umka_full.h"
#include <stdlib.h>

typedef struct Handle {
    uint32_t slot;
    uint32_t generation;
} Handle;

#define NULL_HANDLE (Handle){.slot = 0, .generation = 0}

#define MAX_GAMES 10
#define MAX_TEXTURES 256
#define MAX_RENDER_TEXTURES 8

#define T Texture
#define F_PREFIX texture
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_TEXTURES
#include "pool.h"

#define T RenderTexture2D
#define F_PREFIX renderTexture2D
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_RENDER_TEXTURES
#include "pool.h"

typedef enum GameState {
    STATE_DISABLED,
    STATE_HIDDEN,
    STATE_IDLE,
    STATE_ENABLED,
    STATE_ACTIVE,
} GameState;

typedef struct Game {
    char *name;
    Umka *umka;
    GameState state;
    long lastModified;

    Handle screen;

    Handle parent;
    Handle children[16];
    uint8_t childCount;

    UmkaFuncContext init;
    UmkaFuncContext update;
    UmkaFuncContext draw;
    UmkaFuncContext input;
    UmkaFuncContext hotReload;
} Game;

#define T Game
#define F_PREFIX game
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_GAMES
#include "pool.h"

typedef struct GlobalResources {
    GamePool games;
    TexturePool textures;
    RenderTexture2DPool renderTextures;
} GlobalResources;

extern GlobalResources g_resources;

bool initUmka(Game *gameApi);
void drawGame(Game *game);
void updateGame(Game *game);

Game *gameFromUmka(Umka *umka) {
    GamePool games = g_resources.games;
    for (uint8_t i = 0; i <= games.liveCount; i++) {
        if (games.items[i].item.umka == umka) {
            return &games.items[i].item;
        }
    }
    return NULL;
}

Handle handleFromUmka(Umka *umka) {
    GamePool games = g_resources.games;
    for (uint8_t i = 0; i <= games.liveCount; i++) {
        GameSlot slot = games.items[i];
        if (slot.item.umka == umka) {
            return (Handle){.slot = i, .generation = slot.generation};
        }
    }
    return NULL_HANDLE;
}
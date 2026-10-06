#pragma once
#include "raylib.h"
#include "umka_api.h"
#include <stdint.h>
#include <stdlib.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef ptrdiff_t isize;
typedef size_t usize;
typedef uintptr_t uptr;

typedef struct Handle {
    u32 slot;
    u32 generation;
} Handle;

#define NULL_HANDLE (Handle){.slot = 0, .generation = 0}

#define MAX_GAMES 10
#define MAX_TEXTURES 256
#define MAX_RENDER_TEXTURES 8
#define MAX_MODELS 256
#define MAX_SDF_FONTS 256

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

#define T Model
#define F_PREFIX model
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_MODELS
#include "pool.h"

typedef struct SdfState SdfState;

typedef enum GameState {
    STATE_DISABLED,
    STATE_HIDDEN,
    STATE_IDLE,
    STATE_ENABLED,
    STATE_ACTIVE,
} GameState;

typedef struct Game {
    Handle handle;
    char *name;
    Umka *umka;
    GameState state;
    long lastModified;

    Handle screen;

    Handle parent;
    Handle children[16];
    u8 childCount;

    bool cursorDisabled;

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

typedef struct InputContext {
    u32 handledKeys[512];
    u32 handledMouseButtons[16];
    Vector2 mousePosition;
    f32 mouseWheelMove;
} InputContext;

typedef struct GlobalContext {
    GamePool games;
    TexturePool textures;
    RenderTexture2DPool renderTextures;
    ModelPool models;
    SdfState *sdf;

    InputContext inputs;

    Handle rootGame;
    Handle currentGame;

    f32 lastCheckedGames;
} GlobalContext;

extern GlobalContext g_ctx;

bool initUmka(Game *gameApi);
void drawGame(Handle handle);
void updateGame(Handle handle);

Game *getCurrentGame();
Handle getActiveGameHandle();
void setActiveGame(Handle handle);
void setGameState(Handle handle, GameState newState);
GameState getGameState(Handle handle);

Game *gameFromUmka(Umka *umka) {
    GamePool games = g_ctx.games;
    for (u32 i = 0; i <= games.count; i++) {
        if (games.items[i].item.umka == umka) {
            return &games.items[i].item;
        }
    }
    return NULL;
}

Handle handleFromUmka(Umka *umka) {
    GamePool games = g_ctx.games;
    for (u32 i = 0; i <= games.count; i++) {
        GameSlot slot = games.items[i];
        if (slot.item.umka == umka) {
            return (Handle){.slot = i, .generation = slot.generation};
        }
    }
    return NULL_HANDLE;
}

bool isValidAssetName(const char *name);
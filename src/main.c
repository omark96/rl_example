#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif // PLATFORM_WEB
#include "assert.h"
#include "game_api.c"
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"

GlobalContext g_ctx;

Handle gameHandles[MAX_GAMES];

float lastCheckedGames = 0;

void UpdateDrawFrame() {
    g_ctx.inputs = (InputContext){0};
    handleInput(g_ctx.rootGame);
    runGame(g_ctx.rootGame);

    BeginDrawing();
    drawGame(g_ctx.rootGame);
    Handle activeGameHandle = getActiveGameHandle();
    Game *activeGame = gamePoolGet(&g_ctx.games, activeGameHandle);
    RenderTexture2D *activeScreen
        = renderTexture2DPoolGet(&g_ctx.renderTextures, activeGame->screen);
    DrawTextureRec(activeScreen->texture,
                   (Rectangle){0, 0, activeScreen->texture.width, -activeScreen->texture.height},
                   (Vector2){0, 0}, WHITE);
    DrawFPS(20, 20);
    EndDrawing();

    lastCheckedGames += GetFrameTime();
    if (lastCheckedGames > 0.25) {
        checkForGameUpdates(&g_ctx.games);
        lastCheckedGames = 0;
    }
}

int main() {
    i32 gameCount = 0;

    const i32 screenWidth = 1920;
    const i32 screenHeight = 1080;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "_dev raylib basic window");
    SetTargetFPS(60);

    FilePathList gamePaths = LoadDirectoryFilesEx("games", "DIRS*", false);

    gameCount = gamePaths.count;

    texturePoolInit(&g_ctx.textures, LoadTexture("defaultAssets/default_texture.png"));
    renderTexture2DPoolInit(&g_ctx.renderTextures, (RenderTexture2D){0});
    gamePoolInit(&g_ctx.games, (Game){0});
    initSdf(screenWidth, screenHeight);

    for (i32 i = 0; i < gameCount; i++) {
        const char *gameName = GetFileName(gamePaths.paths[i]);
        Handle gameHandle = initGame(gameName);
        gameHandles[i] = gameHandle;
    }
    for (i32 i = 0; i < gameCount; i++) {
        Game *game = gamePoolGet(&g_ctx.games, gameHandles[i]);
        g_ctx.currentGame = game->handle;
        if (game->umka != NULL) {
            umkaCall(game->umka, &game->init);
        }
    }

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else  // PLATFORM_WEB
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
    CloseWindow();

    for (i32 i = 0; i < gameCount; i++) {
        freeGame(gamePoolGet(&g_ctx.games, gameHandles[i]));
    }
#endif // PLATFORM_WEB

    return 0;
}
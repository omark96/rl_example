#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif // PLATFORM_WEB
#include "assert.h"
#include "game_api.c"
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"

GlobalResources g_resources;

Handle gameHandles[MAX_GAMES];

Handle root;
float lastCheckedGames = 0;

void UpdateDrawFrame() {
    Game *rootGame = gamePoolGet(&g_resources.games, root);
    runGame(rootGame);

    BeginDrawing();
    drawGame(rootGame);
    RenderTexture2D *rootScreen
        = renderTexture2DPoolGet(&g_resources.renderTextures, rootGame->screen);
    DrawTextureRec(rootScreen->texture,
                   (Rectangle){0, 0, rootScreen->texture.width, -rootScreen->texture.height},
                   (Vector2){0, 0}, WHITE);
    EndDrawing();

    lastCheckedGames += GetFrameTime();
    if (lastCheckedGames > 0.25) {
        checkForGameUpdates(&g_resources.games);
        lastCheckedGames = 0;
    }
}

int main() {
    int gameCount = 0;

    Game game;

    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "_dev raylib basic window");
    SetTargetFPS(60);

    FilePathList gamePaths = LoadDirectoryFilesEx("games", "DIRS*", false);

    gameCount = gamePaths.count;
    printf("Number of games: %d\n", gamePaths.count);
    printf("First game: %s\n", gamePaths.paths[0] + 6);

    texturePoolInit(&g_resources.textures, LoadTexture("defaultAssets/default_texture.png"));

    for (int i = 0; i < gameCount; i++) {
        char *gameName = gamePaths.paths[i] + 6;
        gameHandles[i] = gamePoolAdd(&g_resources.games, (Game){0});
        Game *game = gamePoolGet(&g_resources.games, gameHandles[i]);
        initGame(game, gameName);
        RenderTexture2D renderTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        game->screen = renderTexture2DPoolAdd(&g_resources.renderTextures, renderTexture);
        texturePoolAdd(&g_resources.textures, renderTexture.texture);
        game->state = STATE_ENABLED;
        if (strcmp(gameName, "main") == 0) {
            game->state = STATE_ACTIVE;
            root = gameHandles[i];
        }
    }
    for (int i = 0; i < gameCount; i++) {
        Game *game = gamePoolGet(&g_resources.games, gameHandles[i]);
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

    for (int i = 0; i < gameCount; i++) {
        freeGame(gamePoolGet(&g_resources.games, gameHandles[i]));
    }
#endif // PLATFORM_WEB

    return 0;
}
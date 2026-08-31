#include "assert.h"
#include "game_api.c"
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"
#include "umka_full.h"

GamePool games;

Handle gameHandles[MAX_GAMES];

int main() {
    int gameCount = 0;
    int loadScreen = 0;
    int active = 0;

    Game game;

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "_dev raylib basic window");
    SetTargetFPS(60);

    FilePathList gamePaths = LoadDirectoryFilesEx("games", "DIRS*", false);

    gameCount = gamePaths.count;
    printf("Number of games: %d\n", gamePaths.count);
    printf("First game: %s\n", gamePaths.paths[0] + 6);

    for (int i = 0; i < gameCount; i++) {
        char *gameName = gamePaths.paths[i] + 6;
        if (strcmp(gameName, "loadScreen") == 0) {
            loadScreen = i;
            active = loadScreen;
        }
        Game game = {};
        bool initOk = initGame(&game, gameName);
        if (initOk) {
            umkaCall(game.umka, &game.init);
        }
        gameHandles[i] = gamePoolAdd(&games, game);
        Game *stored = gamePoolGet(&games, gameHandles[i]);
        uint16_t gameSlot = handleSlot(gameHandles[i]);
        uint32_t gameGen = handleSlotGen(gameHandles[i]);
        texturePoolInit(&stored->textures, LoadTexture("defaultAssets/default_texture.png"),
                        gameSlot, gameGen);
        renderTexturePoolInit(&game.renderTextures,
                              LoadRenderTexture(GetScreenWidth(), GetScreenHeight()), gameSlot,
                              gameGen);
    }

    float lastCheckedGames = 0;

    while (!WindowShouldClose()) {

        BeginDrawing();
        ClearBackground(GRAY);
        if (active >= 0) {
            Game *game = gamePoolGet(&games, gameHandles[active]);
            if (game->umka != NULL) {
                umkaCall(game->umka, &game->update);
            } else {
                DrawText(TextFormat("Invalid game: %s", game->name), 200, 200, 40, WHITE);
            }
            if (IsKeyPressed(KEY_F5)) {
                reloadGame(game, 0);
            } else if (IsKeyPressed(KEY_P)) {
                active = -1;
            }
        } else {
            for (int i = 0; i < gameCount; i++) {
                DrawText(TextFormat("%d: %s", i + 1, gamePoolGet(&games, gameHandles[i])->name), 50,
                         50 + 25 * i, 20, BLACK);
            }
            if (IsKeyPressed(KEY_ONE)) {
                active = 0;
            } else if (IsKeyPressed(KEY_TWO)) {
                active = 1;
            } else if (IsKeyPressed(KEY_THREE)) {
                active = 2;
            } else if (IsKeyPressed(KEY_FOUR)) {
                active = 3;
            } else if (IsKeyPressed(KEY_FIVE)) {
                active = 4;
            }
        }
        EndDrawing();

        lastCheckedGames += GetFrameTime();
        if (lastCheckedGames > 0.25) {
            checkForGameUpdates(&games);
            lastCheckedGames = 0;
        }
    }
    CloseWindow();

    for (int i = 0; i < gameCount; i++) {
        freeGame(gamePoolGet(&games, gameHandles[i]));
    }

    return 0;
}
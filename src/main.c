#include "assert.h"
#include "game_api.c"
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"
#include "umka_full.h"

GameSlot games[MAX_GAMES];

int main() {
    int gameCount = 0;
    int loadScreen = 0;
    int active = 0;

    GameApi game;

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
        games[i].game = (GameApi){0};
        initGame(&games[i].game, gamePaths.paths[i] + 6);
        umkaCall(games[i].game.umka, &games[i].game.init);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GRAY);
        if (active >= 0) {
            game = games[active].game;
            umkaCall(game.umka, &game.update);
            if (IsKeyPressed(KEY_F5)) {
                GameApi next;
                initGame(&next, game.name);
                hotReload(&game, &next);
                freeGame(&game);
                games[active].game = next;
            } else if (IsKeyPressed(KEY_P)) {
                active = -1;
            }
        } else {
            for (int i = 0; i < gameCount; i++) {
                DrawText(TextFormat("%d: %s", i + 1, games[i].game.name), 50, 50 + 25 * i, 20,
                         BLACK);
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
    }
    CloseWindow();

    for (int i = 0; i < gameCount; i++) {
        freeGame(&games[i].game);
    }

    return 0;
}
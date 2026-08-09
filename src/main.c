#include "assert.h"
#include "game_api.c"
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"
#include "umka_full.h"

GameApi *games[MAX_GAMES];

int gameCount = 0;
int loadScreen = 0;
int active = 0;

int main() {
    GameApi game;

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "_dev raylib basic window");
    SetTargetFPS(60);

    FilePathList gamePaths = LoadDirectoryFilesEx("games", "DIRS*", false);

    gameCount = gamePaths.count;
    printf("Number of games: %d\n", gamePaths.count);
    printf("First game: %s", gamePaths.paths[0] + 6);

    for (int i = 0; i < gameCount; i++) {
        char *gameName = gamePaths.paths[i] + 6;
        if (gameName == "loadScreen") {
            loadScreen = i;
            active = loadScreen;
        }

        games[i] = malloc(sizeof(GameApi));
        initGame(games[i], gamePaths.paths[i] + 6);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GRAY);
        if (active >= 0) {
            game = *games[active];
            umkaCall(game.umka, &game.update);
            if (IsKeyPressed(KEY_F5)) {
                GameApi next;
                initGame(&next, game.name);
                hotReload(&game, &next);
                freeGame(&game);
                *games[active] = next;
            } else if (IsKeyPressed(KEY_P)) {
                active = -1;
            }
        } else {
            for (int i = 0; i < gameCount; i++) {
                DrawText(TextFormat("%d: %s", i + 1, games[i]->name), 50, 50 + 25 * i, 20, BLACK);
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
        freeGame(games[i]);
    }

    return 0;
}
#include "assert.h"
#include "game_api.c"
#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"
#include "umka_full.h"

GlobalResources g_resources;

Handle gameHandles[MAX_GAMES];

int main() {
    int gameCount = 0;

    Game game;

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "_dev raylib basic window");
    SetTargetFPS(60);

    FilePathList gamePaths = LoadDirectoryFilesEx("games", "DIRS*", false);

    gameCount = gamePaths.count;
    printf("Number of games: %d\n", gamePaths.count);
    printf("First game: %s\n", gamePaths.paths[0] + 6);

    texturePoolInit(&g_resources.textures, LoadTexture("defaultAssets/default_texture.png"));

    for (int i = 0; i < gameCount; i++) {
        char *gameName = gamePaths.paths[i] + 6;
        Game game = {};
        bool initOk = initGame(&game, gameName);
        if (initOk) {
            umkaCall(game.umka, &game.init);
        }
        game.screen = renderTexture2DPoolAdd(
            &g_resources.renderTextures, LoadRenderTexture(GetScreenWidth(), GetScreenHeight()));
        gameHandles[i] = gamePoolAdd(&g_resources.games, game);
        Game *stored = gamePoolGet(&g_resources.games, gameHandles[i]);
    }

    float lastCheckedGames = 0;
    Camera3D camera = {0};
    camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool cameraEnabled = false;

    while (!WindowShouldClose()) {
        int game_to_toggle = -1;
        if (cameraEnabled) {
            UpdateCamera(&camera, CAMERA_FREE);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            cameraEnabled = true;
            DisableCursor();
        }
        if (IsKeyPressed(KEY_C)) {
            cameraEnabled = false;
            EnableCursor();
        }
        BeginDrawing();
        ClearBackground(GRAY);

        if (IsKeyPressed(KEY_ONE)) {
            game_to_toggle = 0;
        } else if (IsKeyPressed(KEY_TWO)) {
            game_to_toggle = 1;
        } else if (IsKeyPressed(KEY_THREE)) {
            game_to_toggle = 2;
        } else if (IsKeyPressed(KEY_FOUR)) {
            game_to_toggle = 3;
        } else if (IsKeyPressed(KEY_FIVE)) {
            game_to_toggle = 4;
        }

        for (size_t i = 0; i < gameCount; i++) {
            Game *game = gamePoolGet(&g_resources.games, gameHandles[i]);
            if (i == game_to_toggle) {
                game->active = !game->active;
            }
            if (!game->active) {
                continue;
            }
            RenderTexture2D renderTexture
                = *renderTexture2DPoolGet(&g_resources.renderTextures, game->screen);
            BeginTextureMode(renderTexture);
            ClearBackground(WHITE);
            if (game->umka != NULL) {
                umkaCall(game->umka, &game->update);
            } else {
                DrawText(TextFormat("Invalid game: %s", game->name), 200, 200, 40, WHITE);
            }
            EndTextureMode();
            BeginMode3D(camera);
            DrawTexQuad(renderTexture.texture, (Vector3){5.0f * i, 1.5f, 0}, (Vector3){1.0f, 0, 0},
                        (Vector3){0, 0.5625f, 0}, true, WHITE);
            EndMode3D();
        }

        EndDrawing();

        lastCheckedGames += GetFrameTime();
        if (lastCheckedGames > 0.25) {
            checkForGameUpdates(&g_resources.games);
            lastCheckedGames = 0;
        }
    }
    CloseWindow();

    for (int i = 0; i < gameCount; i++) {
        freeGame(gamePoolGet(&g_resources.games, gameHandles[i]));
    }

    return 0;
}
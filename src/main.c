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
    Handle root;
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
        if (IsKeyPressed(KEY_V)) {
            printf("Pressed v one time\n");
        }
        if (IsKeyPressed(KEY_V)) {
            printf("Pressed v two times\n");
        }
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
    CloseWindow();

    for (int i = 0; i < gameCount; i++) {
        freeGame(gamePoolGet(&g_resources.games, gameHandles[i]));
    }

    return 0;
}
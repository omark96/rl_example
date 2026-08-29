#include "game_api.h"
#include "raylib.h"

void gfxDrawText(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *text = (const char *)umkaGetParam(params, 0)->ptrVal;
    int posX = umkaGetParam(params, 1)->intVal;
    int posY = umkaGetParam(params, 2)->intVal;
    int fontSize = umkaGetParam(params, 3)->intVal;
    Color *color = (Color *)umkaGetParam(params, 4);
    DrawText(text, posX, posY, fontSize, *color);
}

void gfxDrawRectangle(UmkaStackSlot *params, UmkaStackSlot *result) {
    int posX = umkaGetParam(params, 0)->intVal;
    int posY = umkaGetParam(params, 1)->intVal;
    int width = umkaGetParam(params, 2)->intVal;
    int height = umkaGetParam(params, 3)->intVal;
    Color *color = (Color *)umkaGetParam(params, 4);
    DrawRectangle(posX, posY, width, height, *color);
}

void gfxLoadTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    Game *game = gameFromUmka(umka);
    const char *fileName = (const char *)umkaGetParam(params, 0)->ptrVal;
    Texture texture = LoadTexture(fileName);
    Handle handle = texturePoolAdd(&game->textures, texture);
    umkaGetResult(params, result)->intVal = handle;
    printf("New texture handle: %llu\n", handle);
}

void gfxUnloadTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    printf("unloaded\n");
    Umka *umka = umkaGetInstance(result);
    Game *game = gameFromUmka(umka);
    Handle textureHandle = umkaGetParam(params, 0)->intVal;
    Texture *texture = texturePoolGet(&game->textures, textureHandle);
    if (IsTextureValid(*texture)) {
        UnloadTexture(*texture);
    }
    texturePoolRemove(&game->textures, textureHandle);
}

void gfxDrawTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    Game *game = gameFromUmka(umka);
    Handle textureHandle = umkaGetParam(params, 0)->intVal;
    int32_t x = umkaGetParam(params, 1)->intVal;
    int32_t y = umkaGetParam(params, 2)->intVal;
    Color *color = (Color *)umkaGetParam(params, 3);
    Texture *texture = texturePoolGet(&game->textures, textureHandle);
    DrawTexture(*texture, x, y, *color);
}

void gfxAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "drawText", &gfxDrawText);
    umkaAddFunc(umka, "drawRectangle", &gfxDrawRectangle);
    umkaAddFunc(umka, "loadTexture", &gfxLoadTexture);
    umkaAddFunc(umka, "unloadTexture", &gfxUnloadTexture);
    umkaAddFunc(umka, "drawTexture", &gfxDrawTexture);

    const char *umSourceNames[] = {"gfx.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "gfx.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
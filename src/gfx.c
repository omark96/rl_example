#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

void DrawTexQuad(Texture2D tex, Vector3 pos, Vector3 right, Vector3 up, bool flipY, Color tint) {
    Vector3 tl = Vector3Add(Vector3Subtract(pos, right), up);
    Vector3 bl = Vector3Subtract(Vector3Subtract(pos, right), up);
    Vector3 br = Vector3Subtract(Vector3Add(pos, right), up);
    Vector3 tr = Vector3Add(Vector3Add(pos, right), up);

    float t0 = flipY ? 1.0f : 0.0f;
    float t1 = flipY ? 0.0f : 1.0f;

    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    Vector3 n = Vector3Normalize(Vector3CrossProduct(right, up));
    rlNormal3f(n.x, n.y, n.z);
    rlTexCoord2f(0, t0);
    rlVertex3f(tl.x, tl.y, tl.z);
    rlTexCoord2f(0, t1);
    rlVertex3f(bl.x, bl.y, bl.z);
    rlTexCoord2f(1, t1);
    rlVertex3f(br.x, br.y, br.z);
    rlTexCoord2f(1, t0);
    rlVertex3f(tr.x, tr.y, tr.z);
    rlEnd();
    rlSetTexture(0);
}

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
    Handle handle;
    if (IsTextureValid(texture)) {
        handle = texturePoolAdd(&game->textures, texture);
    } else {
        handle = HANDLE_NULL;
    }
    *(Handle *)umkaGetResult(params, result)->ptrVal = handle;
    printf("New texture handle: %llu\n", handle);
}

void gfxUnloadTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    printf("unloaded\n");
    Umka *umka = umkaGetInstance(result);
    Game *game = gameFromUmka(umka);
    Handle textureHandle = *(Handle *)umkaGetParam(params, 0);
    Texture *texture = texturePoolGet(&game->textures, textureHandle);
    if (!texturePoolRemove(&game->textures, textureHandle)) {
        return;
    }
    if (IsTextureValid(*texture)) {
        UnloadTexture(*texture);
    }
}

void gfxDrawTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    Game *game = gameFromUmka(umka);
    Handle textureHandle = *(Handle *)umkaGetParam(params, 0);
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
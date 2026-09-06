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

void gfxBeginMode3D(UmkaStackSlot *params, UmkaStackSlot *result) {
    Camera3D *camera = (Camera3D *)umkaGetParam(params, 0);
    BeginMode3D(*camera);
}

void gfxEndMode3D(UmkaStackSlot *params, UmkaStackSlot *result) { EndMode3D(); }

void gfxUpdateCamera(UmkaStackSlot *params, UmkaStackSlot *result) {
    Camera3D *camera = umkaGetParam(params, 0)->ptrVal;
    CameraMode cameraMode = umkaGetParam(params, 1)->intVal;
    UpdateCamera(camera, cameraMode);
}

void gfxDrawTexQuad(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle *textureHandle = (Handle *)umkaGetParam(params, 0);
    Vector3 *pos = (Vector3 *)umkaGetParam(params, 1);
    Vector3 *right = (Vector3 *)umkaGetParam(params, 2);
    Vector3 *up = (Vector3 *)umkaGetParam(params, 3);
    bool flipY = umkaGetParam(params, 4)->intVal;
    Color *tint = (Color *)umkaGetParam(params, 5);

    Texture2D *texture = texturePoolGet(&g_resources.textures, *textureHandle);

    DrawTexQuad(*texture, *pos, *right, *up, flipY, *tint);
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

void gfxClearBackground(UmkaStackSlot *params, UmkaStackSlot *result) {
    Color *color = (Color *)umkaGetParam(params, 0);
    ClearBackground(*color);
}

void gfxLoadTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *fileName = (const char *)umkaGetParam(params, 0)->ptrVal;
    Texture texture = LoadTexture(fileName);
    Handle handle;
    if (IsTextureValid(texture)) {
        handle = texturePoolAdd(&g_resources.textures, texture);
    } else {
        handle = NULL_HANDLE;
    }
    *(Handle *)umkaGetResult(params, result)->ptrVal = handle;
    printf("New texture handle: %llu\n", handle);
}

void gfxUnloadTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle textureHandle = *(Handle *)umkaGetParam(params, 0);
    Texture *texture = texturePoolGet(&g_resources.textures, textureHandle);
    if (!texturePoolRemove(&g_resources.textures, textureHandle)) {
        return;
    }
    if (IsTextureValid(*texture)) {
        UnloadTexture(*texture);
    }
}

void gfxDrawTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle textureHandle = *(Handle *)umkaGetParam(params, 0);
    int32_t x = umkaGetParam(params, 1)->intVal;
    int32_t y = umkaGetParam(params, 2)->intVal;
    Color *color = (Color *)umkaGetParam(params, 3);
    Texture *texture = texturePoolGet(&g_resources.textures, textureHandle);
    DrawTexture(*texture, x, y, *color);
}

void gfxGetGameScreen(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle gameHandle = *(Handle *)umkaGetParam(params, 0);
    Game *game = gamePoolGet(&g_resources.games, gameHandle);
    *(Handle *)umkaGetResult(params, result)->ptrVal = game->screen;
}

void gfxGetGameScreenTexture(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle renderTextureHandle = *(Handle *)umkaGetParam(params, 0);
    uint32_t textureId
        = renderTexture2DPoolGet(&g_resources.renderTextures, renderTextureHandle)->texture.id;
    Handle textureHandle = {0};
    for (int i = 0; i < g_resources.textures.count; i++) {
        TextureSlot slot = g_resources.textures.items[i];
        Texture texture = slot.item;
        if (texture.id == textureId) {
            textureHandle.slot = i;
            textureHandle.generation = slot.generation;
        }
    }
    *(Handle *)umkaGetResult(params, result)->ptrVal = textureHandle;
}

void gfxAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "drawText", &gfxDrawText);
    umkaAddFunc(umka, "drawRectangle", &gfxDrawRectangle);
    umkaAddFunc(umka, "loadTexture", &gfxLoadTexture);
    umkaAddFunc(umka, "unloadTexture", &gfxUnloadTexture);
    umkaAddFunc(umka, "drawTexture", &gfxDrawTexture);
    umkaAddFunc(umka, "drawTexQuad", &gfxDrawTexQuad);
    umkaAddFunc(umka, "getGameScreen", &gfxGetGameScreen);
    umkaAddFunc(umka, "getGameScreenTexture", &gfxGetGameScreenTexture);
    umkaAddFunc(umka, "endMode3D", &gfxEndMode3D);
    umkaAddFunc(umka, "beginMode3D", &gfxBeginMode3D);
    umkaAddFunc(umka, "updateCamera", &gfxUpdateCamera);
    umkaAddFunc(umka, "clearBackground", &gfxClearBackground);

    const char *umSourceNames[] = {"gfx.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "gfx.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
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

void gfxAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "drawText", &gfxDrawText);
    umkaAddFunc(umka, "drawRectangle", &gfxDrawRectangle);

    const char *umSourceNames[] = {"gfx.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "gfx.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
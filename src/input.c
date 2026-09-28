#include "game_api.h"
#include "raylib.h"

void inputIsMouseButtonPressed(UmkaStackSlot *params, UmkaStackSlot *result) {
    MouseButton button = umkaGetParam(params, 0)->intVal;
    bool pressed = IsMouseButtonPressed(button);
    umkaGetResult(params, result)->intVal = pressed;
}

void inputIsKeyPressed(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle gameHandle = getCurrentGame()->handle;

    KeyboardKey key = umkaGetParam(params, 0)->intVal;
    bool pressed = IsKeyPressed(key);
    uint32_t *inputSlot = &g_ctx.inputs.handledKeys[key];
    if (*inputSlot == 0) {
        *inputSlot = gameHandle.slot;
    } else if (*inputSlot != gameHandle.slot) {
        pressed = false;
    }

    umkaGetResult(params, result)->intVal = pressed;
}

void inputGetMouseX(UmkaStackSlot *params, UmkaStackSlot *result) {
    int x = GetMouseX();
    umkaGetResult(params, result)->intVal = x;
}

void inputGetMouseY(UmkaStackSlot *params, UmkaStackSlot *result) {
    int y = GetMouseY();
    umkaGetResult(params, result)->intVal = y;
}

void inputDisableCursor(UmkaStackSlot *params, UmkaStackSlot *result) {
    if (!IsCursorHidden()) {
        DisableCursor();
    }
}

void inputEnableCursor(UmkaStackSlot *params, UmkaStackSlot *result) {
    if (IsCursorHidden()) {
        EnableCursor();
    }
}
void inputAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "getMouseX", &inputGetMouseX);
    umkaAddFunc(umka, "getMouseY", &inputGetMouseY);
    umkaAddFunc(umka, "isMouseButtonPressed", &inputIsMouseButtonPressed);
    umkaAddFunc(umka, "isKeyPressed", &inputIsKeyPressed);
    umkaAddFunc(umka, "disableCursor", &inputDisableCursor);
    umkaAddFunc(umka, "enableCursor", &inputEnableCursor);

    const char *umSourceNames[] = {"input.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "input.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}

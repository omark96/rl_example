#include "game_api.h"
#include "raylib.h"

void inputIsMouseButtonPressed(UmkaStackSlot *params, UmkaStackSlot *result) {
    MouseButton button = umkaGetParam(params, 0)->intVal;
    bool pressed = IsMouseButtonPressed(button);
    umkaGetResult(params, result)->intVal = pressed;
}

void inputIsMouseButtonDown(UmkaStackSlot *params, UmkaStackSlot *result) {
    MouseButton button = umkaGetParam(params, 0)->intVal;
    bool pressed = IsMouseButtonDown(button);
    umkaGetResult(params, result)->intVal = pressed;
}

void inputIsKeyPressed(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle gameHandle = getCurrentGame()->handle;

    KeyboardKey key = umkaGetParam(params, 0)->intVal;
    bool pressed = IsKeyPressed(key);
    u32 *inputSlot = &g_ctx.inputs.handledKeys[key];
    if (*inputSlot == 0) {
        *inputSlot = gameHandle.slot;
    } else if (*inputSlot != gameHandle.slot) {
        pressed = false;
    }

    umkaGetResult(params, result)->intVal = pressed;
}

void inputGetMouseX(UmkaStackSlot *params, UmkaStackSlot *result) {
    i32 x = GetMouseX();
    umkaGetResult(params, result)->intVal = x;
}

void inputGetMouseY(UmkaStackSlot *params, UmkaStackSlot *result) {
    i32 y = GetMouseY();
    umkaGetResult(params, result)->intVal = y;
}

void inputGetMouseWheelMove(UmkaStackSlot *params, UmkaStackSlot *result) {
    f32 d = GetMouseWheelMove();
    umkaGetResult(params, result)->realVal = (double)d;
}

void inputGetMouseDelta(UmkaStackSlot *params, UmkaStackSlot *result) {
    Vector2 d = GetMouseDelta();
    *(Vector2 *)umkaGetResult(params, result)->ptrVal = d;
}

void inputDisableCursor(UmkaStackSlot *params, UmkaStackSlot *result) {
    if (!getCurrentGame()->cursorDisabled) {
        DisableCursor();
        getCurrentGame()->cursorDisabled = true;
    }
}

void inputEnableCursor(UmkaStackSlot *params, UmkaStackSlot *result) {
    if (getCurrentGame()->cursorDisabled) {
        EnableCursor();
        getCurrentGame()->cursorDisabled = false;
    }
}
void inputAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "getMouseX", &inputGetMouseX);
    umkaAddFunc(umka, "getMouseY", &inputGetMouseY);
    umkaAddFunc(umka, "isMouseButtonPressed", &inputIsMouseButtonPressed);
    umkaAddFunc(umka, "isMouseButtonDown", &inputIsMouseButtonDown);
    umkaAddFunc(umka, "isKeyPressed", &inputIsKeyPressed);
    umkaAddFunc(umka, "disableCursor", &inputDisableCursor);
    umkaAddFunc(umka, "enableCursor", &inputEnableCursor);
    umkaAddFunc(umka, "getMouseDelta", &inputGetMouseDelta);
    umkaAddFunc(umka, "getMouseWheelMove", &inputGetMouseWheelMove);

    const char *umSourceNames[] = {"input.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "input.um"
        , '\0'}};
    for (usize i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}

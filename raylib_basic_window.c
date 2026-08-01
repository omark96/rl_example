#include "stdio.h"
#include "stdlib.h"
#include "raylib.h"
#include "umka_types.h"
#include "umka_api.h"
#include "umka_vm.h"
#include "umka_common.h"
#include "umka_runtime.h"
#include "umka_compiler.h"

void rlDrawText(UmkaStackSlot *params, UmkaStackSlot *result)
{
    const char *text = (const char *)umkaGetParam(params, 0)->ptrVal;
    int posX = umkaGetParam(params, 1)->intVal;
    int posY = umkaGetParam(params, 2)->intVal;
    int fontSize = umkaGetParam(params, 3)->intVal;
    Color *color = (Color *)umkaGetParam(params, 4);
    DrawText(text, posX, posY, fontSize, *color);
}

void rlDrawRectangle(UmkaStackSlot *params, UmkaStackSlot *result)
{
    int posX = umkaGetParam(params, 0)->intVal;
    int posY = umkaGetParam(params, 1)->intVal;
    int width = umkaGetParam(params, 2)->intVal;
    int height = umkaGetParam(params, 3)->intVal;
    Color *color = (Color *)umkaGetParam(params, 4);
    DrawRectangle(posX, posY, width, height, *color);
}

void rlIsMouseButtonPressed(UmkaStackSlot *params, UmkaStackSlot *result)
{
    MouseButton button = umkaGetParam(params, 0)->intVal;
    bool pressed = IsMouseButtonPressed(button);
    umkaGetResult(params, result)->intVal = pressed;
}

void rlGetMouseX(UmkaStackSlot *params, UmkaStackSlot *result)
{
    int x = GetMouseX();
    umkaGetResult(params, result)->intVal = x;
}

void rlGetMouseY(UmkaStackSlot *params, UmkaStackSlot *result)
{
    int y = GetMouseY();
    umkaGetResult(params, result)->intVal = y;
}

void corePrint(UmkaStackSlot *params, UmkaStackSlot *result)
{
    const char *msg = (const char *)umkaGetParam(params, 0)->ptrVal;
    puts(msg);
}

void gameSave(UmkaStackSlot *params, UmkaStackSlot *result)
{
}

void gameLoad(UmkaStackSlot *params, UmkaStackSlot *result)
{
}

typedef struct GameApi
{
    Umka *umka;
    UmkaFuncContext update;
    UmkaFuncContext init;
    UmkaFuncContext serialize;
    UmkaFuncContext deserialize;
    UmkaFuncContext registerState;
} GameApi;

GameApi initGame(char *name)
{
    GameApi gameApi = {0};
    gameApi.umka = umkaAlloc();
    const UmkaType *stateType;

    bool umkaOk = umkaInit(gameApi.umka, name, NULL, 1024 * 1024, NULL, 0, NULL, false, false, NULL);
    if (umkaOk)
    {
        umkaAddFunc(gameApi.umka, "drawText", &rlDrawText);
        umkaAddFunc(gameApi.umka, "drawRectangle", &rlDrawRectangle);
        umkaAddFunc(gameApi.umka, "getMouseX", &rlGetMouseX);
        umkaAddFunc(gameApi.umka, "getMouseY", &rlGetMouseY);
        umkaAddFunc(gameApi.umka, "isMouseButtonPressed", &rlIsMouseButtonPressed);
        umkaAddFunc(gameApi.umka, "print", &corePrint);
        const char *umSourceNames[] = {"rl.um", "core.um"};
        const char *umSourceFiles[] = {
            (const char[]){
#embed "rl.um"
                , '\0'},
            (const char[]){
#embed "core.um"
                , '\0'}};
        for (int i = 0; i < 2; i++)
        {
            umkaAddModule(gameApi.umka, umSourceNames[i], umSourceFiles[i]);
        }
        umkaOk = umkaCompile(gameApi.umka);

        // umkaGetFunc(gameApi.umka, NULL, "registerState", &gameApi.registerState);
        // stateType = umkaGetParamType(gameApi.registerState.params, 0);
        // for (int i = 0; i < stateType->numItems; i++)
        // {
        //     Field *field = stateType->field[i];
        //     int a = 1;
        // }
    }

    if (umkaOk)
    {
        printf("Umka initialized\n");
        umkaGetFunc(gameApi.umka, NULL, "update", &gameApi.update);
        umkaGetFunc(gameApi.umka, NULL, "init", &gameApi.init);
        umkaGetFunc(gameApi.umka, NULL, "serialize", &gameApi.serialize);
        umkaGetFunc(gameApi.umka, NULL, "deserialize", &gameApi.deserialize);
    }
    else
    {
        UmkaError *error = umkaGetError(gameApi.umka);
        printf("Umka error %s (%d, %d): %s\n", error->fileName, error->line, error->pos, error->msg);
    }
    return gameApi;
}
void onWarning(UmkaError *err)
{
    fprintf(stderr, "%s (%s:%d): %s\n", err->fnName, err->fileName, err->line, err->msg);
}

int main()
{
    GameApi game = initGame("example.um");

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "_dev raylib basic window");
    SetTargetFPS(60);
    umkaCall(game.umka, &game.init);
    // umkaCall(game.umka, &game.serialize);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(GRAY);
        umkaCall(game.umka, &game.update);
        EndDrawing();

        if (IsKeyPressed(KEY_F5))
        {
            typedef UmkaDynArray(uint8_t) ByteArray;
            ByteArray out = {0};
            umkaGetResult(game.serialize.params, game.serialize.result)->ptrVal = &out;
            umkaCall(game.umka, &game.serialize);
            int length = umkaGetDynArrayLen(&out);

            uint8_t *gameState = malloc(length * out.itemSize);
            memcpy(gameState, out.data, length * out.itemSize);

            umkaDecRef(game.umka, out.data);

            umkaFree(game.umka);
            game = initGame("example.um");
            umkaCall(game.umka, &game.init);

            ByteArray *in = (ByteArray *)umkaGetParam(game.deserialize.params, 0);
            const UmkaType *arrayType = umkaGetParamType(game.deserialize.params, 0);

            umkaMakeDynArray(game.umka, in, arrayType, length);
            memcpy(in->data, gameState, length * in->itemSize);

            if (umkaCall(game.umka, &game.deserialize) != 0)
            {
                UmkaError *err = umkaGetError(game.umka);
            }

            free(gameState);
        }
    }
    CloseWindow();

    umkaFree(game.umka);

    return 0;
}

/*
generate_serializer(name, type, buf):
    if basic type:
        generate how to serialize
    if array:
        write len
        write for loop
        generate_serializer(base_type, buf)
    if struct



    type State = struct {
    a: int32;
    b: bool;
}

var state: State

fn serialize(state: State): []uint8 {
    var data: []uint8

    var buf: [8]uint8
    a := state.a
    for i := 0; i < 4; i += 1 {
        buf[3 - i] = a & 0xFF
        a = a >> 8
    }
    for i := 0; i < 4; i += 1 {
        data = append(data, buf[i])
    }
    if state.b {
        data = append(data, 1)
    } else {
        data = append(data, 0)
    }
    printf("%v\n", data)
    return data
}

fn deserialize(data: []uint8) : State {
    state := State{}
    count := 0
    for i := 0; i < 4; i++ {
        state.a = state.a << 8
        state.a = state.a + data[count + i]
    }
    count += 4
    state.b = bool(data[count])
    count++
    return state
}


fn main() {
    state.a = (1 << 8) + (1 << 6) + 1
    state.b = true
    data := serialize(state)
    newState := deserialize(data)
    printf("%v\n%v\n", state, newState)
}
*/
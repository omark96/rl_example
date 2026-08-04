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

typedef struct GameApi
{
    Umka *umka;
    UmkaFuncContext update;
    UmkaFuncContext init;
    UmkaFuncContext hotReload;
} GameApi;

void initGame(GameApi *gameApi, char *name)
{
    gameApi->umka = umkaAlloc();
    const UmkaType *stateType;

    bool umkaOk = umkaInit(gameApi->umka, name, NULL, 1024 * 1024, NULL, 0, NULL, false, false, NULL);
    if (umkaOk)
    {
        umkaAddFunc(gameApi->umka, "drawText", &rlDrawText);
        umkaAddFunc(gameApi->umka, "drawRectangle", &rlDrawRectangle);
        umkaAddFunc(gameApi->umka, "getMouseX", &rlGetMouseX);
        umkaAddFunc(gameApi->umka, "getMouseY", &rlGetMouseY);
        umkaAddFunc(gameApi->umka, "isMouseButtonPressed", &rlIsMouseButtonPressed);
        umkaAddFunc(gameApi->umka, "print", &corePrint);
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
            umkaAddModule(gameApi->umka, umSourceNames[i], umSourceFiles[i]);
        }
        umkaOk = umkaCompile(gameApi->umka);

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
        umkaGetFunc(gameApi->umka, NULL, "update", &gameApi->update);
        umkaGetFunc(gameApi->umka, NULL, "init", &gameApi->init);
        umkaGetFunc(gameApi->umka, NULL, "hotReload", &gameApi->hotReload);
    }
    else
    {
        UmkaError *error = umkaGetError(gameApi->umka);
        printf("Umka error %s (%d, %d): %s\n", error->fileName, error->line, error->pos, error->msg);
    }

    umkaCall(gameApi->umka, &gameApi->init);
}
void onWarning(UmkaError *err)
{
    fprintf(stderr, "%s (%s:%d): %s\n", err->fnName, err->fileName, err->line, err->msg);
}

void freeGame(GameApi *game)
{
    umkaFree(game->umka);
    *game = (GameApi){0};
}

void transfer(Umka *dstUmka, void *dst, const UmkaType *dstType, Umka *srcUmka, void *src, const UmkaType *srcType);

static MapNode *transferMapNode(Umka *dstUmka, const UmkaType *dstKeyType, const UmkaType *dstItemType,
                                Umka *srcUmka, MapNode *srcNode, const UmkaType *srcKeyType, const UmkaType *srcItemType)
{
    if (!srcNode)
    {
        return NULL;
    }

    MapNode *dstNode = (MapNode *)umkaAllocData(dstUmka, sizeof(MapNode), NULL);
    dstNode->len = srcNode->len;
    dstNode->priority = srcNode->priority;

    transfer(dstUmka, &dstNode->key, dstKeyType,
             srcUmka, &srcNode->key, srcKeyType);

    transfer(dstUmka, &dstNode->data, dstItemType,
             srcUmka, &srcNode->data, srcItemType);

    dstNode->left = transferMapNode(dstUmka, dstKeyType, dstItemType,
                                    srcUmka, srcNode->left, srcKeyType, srcItemType);
    dstNode->right = transferMapNode(dstUmka, dstKeyType, dstItemType,
                                     srcUmka, srcNode->right, srcKeyType, srcItemType);

    return dstNode;
}

void transfer(Umka *dstUmka, void *dst, const UmkaType *dstType, Umka *srcUmka, void *src, const UmkaType *srcType)
{
    if (srcType->kind != dstType->kind)
        return;

    switch (srcType->kind)
    {
    case TYPE_BOOL:
    case TYPE_CHAR:
    case TYPE_INT8:
    case TYPE_INT16:
    case TYPE_INT32:
    case TYPE_INT:
    case TYPE_REAL32:
    case TYPE_REAL:
    case TYPE_UINT8:
    case TYPE_UINT16:
    case TYPE_UINT32:
    case TYPE_UINT:
    {
        memcpy(dst, src, srcType->size);
        break;
    }
    case TYPE_STRUCT:
    {
        for (int i = 0; i < srcType->numItems; i++)
        {
            const Field *srcField = srcType->field[i];
            const Field *dstField = NULL;
            for (int j = 0; j < dstType->numItems; j++)
            {
                if (strcmp(srcField->name, dstType->field[j]->name) == 0)
                {
                    dstField = dstType->field[j];
                    break;
                }
            }
            if (dstField)
            {
                transfer(dstUmka, dst + dstField->offset, dstField->type, srcUmka, src + srcField->offset, srcField->type);
            }
        }
        break;
    }
    case TYPE_ARRAY:
    {
        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        const int count = srcType->numItems < dstType->numItems
                              ? srcType->numItems
                              : dstType->numItems;

        const int64_t srcStride = srcBase->size;
        const int64_t dstStride = dstBase->size;

        for (int i = 0; i < count; i++)
        {
            transfer(dstUmka, dst + i * dstStride, dstBase,
                     srcUmka, src + i * srcStride, srcBase);
        }
        break;
    }
    case TYPE_DYNARRAY:
    {
        typedef UmkaDynArray(void) DynArray;

        const DynArray *srcArr = (const DynArray *)src;
        DynArray *dstArr = (DynArray *)dst;

        const int len = umkaGetDynArrayLen(srcArr);
        umkaMakeDynArray(dstUmka, dstArr, dstType, len);

        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        for (int i = 0; i < len; i++)
        {
            transfer(dstUmka, dstArr->data + i * dstArr->itemSize, dstBase,
                     srcUmka, srcArr->data + i * srcArr->itemSize, srcBase);
        }
        break;
    }
    case TYPE_STR:
    {
        const char *srcStr = *(const char **)src;
        char **dstSlot = (char **)dst;

        if (*dstSlot)
        {
            umkaDecRef(dstUmka, *dstSlot);
        }

        *dstSlot = srcStr ? umkaMakeStr(dstUmka, srcStr) : NULL;
        break;
    }
    case TYPE_PTR:
    {
        void *srcPtr = *(void **)src;
        void **dstSlot = (void **)dst;

        if (*dstSlot)
        {
            umkaDecRef(dstUmka, *dstSlot);
        }

        if (!srcPtr)
        {
            *dstSlot = NULL;
            break;
        }

        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        if (srcBase->kind != dstBase->kind || dstBase->kind == TYPE_VOID)
        {
            *dstSlot = NULL;
            break;
        }

        void *newObj = (dstBase->kind == TYPE_STRUCT || dstBase->kind == TYPE_ARRAY)
                           ? umkaMakeStruct(dstUmka, dstBase)
                           : umkaAllocData(dstUmka, dstBase->size, NULL);

        *dstSlot = newObj;

        transfer(dstUmka, newObj, dstBase,
                 srcUmka, srcPtr, srcBase);
        break;
    }
    case TYPE_MAP:
    {
        UmkaMap *srcMap = (UmkaMap *)src;
        UmkaMap *dstMap = (UmkaMap *)dst;

        const UmkaType *srcKeyType = srcType->base->field[2]->type;
        const UmkaType *srcItemType = srcType->base->field[3]->type;
        const UmkaType *srcKeyBaseType = umkaGetBaseType(srcKeyType);
        const UmkaType *srcItemBaseType = umkaGetBaseType(srcItemType);

        const UmkaType *dstKeyType = dstType->base->field[2]->type;
        const UmkaType *dstItemType = dstType->base->field[3]->type;
        const UmkaType *dstKeyBaseType = umkaGetBaseType(dstKeyType);
        const UmkaType *dstItemBaseType = umkaGetBaseType(dstItemType);

        if (srcKeyBaseType->kind != dstKeyBaseType->kind || srcItemBaseType->kind != dstItemBaseType->kind)
        {
            break;
        }

        if (!dstMap->type)
        {
            dstMap->type = dstType;
        }

        MapNode *srcRoot = srcMap->root;
        MapNode *oldDstRoot = dstMap->root;

        dstMap->root = transferMapNode(dstUmka, dstKeyType, dstItemType,
                                       srcUmka, srcRoot, srcKeyType, srcItemType);
        if (oldDstRoot)
        {
            umkaDecRef(dstUmka, oldDstRoot);
        }
        break;
    }
    default:
        break;
    }
}

void hotReload(GameApi *curr, GameApi *next)
{
    umkaCall(curr->umka, &curr->hotReload);
    void *currState = umkaGetResult(curr->hotReload.params, curr->hotReload.result)->ptrVal;
    const UmkaType *currType = umkaGetResultType(curr->hotReload.params, curr->hotReload.result)->base;

    umkaCall(next->umka, &next->hotReload);
    void *nextState = umkaGetResult(next->hotReload.params, next->hotReload.result)->ptrVal;
    const UmkaType *nextType = umkaGetResultType(next->hotReload.params, next->hotReload.result)->base;

    transfer(next->umka, nextState, nextType, curr->umka, currState, currType);
}

int main()
{
    GameApi game;
    initGame(&game, "example.um");

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "_dev raylib basic window");
    SetTargetFPS(60);
    umkaCall(game.umka, &game.init);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(GRAY);
        umkaCall(game.umka, &game.update);
        EndDrawing();

        if (IsKeyPressed(KEY_F5))
        {
            GameApi next;
            initGame(&next, "example.um");
            hotReload(&game, &next);
            freeGame(&game);
            game = next;
        }
    }
    CloseWindow();

    umkaFree(game.umka);

    return 0;
}
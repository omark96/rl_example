#include "game_api.h"

void corePrint(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *msg = (const char *)umkaGetParam(params, 0)->ptrVal;
    puts(msg);
}

void coreGetAllGames(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    GamePool *games = &g_ctx.games;
    const UmkaType *resultType = umkaGetResultType(params, result);
    typedef UmkaDynArray(Handle) HandleArray;
    HandleArray *out = umkaGetResult(params, result)->ptrVal;
    umkaMakeDynArray(umka, out, resultType, games->liveCount);
    gamePoolGetAllHandles(games, out->data, games->liveCount);
}
void coreGetGameName(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    GamePool *games = &g_ctx.games;
    Handle gameHandle = *(Handle *)umkaGetParam(params, 0);
    char *name = gamePoolGet(games, gameHandle)->name;
    result->ptrVal = umkaMakeStr(umka, name);
}

void coreRegisterGame(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    char *name = (char *)umkaGetParam(params, 0)->ptrVal;
    GamePool games = g_ctx.games;
    Handle handle = (Handle){0};
    for (int i = 0; i < games.liveCount; i++) {
        Game game = games.items[i].item;
        if (game.name == NULL) {
            continue;
        }
        if (strcmp(game.name, name) == 0) {
            handle.slot = i;
            handle.generation = games.items[i].generation;
        }
    }
    Game *parent = getCurrentGame();
    Handle parentHandle = parent->handle;
    parent->children[parent->childCount] = handle;
    parent->childCount += 1;

    Game *childGame = gamePoolGet(&g_ctx.games, handle);
    childGame->parent = parentHandle;

    Handle *out = (Handle *)umkaGetResult(params, result)->ptrVal;
    *out = handle;
}

void coreCurrentGameState(UmkaStackSlot *params, UmkaStackSlot *result) {
    Game *game = getCurrentGame();
    result->intVal = game->state;
}

void coreGetParentGame(UmkaStackSlot *params, UmkaStackSlot *result) {
    Game *game = getCurrentGame();
    *(Handle *)umkaGetResult(params, result)->ptrVal = game->parent;
}

void coreGetGameState(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle gameHandle = *(Handle *)umkaGetParam(params, 0);
    Game *game = gamePoolGet(&g_ctx.games, gameHandle);
    umkaGetResult(params, result)->intVal = game->state;
}

void coreSetGameState(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle gameHandle = *(Handle *)umkaGetParam(params, 0);
    GameState newState = umkaGetParam(params, 1)->intVal;

    setGameState(gameHandle, newState);
}

void coreSetActiveGame(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle gameHandle = *(Handle *)umkaGetParam(params, 0);

    setActiveGame(gameHandle);
}

void coreAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "print", &corePrint);
    umkaAddFunc(umka, "getAllGames", &coreGetAllGames);
    umkaAddFunc(umka, "getGameName", &coreGetGameName);
    umkaAddFunc(umka, "getParentGame", &coreGetParentGame);
    umkaAddFunc(umka, "currentGameState", &coreCurrentGameState);
    umkaAddFunc(umka, "getGameState", &coreSetGameState);
    umkaAddFunc(umka, "setGameState", &coreSetGameState);
    umkaAddFunc(umka, "setActiveGame", &coreSetActiveGame);
    umkaAddFunc(umka, "registerGame", &coreRegisterGame);

    const char *umSourceNames[] = {"core.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "core.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
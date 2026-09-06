#include "game_api.h"

void corePrint(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *msg = (const char *)umkaGetParam(params, 0)->ptrVal;
    puts(msg);
}

void coreGetAllGames(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    GamePool *games = &g_resources.games;
    const UmkaType *resultType = umkaGetResultType(params, result);
    typedef UmkaDynArray(Handle) HandleArray;
    HandleArray *out = umkaGetResult(params, result)->ptrVal;
    umkaMakeDynArray(umka, out, resultType, games->liveCount);
    gamePoolGetAllHandles(games, out->data, games->liveCount);
}
void coreGetGameName(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    GamePool *games = &g_resources.games;
    Handle gameHandle = *(Handle *)umkaGetParam(params, 0);
    char *name = gamePoolGet(games, gameHandle)->name;
    result->ptrVal = umkaMakeStr(umka, name);
}

void coreRegisterGame(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    char *name = (char *)umkaGetParam(params, 0)->ptrVal;
    GamePool games = g_resources.games;
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
    Game *parent = gameFromUmka(umka);
    Handle parentHandle = handleFromUmka(umka);
    parent->children[parent->childCount] = handle;
    parent->childCount += 1;

    Game *childGame = gamePoolGet(&g_resources.games, handle);
    childGame->parent = parentHandle;

    Handle *out = (Handle *)umkaGetResult(params, result)->ptrVal;
    *out = handle;
}

void coreAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "print", &corePrint);
    umkaAddFunc(umka, "getAllGames", &coreGetAllGames);
    umkaAddFunc(umka, "getGameName", &coreGetGameName);
    umkaAddFunc(umka, "registerGame", &coreRegisterGame);

    const char *umSourceNames[] = {"core.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "core.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
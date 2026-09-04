#include "game_api.h"

void corePrint(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *msg = (const char *)umkaGetParam(params, 0)->ptrVal;
    puts(msg);
}

void coreGetAllGames(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    const UmkaType *resultType = umkaGetResultType(params, result);
    typedef UmkaDynArray(Handle) HandleArray;
    HandleArray *out = umkaGetResult(params, result)->ptrVal;
    umkaMakeDynArray(umka, out, resultType, games.liveCount);
    gamePoolGetAllHandles(&games, out->data, games.liveCount);
}
void coreGetGameName(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);
    Handle gameHandle = *(Handle *)umkaGetParam(params, 0);
    char *name = gamePoolGet(&games, gameHandle)->name;
    result->ptrVal = umkaMakeStr(umka, name);
}

void coreAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "print", &corePrint);
    umkaAddFunc(umka, "getAllGames", &coreGetAllGames);
    umkaAddFunc(umka, "getGameName", &coreGetGameName);

    const char *umSourceNames[] = {"core.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "core.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
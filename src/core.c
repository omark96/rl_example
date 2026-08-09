#include "game_api.h"

void corePrint(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *msg = (const char *)umkaGetParam(params, 0)->ptrVal;
    puts(msg);
}

void coreAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "print", &corePrint);

    const char *umSourceNames[] = {"core.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "core.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
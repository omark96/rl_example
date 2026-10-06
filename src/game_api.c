#include "game_api.h"
#include "core.c"
#include "gfx.c"
#include "input.c"
#include "sdf.c"

bool initGame(Game *game, const char *name) {
    game->name = strdup(name);
    return initUmka(game);
}

bool initUmka(Game *game) {
    game->umka = umkaAlloc();
    const UmkaType *stateType;
    char gamePath[PATH_MAX];
    snprintf(gamePath, sizeof(gamePath), "games/%s/main.um", game->name);

    game->lastModified = GetFileModTime(gamePath);

    bool umkaOk
        = umkaInit(game->umka, gamePath, NULL, 1024 * 1024, NULL, 0, NULL, false, false, NULL);
    if (umkaOk) {
        coreAddUmkaModule(game->umka);
        gfxAddUmkaModule(game->umka);
        inputAddUmkaModule(game->umka);
        sdfAddUmkaModule(game->umka);

        umkaOk = umkaCompile(game->umka);
    }

    if (!umkaOk) {
        UmkaError *error = umkaGetError(game->umka);
        printf("Umka error %s (%d, %d): %s\n", error->fileName, error->line, error->pos,
               error->msg);
        umkaFree(game->umka);
        game->umka = NULL;
        return false;
    }
    printf("Umka initialized\n");
    umkaGetFunc(game->umka, NULL, "update", &game->update);
    umkaGetFunc(game->umka, NULL, "init", &game->init);
    umkaGetFunc(game->umka, NULL, "hotReload", &game->hotReload);
    umkaGetFunc(game->umka, NULL, "draw", &game->draw);
    umkaGetFunc(game->umka, NULL, "input", &game->input);

    return true;
}

bool compareHandles(Handle a, Handle b) { return a.generation == b.generation && a.slot == b.slot; }

bool isRootGame(Handle handle) { return compareHandles(handle, g_ctx.rootGame); }

bool isNullHandle(Handle handle) { return compareHandles(handle, NULL_HANDLE); }

void runGame(Handle handle) {
    Game *game = gamePoolGet(&g_ctx.games, handle);

    for (int i = 0; i < game->childCount; i++) {
        runGame(game->children[i]);
    }

    if (game->umka == NULL) {
        return;
    }

    g_ctx.currentGame = handle;

    switch (game->state) {
    case STATE_ACTIVE:
        umkaCall(game->umka, &game->update);
        break;
    case STATE_ENABLED:
    case STATE_HIDDEN:
    case STATE_IDLE:
        umkaCall(game->umka, &game->update);
        break;
    }
}

void drawGame(Handle handle) {
    Game *game = gamePoolGet(&g_ctx.games, handle);

    for (int i = 0; i < game->childCount; i++) {
        drawGame(game->children[i]);
    }

    g_ctx.currentGame = handle;

    RenderTexture2D renderTexture = *renderTexture2DPoolGet(&g_ctx.renderTextures, game->screen);
    BeginTextureMode(renderTexture);
    ClearBackground(BLACK);
    if (game->umka == NULL) {
        DrawText(TextFormat("Invalid game: %s", game->name), 200, 200, 40, WHITE);
        return;
    }

    switch (game->state) {
    case STATE_ACTIVE:
    case STATE_ENABLED:
        umkaCall(game->umka, &game->draw);
        break;
    case STATE_IDLE:
    case STATE_HIDDEN:
        break;
    }
    EndTextureMode();
}

void handleInput(Handle handle) {
    Game *game = gamePoolGet(&g_ctx.games, handle);
    g_ctx.currentGame = handle;

    switch (game->state) {
    case STATE_ACTIVE:
        umkaCall(game->umka, &game->input);
        break;
    case STATE_ENABLED:
        if (isRootGame(handle)) {
            umkaCall(game->umka, &game->input);
        }
        break;
    case STATE_IDLE:
    case STATE_HIDDEN:
        break;
    }

    for (usize i = 0; i < game->childCount; i++) {
        handleInput(game->children[i]);
    }
}

Game *getCurrentGame() { return gamePoolGet(&g_ctx.games, g_ctx.currentGame); }

Handle getActiveGameHandle() {
    for (usize i = 0; i < g_ctx.games.count; i++) {
        Game game = g_ctx.games.items[i].item;
        if (game.state == STATE_ACTIVE) {
            return game.handle;
        }
    }
    return NULL_HANDLE;
}

void setGameState(Handle handle, GameState newState) {
    Game *game = gamePoolGet(&g_ctx.games, handle);
    game->state = newState;
}

GameState getGameState(Handle handle) {
    Game *game = gamePoolGet(&g_ctx.games, handle);
    return game->state;
}

void setActiveGame(Handle handle) {
    Game *game = gamePoolGet(&g_ctx.games, handle);

    if (isNullHandle(handle)) {
        return;
    }
    Handle activeHandle = getActiveGameHandle();
    if (!isNullHandle(activeHandle)) {
        if (isRootGame(activeHandle)) {
            setGameState(activeHandle, STATE_ENABLED);
        } else {
            setGameState(activeHandle, STATE_HIDDEN);
        }
    }

    setGameState(handle, STATE_ACTIVE);

    if (game->cursorDisabled) {
        DisableCursor();
    } else {
        EnableCursor();
    }

    for (usize i = 0; i < game->childCount; i++) {
        Handle childHandle = game->children[i];
        setGameState(childHandle, STATE_ENABLED);
    }
}

void onWarning(UmkaError *err) {
    fprintf(stderr, "%s (%s:%d): %s\n", err->fnName, err->fileName, err->line, err->msg);
}

void freeGame(Game *game) {
    if (!game->umka) {
        return;
    }
    umkaFree(game->umka);
    *game = (Game){0};
}

void transfer(Umka *dstUmka, void *dst, const UmkaType *dstType, Umka *srcUmka, void *src,
              const UmkaType *srcType) {
    if (umkaGetTypeKind(srcType) != umkaGetTypeKind(dstType))
        return;

    switch (umkaGetTypeKind(srcType)) {
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
    case TYPE_UINT: {
        memcpy(dst, src, umkaGetTypeSize(srcType));
        break;
    }
    case TYPE_STRUCT: {
        for (usize i = 0;; i++) {
            const char *fieldName = umkaGetFieldName(srcType, i);
            if (!fieldName)
                break;

            const UmkaType *dstFieldType = umkaGetFieldType(dstType, fieldName);
            if (!dstFieldType)
                continue;

            const UmkaType *srcFieldType = umkaGetFieldType(srcType, fieldName);
            const i32 dstOffset = umkaGetFieldOffset(dstType, fieldName);
            const i32 srcOffset = umkaGetFieldOffset(srcType, fieldName);

            transfer(dstUmka, dst + dstOffset, dstFieldType, srcUmka, src + srcOffset,
                     srcFieldType);
        }
        break;
    }
    case TYPE_ARRAY: {
        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        const i32 srcLen = umkaGetTypeLen(srcType);
        const i32 dstLen = umkaGetTypeLen(dstType);
        const i32 count = srcLen < dstLen ? srcLen : dstLen;

        const i64 srcStride = umkaGetTypeSize(srcBase);
        const i64 dstStride = umkaGetTypeSize(dstBase);

        for (usize i = 0; i < count; i++) {
            transfer(dstUmka, dst + i * dstStride, dstBase, srcUmka, src + i * srcStride, srcBase);
        }
        break;
    }
    case TYPE_DYNARRAY: {
        typedef UmkaDynArray(void) DynArray;

        const DynArray *srcArr = (const DynArray *)src;
        DynArray *dstArr = (DynArray *)dst;

        const i32 len = umkaGetDynArrayLen(srcArr);
        umkaMakeDynArray(dstUmka, dstArr, dstType, len);

        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        for (i32 i = 0; i < len; i++) {
            transfer(dstUmka, dstArr->data + i * dstArr->itemSize, dstBase, srcUmka,
                     srcArr->data + i * srcArr->itemSize, srcBase);
        }
        break;
    }
    case TYPE_STR: {
        const char *srcStr = *(const char **)src;
        char **dstSlot = (char **)dst;

        if (*dstSlot) {
            umkaDecRef(dstUmka, *dstSlot);
        }

        *dstSlot = srcStr ? umkaMakeStr(dstUmka, srcStr) : NULL;
        break;
    }
    case TYPE_PTR: {
        void *srcPtr = *(void **)src;
        void **dstSlot = (void **)dst;

        if (*dstSlot) {
            umkaDecRef(dstUmka, *dstSlot);
        }

        if (!srcPtr) {
            *dstSlot = NULL;
            break;
        }

        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        if (umkaGetTypeKind(srcBase) != umkaGetTypeKind(dstBase)
            || umkaGetTypeKind(dstBase) == TYPE_VOID) {
            *dstSlot = NULL;
            break;
        }

        const UmkaTypeKind dstBaseKind = umkaGetTypeKind(dstBase);
        void *newObj = (dstBaseKind == TYPE_STRUCT || dstBaseKind == TYPE_ARRAY)
                           ? umkaMakeStruct(dstUmka, dstBase)
                           : umkaAllocData(dstUmka, umkaGetTypeSize(dstBase), NULL);

        *dstSlot = newObj;

        transfer(dstUmka, newObj, dstBase, srcUmka, srcPtr, srcBase);
        break;
    }
    case TYPE_MAP: {
        UmkaMap *srcMap = (UmkaMap *)src;
        UmkaMap *dstMap = (UmkaMap *)dst;

        const UmkaType *srcKeyType = umkaGetMapKeyType(srcType);
        const UmkaType *srcItemType = umkaGetMapItemType(srcType);
        const UmkaType *dstKeyType = umkaGetMapKeyType(dstType);
        const UmkaType *dstItemType = umkaGetMapItemType(dstType);

        if (umkaGetTypeKind(srcKeyType) != umkaGetTypeKind(dstKeyType)
            || umkaGetTypeKind(srcItemType) != umkaGetTypeKind(dstItemType)) {
            break;
        }

        if (!dstMap->type) {
            dstMap->type = dstType;
        }

        UmkaDynArray(void) keys = {0};
        umkaGetMapKeys(srcUmka, srcMap, &keys);

        const i32 len = umkaGetDynArrayLen(&keys);
        const i32 srcKeySize = umkaGetTypeSize(srcKeyType);
        const i32 dstKeySize = umkaGetTypeSize(dstKeyType);
        const UmkaTypeKind dstKeyKind = umkaGetTypeKind(dstKeyType);

        void *dstKey = calloc(1, dstKeySize);

        for (i32 i = 0; i < len; i++) {
            void *srcKey = keys.data + i * srcKeySize;

            // Re-create the key inside the destination instance, then index both maps
            memset(dstKey, 0, dstKeySize);
            transfer(dstUmka, dstKey, dstKeyType, srcUmka, srcKey, srcKeyType);

            void *srcItem = umkaGetMapItem(srcUmka, srcMap, srcKey);
            void *dstItem = umkaGetMapItem(dstUmka, dstMap, dstKey);

            if (srcItem && dstItem) {
                transfer(dstUmka, dstItem, dstItemType, srcUmka, srcItem, srcItemType);
            }

            // umkaGetMapItem took its own reference to the key
            if (dstKeyKind == TYPE_STR || dstKeyKind == TYPE_PTR) {
                umkaDecRef(dstUmka, *(void **)dstKey);
            }
        }

        free(dstKey);
        break;
    }
    default:
        break;
    }
}

void hotReload(Game *curr, Game *next) {
    umkaCall(curr->umka, &curr->hotReload);
    void *currState = umkaGetResult(curr->hotReload.params, curr->hotReload.result)->ptrVal;
    const UmkaType *currType
        = umkaGetBaseType(umkaGetResultType(curr->hotReload.params, curr->hotReload.result));

    umkaCall(next->umka, &next->hotReload);
    void *nextState = umkaGetResult(next->hotReload.params, next->hotReload.result)->ptrVal;
    const UmkaType *nextType
        = umkaGetBaseType(umkaGetResultType(next->hotReload.params, next->hotReload.result));

    transfer(next->umka, nextState, nextType, curr->umka, currState, currType);
}

bool reloadGame(Game *curr, long modTime) {
    Game next = {0};
    next.name = curr->name;

    if (!initUmka(&next)) {
        curr->lastModified = modTime;
        return false;
    }
    hotReload(curr, &next);
    if (curr->umka) {
        umkaFree(curr->umka);
    }
    curr->umka = next.umka;
    curr->update = next.update;
    curr->init = next.init;
    curr->hotReload = next.hotReload;
    curr->draw = next.draw;
    curr->input = next.input;
    if (modTime) {
        curr->lastModified = modTime;
    }
    return true;
}

void checkForGameUpdates(GamePool *games) {
    FilePathList gameDirs = LoadDirectoryFilesEx("games", "DIRS*", false);

    for (usize i = 0; i < gameDirs.count; i++) {
        const char *gameDir = gameDirs.paths[i];
        const char *gameName = GetFileName(gameDir);

        for (usize j = 0; j < games->count; j++) {
            Game *game = &games->items[j].item;
            if (!game->name || strcmp(game->name, gameName) != 0) {
                continue;
            }
            long lastModified = 0;
            FilePathList umkaFiles = LoadDirectoryFilesEx(gameDir, ".um", true);
            for (usize i = 0; i < umkaFiles.count; i++) {
                long modTime = GetFileModTime(umkaFiles.paths[i]);
                if (modTime > lastModified) {
                    lastModified = modTime;
                }
            }
            UnloadDirectoryFiles(umkaFiles);
            if (lastModified > game->lastModified) {
                reloadGame(game, lastModified);
            }
            break;
        }
    }

    UnloadDirectoryFiles(gameDirs);
}

// TODO: Broken and won't allow inserting a file extension.
//       Need a more robust system for what folders a game can load assets from.
bool isValidAssetName(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    for (const char *c = name; *c != '\0'; c++) {
        bool ok = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9')
                  || *c == '_' || *c == '-' || *c == '/' || *c == '\\';
        if (!ok) {
            return false;
        }
    }
    return true;
}
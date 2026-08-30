#include "game_api.h"
#include "core.c"
#include "gfx.c"
#include "input.c"

bool initGame(Game *game, char *name) {
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

    return true;
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
              const UmkaType *srcType);

static MapNode *transferMapNode(Umka *dstUmka, const UmkaType *dstKeyType,
                                const UmkaType *dstItemType, Umka *srcUmka, MapNode *srcNode,
                                const UmkaType *srcKeyType, const UmkaType *srcItemType) {
    if (!srcNode) {
        return NULL;
    }

    MapNode *dstNode = (MapNode *)umkaAllocData(dstUmka, sizeof(MapNode), NULL);
    dstNode->len = srcNode->len;
    dstNode->priority = srcNode->priority;

    transfer(dstUmka, &dstNode->key, dstKeyType, srcUmka, &srcNode->key, srcKeyType);

    transfer(dstUmka, &dstNode->data, dstItemType, srcUmka, &srcNode->data, srcItemType);

    dstNode->left = transferMapNode(dstUmka, dstKeyType, dstItemType, srcUmka, srcNode->left,
                                    srcKeyType, srcItemType);
    dstNode->right = transferMapNode(dstUmka, dstKeyType, dstItemType, srcUmka, srcNode->right,
                                     srcKeyType, srcItemType);

    return dstNode;
}

void transfer(Umka *dstUmka, void *dst, const UmkaType *dstType, Umka *srcUmka, void *src,
              const UmkaType *srcType) {
    if (srcType->kind != dstType->kind)
        return;

    switch (srcType->kind) {
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
        memcpy(dst, src, srcType->size);
        break;
    }
    case TYPE_STRUCT: {
        for (int i = 0; i < srcType->numItems; i++) {
            const Field *srcField = srcType->field[i];
            const Field *dstField = NULL;
            for (int j = 0; j < dstType->numItems; j++) {
                if (strcmp(srcField->name, dstType->field[j]->name) == 0) {
                    dstField = dstType->field[j];
                    break;
                }
            }
            if (dstField) {
                transfer(dstUmka, dst + dstField->offset, dstField->type, srcUmka,
                         src + srcField->offset, srcField->type);
            }
        }
        break;
    }
    case TYPE_ARRAY: {
        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        const int count
            = srcType->numItems < dstType->numItems ? srcType->numItems : dstType->numItems;

        const int64_t srcStride = srcBase->size;
        const int64_t dstStride = dstBase->size;

        for (int i = 0; i < count; i++) {
            transfer(dstUmka, dst + i * dstStride, dstBase, srcUmka, src + i * srcStride, srcBase);
        }
        break;
    }
    case TYPE_DYNARRAY: {
        typedef UmkaDynArray(void) DynArray;

        const DynArray *srcArr = (const DynArray *)src;
        DynArray *dstArr = (DynArray *)dst;

        const int len = umkaGetDynArrayLen(srcArr);
        umkaMakeDynArray(dstUmka, dstArr, dstType, len);

        const UmkaType *srcBase = umkaGetBaseType(srcType);
        const UmkaType *dstBase = umkaGetBaseType(dstType);

        for (int i = 0; i < len; i++) {
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

        if (srcBase->kind != dstBase->kind || dstBase->kind == TYPE_VOID) {
            *dstSlot = NULL;
            break;
        }

        void *newObj = (dstBase->kind == TYPE_STRUCT || dstBase->kind == TYPE_ARRAY)
                           ? umkaMakeStruct(dstUmka, dstBase)
                           : umkaAllocData(dstUmka, dstBase->size, NULL);

        *dstSlot = newObj;

        transfer(dstUmka, newObj, dstBase, srcUmka, srcPtr, srcBase);
        break;
    }
    case TYPE_MAP: {
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

        if (srcKeyBaseType->kind != dstKeyBaseType->kind
            || srcItemBaseType->kind != dstItemBaseType->kind) {
            break;
        }

        if (!dstMap->type) {
            dstMap->type = dstType;
        }

        MapNode *srcRoot = srcMap->root;
        MapNode *oldDstRoot = dstMap->root;

        dstMap->root = transferMapNode(dstUmka, dstKeyType, dstItemType, srcUmka, srcRoot,
                                       srcKeyType, srcItemType);
        if (oldDstRoot) {
            umkaDecRef(dstUmka, oldDstRoot);
        }
        break;
    }
    default:
        break;
    }
}

void hotReload(Game *curr, Game *next) {
    if (!next->umka) {
        return;
    }
    if (!curr->umka) {
        umkaCall(next->umka, &next->init);
        return;
    }
    umkaCall(curr->umka, &curr->hotReload);
    void *currState = umkaGetResult(curr->hotReload.params, curr->hotReload.result)->ptrVal;
    const UmkaType *currType
        = umkaGetResultType(curr->hotReload.params, curr->hotReload.result)->base;

    umkaCall(next->umka, &next->hotReload);
    void *nextState = umkaGetResult(next->hotReload.params, next->hotReload.result)->ptrVal;
    const UmkaType *nextType
        = umkaGetResultType(next->hotReload.params, next->hotReload.result)->base;

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
    if (modTime) {
        curr->lastModified = modTime;
    }
    return true;
}

void checkForGameUpdates(GamePool *games) {
    FilePathList gamePaths = LoadDirectoryFilesEx("games", "DIRS*", false);

    int gameCount = gamePaths.count;

    char gamePath[PATH_MAX];

    for (int i = 0; i < gameCount; i++) {
        char *gameName = gamePaths.paths[i] + 6;
        snprintf(gamePath, sizeof(gamePath), "games/%s/main.um", gameName);

        for (int j = 0; j < games->count; j++) {
            Game *game = &games->items[j].item;
            if (!game->name) {
                continue;
            }
            if (strcmp(game->name, gameName) == 0) {
                long lastModified = GetFileModTime(gamePath);
                if (lastModified > game->lastModified) {
                    reloadGame(game, lastModified);
                }
                break;
            }
        }
    }

    UnloadDirectoryFiles(gamePaths);
}
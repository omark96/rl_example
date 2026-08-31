#include "handle.h"

#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)
#define POOL_FN_PREFIX CAT(F_PREFIX, Pool)
#define POOL_FN(name) CAT(POOL_FN_PREFIX, name)

#define SLOT_TYPE CAT(T, Slot)
#define POOL_TYPE CAT(T, Pool)

#ifndef POOL_INIT_SIZE
#define POOL_INIT_SIZE 4
#endif // POOL_INIT_SIZE

#ifndef POOL_MAX_CAP
#define POOL_MAX_CAP 100000
#endif // POOL_MAX_CAP

typedef struct SLOT_TYPE {
    T item;
    uint32_t generation;
    uint32_t nextFree;
} SLOT_TYPE;

typedef struct POOL_TYPE {
    SLOT_TYPE *items;
    uint32_t count;
    uint32_t liveCount;
    uint32_t cap;
    uint32_t firstFree;
    uint8_t ownerId;
    uint8_t ownerGen;
} POOL_TYPE;

void POOL_FN(Init)(POOL_TYPE *pool, uint8_t ownerId, uint8_t ownerGen);
bool POOL_FN(Grow)(POOL_TYPE *pool);
Handle POOL_FN(Add)(POOL_TYPE *pool, T item);
bool POOL_FN(Remove)(POOL_TYPE *pool, Handle handle);
bool POOL_FN(Resolve)(POOL_TYPE *pool, Handle handle, uint16_t *outSlotId);
T *POOL_FN(Get)(POOL_TYPE *pool, Handle handle);

#ifdef POOL_IMPLEMENTATION
void POOL_FN(Init)(POOL_TYPE *pool, uint8_t ownerId, uint8_t ownerGen) {
    pool->items = malloc(sizeof(SLOT_TYPE) * POOL_INIT_SIZE);
    pool->items[0] = (SLOT_TYPE){0};
    pool->count = 0;
    pool->cap = POOL_INIT_SIZE;
    pool->firstFree = 0;
    pool->ownerId = ownerId;
    pool->ownerGen = ownerGen;
    for (size_t i = 0; i < POOL_INIT_SIZE; i++) {
        pool->items[i].generation = 0;
        pool->items[i].item = (T){0};
        pool->items[i].nextFree = 0;
    }
}

bool POOL_FN(Grow)(POOL_TYPE *pool) {
    if (pool->cap >= POOL_MAX_CAP) {
        return false;
    }
    uint32_t oldCap = pool->cap;
    pool->cap = oldCap ? oldCap * 2 : 4;
    pool->cap = pool->cap > POOL_MAX_CAP ? POOL_MAX_CAP : pool->cap;

    pool->items = realloc(pool->items, sizeof(SLOT_TYPE) * pool->cap);

    for (size_t i = oldCap; i < pool->cap; i++) {
        pool->items[i].generation = 0;
        pool->items[i].item = (T){0};
        pool->items[i].nextFree = 0;
    }

    return true;
}

Handle POOL_FN(Add)(POOL_TYPE *pool, T item) {
    uint32_t slotId = pool->firstFree;
    if (slotId != 0) {
        pool->firstFree = pool->items[slotId].nextFree;
    } else {
        pool->count += 1;
        if (pool->count >= pool->cap) {
            bool ok = POOL_FN(Grow)(pool);
        }
        slotId = pool->count;
    }

    pool->liveCount += 1;

    SLOT_TYPE *slot = &pool->items[slotId];
    slot->nextFree = 0;
    slot->item = item;
    slot->generation += 1;

    return makeHandle(POOL_RES_TYPE, pool->ownerId, pool->ownerGen, slotId, slot->generation);
}

bool POOL_FN(Resolve)(POOL_TYPE *pool, Handle handle, uint16_t *outSlotId) {
    if (handleType(handle) != POOL_RES_TYPE) {
        return false;
    }
    if (handleOwnerId(handle) != pool->ownerId) {
        return false;
    }
    if (handleOwnerGen(handle) != pool->ownerGen) {
        return false;
    }
    uint16_t slotId = handleSlot(handle);
    if (slotId == 0 || slotId > pool->count) {
        return false;
    }
    if (pool->items[slotId].generation != handleSlotGen(handle)) {
        return false;
    }
    *outSlotId = slotId;
    return true;
}

bool POOL_FN(Remove)(POOL_TYPE *pool, Handle handle) {
    uint16_t slotId;
    if (!POOL_FN(Resolve)(pool, handle, &slotId)) {
        return false;
    }

    pool->liveCount -= 1;

    SLOT_TYPE *slot = &pool->items[slotId];
    slot->generation += 1;
    slot->item = (T){0};
    slot->nextFree = pool->firstFree;
    pool->firstFree = slotId;
    return true;
}

T *POOL_FN(Get)(POOL_TYPE *pool, Handle handle) {
    uint16_t slotId;
    if (!POOL_FN(Resolve)(pool, handle, &slotId)) {
        return &pool->items[0].item;
    }
    return &pool->items[slotId].item;
}

#undef POOL_IMPLEMENTATION
#endif // POOL_IMPLEMENTATION

#undef POOL_RES_TYPE
#undef POOL_MAX_CAP
#undef SLOT_TYPE
#undef POOL_TYPE
#undef F_PREFIX
#undef T
#undef NAME
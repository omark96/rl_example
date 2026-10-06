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
    u32 generation;
    u32 nextFree;
} SLOT_TYPE;

typedef struct POOL_TYPE {
    SLOT_TYPE *items;
    u32 count;
    u32 liveCount;
    u32 cap;
    u32 firstFree;
} POOL_TYPE;

void POOL_FN(Init)(POOL_TYPE *pool, T defaultItem);
bool POOL_FN(Grow)(POOL_TYPE *pool);
Handle POOL_FN(Add)(POOL_TYPE *pool, T item);
bool POOL_FN(Remove)(POOL_TYPE *pool, Handle handle);
bool POOL_FN(Resolve)(POOL_TYPE *pool, Handle handle, u32 *outSlotId);
T *POOL_FN(Get)(POOL_TYPE *pool, Handle handle);
void POOL_FN(SetDefault)(POOL_TYPE *pool, T item);

#ifdef POOL_IMPLEMENTATION
void POOL_FN(Init)(POOL_TYPE *pool, T defaultItem) {
    pool->items = malloc(sizeof(SLOT_TYPE) * POOL_INIT_SIZE);
    pool->count = 1;
    pool->liveCount = 0;
    pool->cap = POOL_INIT_SIZE;
    pool->firstFree = 0;
    for (usize i = 0; i < POOL_INIT_SIZE; i++) {
        pool->items[i].generation = 0;
        pool->items[i].item = (T){0};
        pool->items[i].nextFree = 0;
    }
    pool->items[0].item = defaultItem;
}

bool POOL_FN(Grow)(POOL_TYPE *pool) {
    if (pool->cap >= POOL_MAX_CAP) {
        return false;
    }
    u32 oldCap = pool->cap;
    pool->cap = oldCap ? oldCap * 2 : 4;
    pool->cap = pool->cap > POOL_MAX_CAP ? POOL_MAX_CAP : pool->cap;

    pool->items = realloc(pool->items, sizeof(SLOT_TYPE) * pool->cap);

    for (usize i = oldCap; i < pool->cap; i++) {
        pool->items[i].generation = 0;
        pool->items[i].item = (T){0};
        pool->items[i].nextFree = 0;
    }

    return true;
}

Handle POOL_FN(Add)(POOL_TYPE *pool, T item) {
    u32 slotId = pool->firstFree;
    if (slotId != 0) {
        pool->firstFree = pool->items[slotId].nextFree;
    } else {
        if (pool->count >= pool->cap) {
            bool ok = POOL_FN(Grow)(pool);
        }
        slotId = pool->count;
        pool->count += 1;
    }

    pool->liveCount += 1;

    SLOT_TYPE *slot = &pool->items[slotId];
    slot->nextFree = 0;
    slot->item = item;
    slot->generation += 1;

    return (Handle){.slot = slotId, .generation = slot->generation};
}

bool POOL_FN(Resolve)(POOL_TYPE *pool, Handle handle, u32 *outSlotId) {
    u32 slotId = handle.slot;
    if (slotId == 0 || slotId > pool->count) {
        return false;
    }
    if (pool->items[slotId].generation != handle.generation) {
        return false;
    }
    *outSlotId = slotId;
    return true;
}

bool POOL_FN(Remove)(POOL_TYPE *pool, Handle handle) {
    u32 slotId;
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
    u32 slotId;
    if (!POOL_FN(Resolve)(pool, handle, &slotId)) {
        return &pool->items[0].item;
    }
    return &pool->items[slotId].item;
}

u32 POOL_FN(GetAllHandles)(POOL_TYPE *pool, Handle *handles, u32 size) {
    u32 max = size < pool->liveCount ? size : pool->liveCount;
    u32 next = 0;
    for (u32 i = 0; i <= pool->count; i++) {
        SLOT_TYPE slot = pool->items[i];
        if (slot.generation & 1) {
            handles[next] = (Handle){.slot = i, .generation = slot.generation};
            next += 1;
        }
        if (next >= max) {
            break;
        }
    }
    return next;
}

void POOL_FN(SetDefault)(POOL_TYPE *pool, T item) { pool->items[0].item = item; }

#undef POOL_IMPLEMENTATION
#endif // POOL_IMPLEMENTATION

#undef POOL_RES_TYPE
#undef POOL_MAX_CAP
#undef SLOT_TYPE
#undef POOL_TYPE
#undef F_PREFIX
#undef T
#undef NAME
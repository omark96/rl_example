#pragma once
#include <stdint.h>
#define H_TYPE_SHIFT 58
#define H_OWNER_SHIFT 51
#define H_OGEN_SHIFT 47
#define H_SLOT_SHIFT 31
#define H_SGEN_SHIFT 0

#define H_TYPE_MASK 0x1FULL
#define H_OWNER_MASK 0x7FULL
#define H_OGEN_MASK 0x0FULL
#define H_SLOT_MASK 0xFFFFULL
#define H_SGEN_MASK 0x7FFFFFFFULL

#define HANDLE_NULL 0ULL

typedef uint64_t Handle;

typedef enum {
    RES_NONE = 0,
    RES_GAME,
    RES_TEXTURE,
    RES_RENDER_TEXTURE,
    RES_TYPE_COUNT
} ResType;

static_assert(RES_TYPE_COUNT <= 32, "type field overflow");

static inline uint8_t handleType(Handle h) { return (h >> H_TYPE_SHIFT) & H_TYPE_MASK; }

static inline uint8_t handleOwnerId(Handle h) { return (h >> H_OWNER_SHIFT) & H_OWNER_MASK; }

static inline uint8_t handleOwnerGen(Handle h) { return (h >> H_OGEN_SHIFT) & H_OGEN_MASK; }

static inline uint16_t handleSlot(Handle h) { return (h >> H_SLOT_SHIFT) & H_SLOT_MASK; }

static inline uint32_t handleSlotGen(Handle h) { return h & H_SGEN_MASK; }

static inline Handle makeHandle(uint8_t type, uint8_t gameId, uint8_t gameGen, uint16_t slot,
                                uint32_t slotGen) {
    return ((Handle)(type & H_TYPE_MASK) << H_TYPE_SHIFT)
           | ((Handle)(gameId & H_OWNER_MASK) << H_OWNER_SHIFT)
           | ((Handle)(gameGen & H_OGEN_MASK) << H_OGEN_SHIFT)
           | ((Handle)(slot & H_SLOT_MASK) << H_SLOT_SHIFT) | (Handle)(slotGen & H_SGEN_MASK);
}
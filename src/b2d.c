#include "box2d.h"
#include "game_api.h"

typedef struct umVec2 {
    float x;
    float y;
} umVec2;

typedef struct b2dWorldDef {
    umVec2 gravity;
    float restitutionThreshold;
    int restitutionIterations;
    bool enableRestitutionPropagation;
    float hitEventThreshold;
    float contactHertz;
    float contactDampingRatio;
    float contactSpeed;
    float maximumLinearSpeed;
    bool enableSleep;
    bool enableContinuous;
} b2dWorldDef;

void b2dDefaultWorldDef(UmkaStackSlot *params, UmkaStackSlot *result) {
    b2WorldDef b2Def = b2DefaultWorldDef();
    b2dWorldDef
}
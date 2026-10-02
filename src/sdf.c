#include "game_api.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

typedef enum {
    SDF_CIRCLE = 0,
    SDF_ROUND_RECT = 1,
} SdfShapeType;

typedef struct {
    Vector2 center;
    Vector2 halfSize;
    Vector4 params;
    Vector4 color;
    float intensity;
    float type; // SdfShapeType
    float rotation;
    float stroke;
} SdfInstance;

#define SDF_MAX_INSTANCES 4096

typedef struct {
    SdfInstance instances[SDF_MAX_INSTANCES];
    int count;
} SdfCommandBuffer;

typedef struct {
    Shader shader;
    int locMvp;
    int locGlowPad;
    uint32_t vao;
    uint32_t quadVbo;
    uint32_t instanceVbo;
    float glowPad;
    SdfCommandBuffer commandBuffer;
} SdfRenderer;

SdfRenderer sdf;

#ifdef PLATFORM_WEB
#define GLSL_VERSION "#version 300 es\n"
#else
#define GLSL_VERSION "#version 330\n"
#endif

Shader loadSdfShader(const char *vsPath, const char *fsPath) {
    char *vsBody = LoadFileText(vsPath);
    char *fsBody = LoadFileText(fsPath);

    uint32_t vsLen = strlen(GLSL_VERSION) + strlen(vsBody) + 1;
    char *vs = malloc(vsLen);
    snprintf(vs, vsLen, "%s%s", GLSL_VERSION, vsBody);

    uint32_t fsLen = strlen(GLSL_VERSION) + strlen(fsBody) + 1;
    char *fs = malloc(fsLen);
    snprintf(fs, fsLen, "%s%s", GLSL_VERSION, fsBody);

    Shader shader = LoadShaderFromMemory(vs, fs);

    free(vs);
    free(fs);

    UnloadFileText(vsBody);
    UnloadFileText(fsBody);

    return shader;
}

void initSdf(uint32_t screenWidth, uint32_t screenHeight) {
    sdf.shader = loadSdfShader("defaultAssets/shaders/sdf.vs", "defaultAssets/shaders/sdf.fs");
    sdf.locMvp = GetShaderLocation(sdf.shader, "mvp");
    sdf.locGlowPad = GetShaderLocation(sdf.shader, "glowPad");
    sdf.glowPad = 2.0f;

    float quad[12] = {
        -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1,
    };

    sdf.vao = rlLoadVertexArray();
    rlEnableVertexArray(sdf.vao);

    sdf.quadVbo = rlLoadVertexBuffer(quad, sizeof(quad), false);
    rlEnableVertexBuffer(sdf.quadVbo);
    rlSetVertexAttribute(0, 2, RL_FLOAT, false, 2 * sizeof(float), 0);
    rlEnableVertexAttribute(0);

    sdf.instanceVbo = rlLoadVertexBuffer(NULL, SDF_MAX_INSTANCES * sizeof(SdfInstance), true);
    rlEnableVertexBuffer(sdf.instanceVbo);
    uint32_t s = sizeof(SdfInstance);
    rlSetVertexAttribute(1, 4, RL_FLOAT, false, s, offsetof(SdfInstance, center));
    rlSetVertexAttribute(2, 4, RL_FLOAT, false, s, offsetof(SdfInstance, params));
    rlSetVertexAttribute(3, 4, RL_FLOAT, false, s, offsetof(SdfInstance, color));
    rlSetVertexAttribute(4, 4, RL_FLOAT, false, s, offsetof(SdfInstance, intensity));
    for (int i = 1; i <= 4; i++) {
        rlEnableVertexAttribute(i);
        rlSetVertexAttributeDivisor(i, 1);
    }

    rlDisableVertexArray();
    rlDisableVertexBuffer();
}

void flushSdf(SdfCommandBuffer *buffer) {
    if (buffer->count == 0)
        return;

    rlDrawRenderBatchActive();

    rlUpdateVertexBuffer(sdf.instanceVbo, buffer->instances, buffer->count * sizeof(SdfInstance),
                         0);

    Matrix mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());

    rlEnableShader(sdf.shader.id);
    rlSetUniformMatrix(sdf.locMvp, mvp);
    rlSetUniform(sdf.locGlowPad, &sdf.glowPad, RL_SHADER_UNIFORM_FLOAT, 1);

    rlEnableVertexArray(sdf.vao);
    rlDisableBackfaceCulling();
    rlDrawVertexArrayInstanced(0, 6, buffer->count);
    rlEnableBackfaceCulling();
    rlDisableVertexArray();
    rlDisableShader();

    buffer->count = 0;
}

SdfInstance *pushSdf(SdfCommandBuffer *buffer) {
    if (buffer->count == SDF_MAX_INSTANCES) {
        flushSdf(buffer);
    }
    SdfInstance *inst = &buffer->instances[buffer->count++];
    *inst = (SdfInstance){0};
    return inst;
}

void drawCircleSdf(SdfCommandBuffer *buffer, Vector2 center, float radius, Vector4 color,
                   float intensity) {
    SdfInstance *inst = pushSdf(buffer);
    inst->center = center;
    inst->halfSize = (Vector2){radius, radius};
    inst->params.x = radius;
    inst->color = color;
    inst->intensity = intensity;
    inst->type = (float)SDF_CIRCLE;
}

void beginSdfMode() { BeginBlendMode(BLEND_ALPHA_PREMULTIPLY); }

void endSdfMode() {
    flushSdf(&sdf.commandBuffer);

    EndBlendMode();
}

void sdfBeginSdfMode(UmkaStackSlot *params, UmkaStackSlot *result) { beginSdfMode(); }

void sdfEndSdfMode(UmkaStackSlot *params, UmkaStackSlot *result) {
    SdfCommandBuffer *buffer = umkaGetParam(params, 0)->ptrVal;
    flushSdf(buffer);
    endSdfMode();
}

void sdfFlushSdf(UmkaStackSlot *params, UmkaStackSlot *result) {
    SdfCommandBuffer *buffer = umkaGetParam(params, 0)->ptrVal;
    flushSdf(buffer);
}

void sdfAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "_endSdfMode", &sdfEndSdfMode);
    umkaAddFunc(umka, "beginSdfMode", &sdfBeginSdfMode);
    umkaAddFunc(umka, "flushSdf", &sdfFlushSdf);

    const char *umSourceNames[] = {"sdf.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "sdf.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
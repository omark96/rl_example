#include "game_api.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "yyjson.h"

typedef struct {
    float left;
    float bottom;
    float right;
    float top;
} Bounds;

typedef struct {
    uint32_t codePoint;
    Bounds planeRect;
    Bounds atlasRect;
    float advance;
} GlyphData;

typedef struct {
    GlyphData *glyphData;
} SDFFont;

typedef struct {
    Vector2 center;
    Vector2 extent;
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

void addGlyph(GlyphData *glyphs, yyjson_val *glyph, size_t idx) {
    yyjson_val *unicode = yyjson_obj_get(glyph, "unicode");
    yyjson_val *advance = yyjson_obj_get(glyph, "advance");
    yyjson_val *planeBounds = yyjson_obj_get(glyph, "planeBounds");
    yyjson_val *atlasBounds = yyjson_obj_get(glyph, "atlasBounds");

    GlyphData glyphData = {0};
    glyphData.codePoint = yyjson_get_int(unicode);
    yyjson_val *planeLeft = yyjson_obj_get(planeBounds, "left");
    yyjson_val *planeBottom = yyjson_obj_get(planeBounds, "bottom");
    yyjson_val *planeRight = yyjson_obj_get(planeBounds, "right");
    yyjson_val *planeTop = yyjson_obj_get(planeBounds, "top");
    glyphData.planeRect
        = (Bounds){(float)yyjson_get_real(planeLeft), (float)yyjson_get_real(planeBottom),
                   (float)yyjson_get_real(planeRight), (float)yyjson_get_real(planeTop)};

    yyjson_val *atlasLeft = yyjson_obj_get(atlasBounds, "left");
    yyjson_val *atlasBottom = yyjson_obj_get(atlasBounds, "bottom");
    yyjson_val *atlasRight = yyjson_obj_get(atlasBounds, "right");
    yyjson_val *atlasTop = yyjson_obj_get(atlasBounds, "top");
    glyphData.atlasRect = (Bounds){yyjson_get_real(atlasLeft), yyjson_get_real(atlasBottom),
                                   yyjson_get_real(atlasRight), yyjson_get_real(atlasTop)};
    glyphData.advance = yyjson_get_real(advance);
    glyphs[idx] = glyphData;
}

SDFFont loadSdfFont(const char *fontName) {
    SDFFont font = {0};
    if (!isValidAssetName(fontName)) {
        printf("Invalid font name");
        return font;
    }
    char path[256];
    int pathLen = snprintf(path, sizeof(path), "defaultAssets/fonts/%s.json", fontName);
    if (pathLen < 0 || pathLen >= sizeof(path)) {
        printf("Too long font name");
        return font;
    }
    yyjson_read_err err;
    yyjson_doc *doc = yyjson_read_file(path, 0, NULL, &err);
    if (doc == NULL) {
        printf("%s:, %s", err.pos, err.msg);
        return font;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *name = yyjson_obj_get(root, "name");
    printf("%s\n", yyjson_get_str(name));
    yyjson_val *glyphs = yyjson_obj_get(root, "glyphs");
    size_t glyphCount = yyjson_arr_size(glyphs);
    printf("%d\n", glyphCount);
    font.glyphData = malloc(glyphCount * sizeof(GlyphData));
    size_t idx, max;
    yyjson_val *val;
    yyjson_arr_foreach(glyphs, idx, max, val) { addGlyph(font.glyphData, val, idx); }
    assert(false);
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

void sdfLoadFont(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *name = (const char *)umkaGetParam(params, 0)->ptrVal;
    SDFFont font = loadSdfFont(name);
    Handle handle = {0};
    *(Handle *)umkaGetResult(params, result)->ptrVal = handle;
}

void sdfAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "_endSdfMode", &sdfEndSdfMode);
    umkaAddFunc(umka, "beginSdfMode", &sdfBeginSdfMode);
    umkaAddFunc(umka, "flushSdf", &sdfFlushSdf);
    umkaAddFunc(umka, "loadFont", &sdfLoadFont);

    const char *umSourceNames[] = {"sdf.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "sdf.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
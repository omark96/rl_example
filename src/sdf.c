#include "game_api.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "yyjson.h"

typedef struct {
    uint32_t codePoint;
    Vector2 planeCenter;
    Vector2 planeExtent;
    Vector2 uvCenter;
    Vector2 uvExtent;
    float advance;
} GlyphData;

typedef struct {
    float lineHeight;
    float ascender;
    float descender;
    float distanceRange;
    Vector2 atlasSize;
} FontMetrics;

typedef struct {
    FontMetrics metrics;
    GlyphData *glyphData;
    uint32_t glyphCount;
    Handle texture;
} SdfFont;

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
    int locAtlas;
    int locPxRange;
    uint32_t vao;
    uint32_t quadVbo;
    uint32_t instanceVbo;
    float glowPad;
    SdfCommandBuffer commandBuffer;
    Handle font;
} SdfRenderer;

#define T SdfFont
#define F_PREFIX sdfFont
#define POOL_IMPLEMENTATION
#define POOL_MAX_CAP MAX_SDF_FONTS
#include "pool.h"

typedef struct SdfState {
    SdfRenderer renderer;
    SdfFontPool fonts;
} SdfState;

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

void addGlyph(GlyphData *glyphs, yyjson_val *glyph, size_t idx, float atlasW, float atlasH) {
    yyjson_val *unicode = yyjson_obj_get(glyph, "unicode");
    yyjson_val *advance = yyjson_obj_get(glyph, "advance");
    yyjson_val *planeBounds = yyjson_obj_get(glyph, "planeBounds");
    yyjson_val *atlasBounds = yyjson_obj_get(glyph, "atlasBounds");

    GlyphData glyphData = {0};
    glyphData.codePoint = yyjson_get_int(unicode);

    if (glyphData.codePoint == 72) {
        int a = 1;
    }

    yyjson_val *planeLeft = yyjson_obj_get(planeBounds, "left");
    yyjson_val *planeBottom = yyjson_obj_get(planeBounds, "bottom");
    yyjson_val *planeRight = yyjson_obj_get(planeBounds, "right");
    yyjson_val *planeTop = yyjson_obj_get(planeBounds, "top");
    glyphData.planeCenter.x
        = ((float)yyjson_get_num(planeLeft) + (float)yyjson_get_num(planeRight)) * 0.5f;
    glyphData.planeCenter.y
        = ((float)yyjson_get_num(planeTop) + (float)yyjson_get_num(planeBottom)) * 0.5f;
    glyphData.planeExtent.x
        = ((float)yyjson_get_num(planeRight) - (float)yyjson_get_num(planeLeft)) * 0.5f;
    glyphData.planeExtent.y
        = fabsf((float)yyjson_get_num(planeTop) - (float)yyjson_get_num(planeBottom)) * 0.5f;

    yyjson_val *atlasLeft = yyjson_obj_get(atlasBounds, "left");
    yyjson_val *atlasBottom = yyjson_obj_get(atlasBounds, "bottom");
    yyjson_val *atlasRight = yyjson_obj_get(atlasBounds, "right");
    yyjson_val *atlasTop = yyjson_obj_get(atlasBounds, "top");

    float atlasLeftVal = (float)yyjson_get_num(atlasLeft);
    float atlasRightVal = (float)yyjson_get_num(atlasRight);
    glyphData.uvCenter.x = (atlasLeftVal + atlasRightVal) * 0.5f / atlasW;
    glyphData.uvCenter.y
        = ((float)yyjson_get_num(atlasTop) + (float)yyjson_get_num(atlasBottom)) * 0.5f / atlasH;
    glyphData.uvExtent.x
        = ((float)yyjson_get_num(atlasRight) - (float)yyjson_get_num(atlasLeft)) * 0.5f / atlasW;
    glyphData.uvExtent.y
        = fabsf((float)yyjson_get_num(atlasTop) - (float)yyjson_get_num(atlasBottom)) * 0.5f
          / atlasH;

    glyphData.advance = yyjson_get_num(advance);

    glyphData.uvCenter.y = 1.0f - glyphData.uvCenter.y;
    glyphData.planeCenter.y = -glyphData.planeCenter.y;

    glyphs[idx] = glyphData;
}

SdfFont loadSdfFont(const char *fontName) {
    SdfFont font = {0};
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

    yyjson_val *atlas = yyjson_obj_get(root, "atlas");
    font.metrics.atlasSize = (Vector2){yyjson_get_num(yyjson_obj_get(atlas, "width")),
                                       yyjson_get_num(yyjson_obj_get(atlas, "height"))};
    font.metrics.distanceRange = yyjson_get_num(yyjson_obj_get(atlas, "distanceRange"));

    yyjson_val *metrics = yyjson_obj_get(root, "metrics");
    font.metrics.lineHeight = yyjson_get_num(yyjson_obj_get(metrics, "lineHeight"));
    font.metrics.ascender = yyjson_get_num(yyjson_obj_get(metrics, "ascender"));
    font.metrics.descender = yyjson_get_num(yyjson_obj_get(metrics, "descender"));

    yyjson_val *glyphs = yyjson_obj_get(root, "glyphs");
    size_t glyphCount = yyjson_arr_size(glyphs);
    font.glyphCount = glyphCount;
    font.glyphData = malloc(glyphCount * sizeof(GlyphData));
    size_t idx, max;
    yyjson_val *val;
    yyjson_arr_foreach(glyphs, idx, max, val) {
        addGlyph(font.glyphData, val, idx, font.metrics.atlasSize.x, font.metrics.atlasSize.y);
    }

    pathLen = snprintf(path, sizeof(path), "defaultAssets/fonts/%s.png", fontName);
    if (pathLen < 0 || pathLen >= sizeof(path)) {
        printf("Too long font name");
        return font;
    }

    font.texture = texturePoolAdd(&g_ctx.textures, LoadTexture(path));
    yyjson_doc_free(doc);
    return font;
}

void initSdf(uint32_t screenWidth, uint32_t screenHeight) {
    g_ctx.sdf = malloc(sizeof(SdfState));
    *g_ctx.sdf = (SdfState){0};
    SdfFont defaultFont = loadSdfFont("FiraSans");
    sdfFontPoolInit(&g_ctx.sdf->fonts, defaultFont);
    SdfRenderer *renderer = &g_ctx.sdf->renderer;
    *renderer = (SdfRenderer){0};
    renderer->shader
        = loadSdfShader("defaultAssets/shaders/sdf.vs", "defaultAssets/shaders/sdf.fs");
    renderer->locMvp = GetShaderLocation(renderer->shader, "mvp");
    renderer->locGlowPad = GetShaderLocation(renderer->shader, "glowPad");
    renderer->glowPad = 2.0f;

    renderer->locAtlas = GetShaderLocation(renderer->shader, "uAtlas");
    renderer->locPxRange = GetShaderLocation(renderer->shader, "uPxRange");

    float quad[12] = {
        -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1,
    };

    renderer->vao = rlLoadVertexArray();
    rlEnableVertexArray(renderer->vao);

    renderer->quadVbo = rlLoadVertexBuffer(quad, sizeof(quad), false);
    rlEnableVertexBuffer(renderer->quadVbo);
    rlSetVertexAttribute(0, 2, RL_FLOAT, false, 2 * sizeof(float), 0);
    rlEnableVertexAttribute(0);

    renderer->instanceVbo = rlLoadVertexBuffer(NULL, SDF_MAX_INSTANCES * sizeof(SdfInstance), true);
    rlEnableVertexBuffer(renderer->instanceVbo);
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

void flushSdf(SdfCommandBuffer *buffer, Handle fontHandle) {
    SdfRenderer renderer = g_ctx.sdf->renderer;
    if (buffer->count == 0)
        return;

    rlDrawRenderBatchActive();

    rlUpdateVertexBuffer(renderer.instanceVbo, buffer->instances,
                         buffer->count * sizeof(SdfInstance), 0);

    Matrix mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());

    SdfFont font = *sdfFontPoolGet(&g_ctx.sdf->fonts, fontHandle);
    Texture2D fontTexture = *texturePoolGet(&g_ctx.textures, font.texture);

    rlActiveTextureSlot(0);
    rlEnableTexture(fontTexture.id);
    int unit = 0;
    float pxRange = font.metrics.distanceRange;

    rlEnableShader(renderer.shader.id);

    rlSetUniform(renderer.locAtlas, &unit, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(renderer.locPxRange, &pxRange, RL_SHADER_UNIFORM_FLOAT, 1);

    rlSetUniformMatrix(renderer.locMvp, mvp);
    rlSetUniform(renderer.locGlowPad, &renderer.glowPad, RL_SHADER_UNIFORM_FLOAT, 1);
    rlEnableVertexArray(renderer.vao);
    rlDisableBackfaceCulling();
    rlDrawVertexArrayInstanced(0, 6, buffer->count);
    rlEnableBackfaceCulling();
    rlDisableVertexArray();
    rlDisableShader();

    buffer->count = 0;
}

// SdfInstance *pushSdf(SdfCommandBuffer *buffer) {
//     if (buffer->count == SDF_MAX_INSTANCES) {
//         flushSdf(buffer);
//     }
//     SdfInstance *inst = &buffer->instances[buffer->count++];
//     *inst = (SdfInstance){0};
//     return inst;
// }

void setFont(SdfRenderer *renderer, Handle font) {}

void beginSdfMode() { BeginBlendMode(BLEND_ALPHA_PREMULTIPLY); }

void endSdfMode() { EndBlendMode(); }

void sdfBeginSdfMode(UmkaStackSlot *params, UmkaStackSlot *result) { beginSdfMode(); }

void sdfEndSdfMode(UmkaStackSlot *params, UmkaStackSlot *result) {
    SdfCommandBuffer *buffer = umkaGetParam(params, 0)->ptrVal;
    Handle fontHandle = *(Handle *)umkaGetParam(params, 1);
    flushSdf(buffer, fontHandle);
    endSdfMode();
}

void sdfFlushSdf(UmkaStackSlot *params, UmkaStackSlot *result) {
    SdfCommandBuffer *buffer = umkaGetParam(params, 0)->ptrVal;
    Handle fontHandle = *(Handle *)umkaGetParam(params, 1);
    flushSdf(buffer, fontHandle);
}

void sdfLoadFont(UmkaStackSlot *params, UmkaStackSlot *result) {
    const char *name = (const char *)umkaGetParam(params, 0)->ptrVal;
    SdfFont font = loadSdfFont(name);
    Handle handle = sdfFontPoolAdd(&g_ctx.sdf->fonts, font);
    *(Handle *)umkaGetResult(params, result)->ptrVal = handle;
}

void sdfDefaultFontHandle(UmkaStackSlot *params, UmkaStackSlot *result) {
    *(Handle *)umkaGetResult(params, result)->ptrVal = NULL_HANDLE;
}

void sdfFontGlyphs(UmkaStackSlot *params, UmkaStackSlot *result) {
    Umka *umka = umkaGetInstance(result);

    Handle fontHandle = *(Handle *)umkaGetParam(params, 0);
    const UmkaType *resultType = umkaGetResultType(params, result);

    SdfFont font = *sdfFontPoolGet(&g_ctx.sdf->fonts, fontHandle);
    typedef UmkaDynArray(GlyphData) GlyphDataArray;
    GlyphDataArray *out = umkaGetResult(params, result)->ptrVal;
    umkaMakeDynArray(umka, out, resultType, font.glyphCount);

    memcpy(out->data, font.glyphData, sizeof(GlyphData) * font.glyphCount);
}

void sdfFontMetrics(UmkaStackSlot *params, UmkaStackSlot *result) {
    Handle fontHandle = *(Handle *)umkaGetParam(params, 0);

    SdfFont font = *sdfFontPoolGet(&g_ctx.sdf->fonts, fontHandle);

    *(FontMetrics *)umkaGetResult(params, result)->ptrVal = font.metrics;
}

void sdfAddUmkaModule(Umka *umka) {
    umkaAddFunc(umka, "_endSdfMode", &sdfEndSdfMode);
    umkaAddFunc(umka, "_beginSdfMode", &sdfBeginSdfMode);
    umkaAddFunc(umka, "_flush", &sdfFlushSdf);
    umkaAddFunc(umka, "_loadFont", &sdfLoadFont);
    umkaAddFunc(umka, "sdfFontMetrics", &sdfFontMetrics);
    umkaAddFunc(umka, "sdfFontGlyphs", &sdfFontGlyphs);
    umkaAddFunc(umka, "sdfDefaultFontHandle", &sdfDefaultFontHandle);

    const char *umSourceNames[] = {"sdf.um"};
    const char *umSourceFiles[] = {(const char[]){
#embed "sdf.um"
        , '\0'}};
    for (int i = 0; i < sizeof(umSourceFiles) / sizeof(umSourceFiles[0]); i++) {
        umkaAddModule(umka, umSourceNames[i], umSourceFiles[i]);
    }
}
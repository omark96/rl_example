precision highp float;
precision highp int;

in vec2 vLocal;
flat in vec4 vParams;
flat in vec4 vColor;
flat in vec2 vExtent;
flat in int vType;
flat in float vStroke;

uniform sampler2D uAtlas;
uniform float uPxRange;

out vec4 fragColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

float sdRoundRectangle(vec2 p, vec2 extent, float borderRadius){
    vec2 q = abs(p) - extent + borderRadius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - borderRadius;
}

void main() {
    float d = 1e6;
    switch (vType){
        case 0:
            d = sdCircle(vLocal, vParams.x);
            break;
        case 1:
            d = sdRoundRectangle(vLocal, vExtent, vParams.x);
            break;
        case 2:
            vec2 uvCenter = vParams.xy;
            vec2 uvExtent = vParams.zw;
            vec2 t = clamp(vLocal / vExtent, -1.0, 1.0);
            vec2 uv = uvCenter + t * uvExtent;

            vec3 msd = texture(uAtlas, uv).rgb;
            float sd = median(msd.r, msd.g, msd.b);

            vec2 atlasPxPerLocal = uvExtent * vec2(textureSize(uAtlas, 0)) / vExtent;
            float pxPerLocal = 0.5 * (atlasPxPerLocal.x + atlasPxPerLocal.y);

            d = (0.5 - sd) * uPxRange / pxPerLocal;
        default:
            break;
    }

    if (vStroke > 0.0) {
        d = abs(d + vStroke * 0.5) - vStroke * 0.5;
    }

    float w = fwidth(d);
    float coverage = clamp(0.5 - d / max(w, 1e-6), 0.0, 1.0);

    float a = vColor.a * coverage;
    fragColor = vec4(vColor.rgb * a, a);
}
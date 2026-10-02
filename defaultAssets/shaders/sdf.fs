precision highp float;
precision highp int;

in vec2 vLocal;
flat in vec4 vParams;
flat in vec4 vColor;
flat in vec2 vExtent;
flat in int vType;
flat in float vStroke;

out vec4 fragColor;

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
layout(location = 0) in vec2 aCorner;
layout(location = 1) in vec4 aCenterHalf;
layout(location = 2) in vec4 aParams;
layout(location = 3) in vec4 aColor;
layout(location = 4)  in vec4 aMisc;

uniform mat4 mvp;
uniform float glowPad;

out vec2 vLocal;
flat out vec4 vParams;
flat out vec4 vColor;
flat out vec2 vExtent;
flat out int vType;
flat out float vStroke;


void main() {
    vec2 extent = aCenterHalf.zw + glowPad;
    vLocal = aCorner * extent;

    float rot = aMisc.z;
    float cosRot = cos(rot);
    float sinRot = sin(rot);
    vec2 rotation = vec2(cosRot * vLocal.x - sinRot * vLocal.y, 
                         sinRot * vLocal.x + cosRot * vLocal.y);

    vParams = aParams;
    vExtent = aCenterHalf.zw;
    vColor = vec4(aColor.rgb * aMisc.x, aColor.a);
    vType = int(aMisc.y + 0.5);
    vStroke = aMisc.w;

    gl_Position = mvp * vec4(aCenterHalf.xy + rotation, 0.0, 1.0);
}
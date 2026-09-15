#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 itemSize;
    vec2 origin;
    float rounding;
    float reach;
    float strength;
    vec4 colRipple;
};

void main()
{
    vec2 point = qt_TexCoord0 * itemSize;
    vec2 middle = itemSize * 0.5;
    float radius = min(rounding, min(middle.x, middle.y));
    vec2 edge = abs(point - middle) - middle + radius;
    float border = length(max(edge, 0.0)) + min(max(edge.x, edge.y), 0.0) - radius;
    float inside = 1.0 - smoothstep(-0.5, 0.5, border);
    float falloff = clamp((reach - distance(point, origin)) / max(reach * 0.4, 0.001), 0.0, 1.0);
    float alpha = qt_Opacity * strength * inside * falloff;
    fragColor = vec4(colRipple.rgb * alpha, alpha);
}

#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 itemSize;
    float rounding;
};

layout(binding = 1) uniform sampler2D source;

void main()
{
    vec2 middle = itemSize * 0.5;
    float radius = min(rounding, min(middle.x, middle.y));
    vec2 edge = abs(qt_TexCoord0 * itemSize - middle) - middle + radius;
    float reach = length(max(edge, 0.0)) + min(max(edge.x, edge.y), 0.0) - radius;
    float inside = 1.0 - smoothstep(-0.5, 0.5, reach);
    fragColor = texture(source, qt_TexCoord0) * (qt_Opacity * inside);
}

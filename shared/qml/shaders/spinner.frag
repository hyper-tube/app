#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 itemSize;
    vec4 colArc;
    float thickness;
    float head;
    float sweep;
};

const float kTurn = 6.28318530718;

float capCoverage(vec2 point, float radius, float turn, float reach)
{
    vec2 centre = radius * vec2(sin(turn * kTurn), -cos(turn * kTurn));
    return 1.0 - smoothstep(reach - 0.75, reach + 0.75, distance(point, centre));
}

void main()
{
    vec2 middle = itemSize * 0.5;
    vec2 point = qt_TexCoord0 * itemSize - middle;
    float reach = thickness * 0.5;
    float radius = min(middle.x, middle.y) - reach - 1.0;

    float band = 1.0 - smoothstep(-0.75, 0.75, abs(length(point) - radius) - reach);
    float turn = mod(atan(point.x, -point.y) / kTurn, 1.0);
    float span = mod(turn - head, 1.0) <= sweep ? band : 0.0;

    float coverage = max(span, max(capCoverage(point, radius, head, reach),
                                   capCoverage(point, radius, head + sweep, reach)));
    float alpha = qt_Opacity * clamp(coverage, 0.0, 1.0);
    fragColor = vec4(colArc.rgb * alpha, alpha);
}

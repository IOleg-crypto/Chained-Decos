#version 330 core

in vec3 v_WorldPos;

uniform vec3 u_CameraPos;
uniform vec4 u_GridColor;   // rgb = color, a = base alpha
uniform float u_GridSize;   // primary spacing
uniform float u_FadeStart;  // distance where fade begins
uniform float u_FadeEnd;    // distance where grid is fully invisible

out vec4 o_Color;

float grid(vec2 coord, float spacing)
{
    vec2 g = abs(fract(coord / spacing - 0.5) - 0.5) / (fwidth(coord / spacing) + 0.001);
    return 1.0 - min(min(g.x, g.y), 1.0);
}

void main()
{
    vec2 coord = v_WorldPos.xz;

    // Primary grid lines
    float g1 = grid(coord, u_GridSize);
    // Secondary grid lines (every 10th)
    float g2 = grid(coord, u_GridSize * 10.0);

    // Distance-based fade
    float dist = length(v_WorldPos - u_CameraPos);
    float fade = 1.0 - clamp((dist - u_FadeStart) / (u_FadeEnd - u_FadeStart), 0.0, 1.0);
    fade *= fade; // quadratic for smoother transition

    // Combine: primary lines at 40% alpha, secondary at 80%
    float alpha = max(g1 * 1, g2 * 0.8) * fade;

    if (alpha < 0.01) discard;

    o_Color = vec4(u_GridColor.rgb, alpha * u_GridColor.a);
}

#version 430 core

in vec3 v_WorldPos;

uniform vec3 u_CameraPos;
uniform vec4 u_GridColor;   // rgb = color, a = base alpha
uniform float u_GridSize;   // primary spacing
uniform float u_SecondarySpacing; // secondary spacing (major gridlines)
uniform float u_FadeStart;  // distance where fade begins
uniform float u_FadeEnd;    // distance where grid is fully invisible

out vec4 o_Color;

float grid(vec2 coord, float spacing, float lineWidth)
{
    vec2 grid_coord = coord / spacing;
    vec2 derivative = fwidth(grid_coord);

    vec2 scaledLine = lineWidth * derivative;

    vec2 grid_val = abs(fract(grid_coord - 0.5) - 0.5);

    vec2 aa = smoothstep(scaledLine, vec2(0.0), grid_val);

    return max(aa.x, aa.y);
}

void main()
{
    vec2 coord = v_WorldPos.xz;

    float g1 = grid(coord, u_GridSize, 0.8);
    float g2 = grid(coord, u_SecondarySpacing, 0.5);

    float dist = length(v_WorldPos - u_CameraPos);
    float fade_range = max(u_FadeEnd - u_FadeStart, 0.001);
    float fade = 1.0 - clamp((dist - u_FadeStart) / fade_range, 0.0, 1.0);
    fade = fade * fade;

    vec3 viewDir = normalize(u_CameraPos - v_WorldPos);
    float viewAngle = abs(viewDir.y);
    float angleFade = smoothstep(0.0, 0.3, viewAngle);

    float alpha_mask = max(g1 * 0.3, g2 * 0.6);
    float final_alpha = alpha_mask * fade * angleFade;

    if (final_alpha < 0.005) discard;

    o_Color = vec4(u_GridColor.rgb, final_alpha * u_GridColor.a);
}

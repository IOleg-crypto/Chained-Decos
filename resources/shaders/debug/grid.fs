#version 330 core

// Input vertex attributes (from vertex shader)
in vec3 fragWorldPos;

// Input uniform values
uniform mat4 mvp;
uniform vec3 cameraPos;
uniform vec4 gridColor;
uniform float gridSize; // Spacing (e.g. 1.0 or 10.0)
uniform float uTime;

#include "../include/fog.glsl"

// Output fragment color
out vec4 finalColor;

float grid(vec2 st, float res)
{
    vec2 grid = fract(st / res);
    vec2 line = step(vec2(1.0 - 0.02 / res), grid) + step(grid, vec2(0.02 / res));
    return max(line.x, line.y);
}

// Improved grid with anti-aliasing based on screen derivatives
float pr_grid(vec2 coord, float spacing) 
{
    vec2 grid = abs(fract(coord / spacing - 0.5) - 0.5) / (fwidth(coord / spacing) + 0.001);
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

void main()
{
    // Access world-space XZ for tiling
    vec2 coord = fragWorldPos.xz;
    
    // Primary grid (spacing from uniform)
    float g1 = pr_grid(coord, gridSize);
    
    // Secondary thick grid (every 10 primary units)
    float g2 = pr_grid(coord, gridSize * 10.0);
    
    // Distance from camera to this fragment
    float dist = length(fragWorldPos - cameraPos);
    
    // Fade out based on distance (linear fade from 100m start to 8000m end)
    float fade = 1.0 - clamp((dist - 100.0) / 7900.0, 0.0, 1.0);
    // Use quadratic falloff for smoother horizon transition
    fade = fade * fade;
    
    // Mix grid lines
    float alpha = max(g1 * 0.4, g2 * 0.8);
    
    if (alpha * fade < 0.01) discard;
    
    // Use the uniform gridColor
    vec4 result = vec4(gridColor.rgb, alpha * fade * gridColor.a);
    finalColor = ApplyFog(result, fragWorldPos, cameraPos, uTime);
}

#version 430 core

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec3 fragPosition;

// Input uniform values
layout(binding = 0) uniform sampler2D environmentMap;
uniform int doGamma;
uniform float fragGamma;
uniform float exposure;
uniform float brightness;
uniform float contrast;
uniform float uTime;
uniform int isHDR;

#include "../include/fog_skybox.glsl"

// Output fragment color
layout(location = 0) out vec4 finalColor;

// Tone mapping (ACES)
vec3 ACES(vec3 x)
{
    return clamp(
        (x * (2.51 * x + 0.03)) /
        (x * (2.43 * x + 0.59) + 0.14),
        0.0, 1.0
    );
}

#define PI 3.14159265358979323846

void main()
{
    vec3 v = normalize(fragPosition);
    
    // Convert Cartesian direction to equirectangular UVs
    vec2 uv = vec2(atan(v.z, v.x) / (2.0 * PI) + 0.5, asin(clamp(v.y, -1.0, 1.0)) / PI + 0.5);
    
    // Filter derivative seam
    vec2 dX = dFdx(uv);
    vec2 dY = dFdy(uv);
    if (dX.x > 0.5) dX.x -= 1.0;
    else if (dX.x < -0.5) dX.x += 1.0;
    if (dY.x > 0.5) dY.x -= 1.0;
    else if (dY.x < -0.5) dY.x += 1.0;

    // Fetch color from texture map with filtered derivatives
    vec3 color = textureGrad(environmentMap, uv, dX, dY).rgb;

    // Apply exposure, brightness, contrast
    color *= exposure;
    color += brightness;
    color = (color - 0.5) * contrast + 0.5;

    // HDR tone mapping
    if (isHDR == 1)
        color = ACES(color);

    if (doGamma == 1)
    {
        color = pow(color, vec3(1.0 / fragGamma));
    }

    vec4 background = vec4(color, 1.0);

    if (fogEnabled == 1)
    {
        float horizonEffect = 1.0 - abs(v.y);
        horizonEffect = pow(horizonEffect, 3.0);
        float fogFactor = clamp(horizonEffect * clamp(fogDensity * 10.0, 0.0, 1.0), 0.0, 1.0);
        finalColor = mix(background, fogColor, fogFactor);
    }
    else
    {
        finalColor = background;
    }
}

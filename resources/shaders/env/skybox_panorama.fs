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

float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(p + vec3(0, 0, 0)), hash(p + vec3(1, 0, 0)), f.x),
                   mix(hash(p + vec3(0, 1, 0)), hash(p + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(hash(p + vec3(0, 0, 1)), hash(p + vec3(1, 0, 1)), f.x),
                   mix(hash(p + vec3(0, 1, 1)), hash(p + vec3(1, 1, 1)), f.x), f.y), f.z);
}

#define PI 3.14159265358979323846

void main()
{
    vec3 v = normalize(fragPosition);
    
    // Convert Cartesian direction to equirectangular UVs
    vec2 uv = vec2(atan(v.z, v.x) / (2.0 * PI) + 0.5, asin(clamp(v.y, -1.0, 1.0)) / PI + 0.5);
    
    // Filter derivative seam: across the wrap boundary (uv.x jumping 0 -> 1),
    // derivatives jump to ~1.0 causing mipmap popping / vertical line artifact.
    vec2 dX = dFdx(uv);
    vec2 dY = dFdy(uv);
    if (dX.x > 0.5) dX.x -= 1.0;
    else if (dX.x < -0.5) dX.x += 1.0;
    if (dY.x > 0.5) dY.x -= 1.0;
    else if (dY.x < -0.5) dY.x += 1.0;

    // Fetch color from texture map with filtered derivatives
    vec3 color = textureGrad(environmentMap, uv, dX, dY).rgb;

    // Apply exposure
    color *= exposure;

    // Apply brightness
    color += brightness;

    // Apply contrast
    color = (color - 0.5) * contrast + 0.5;

    // HDR tone mapping
    if (isHDR == 1)
        color = ACES(color);

    if (doGamma == 1) // Apply gamma correction
    {
        color = pow(color, vec3(1.0/fragGamma));
    }

    vec4 background = vec4(color, 1.0);

    if (fogEnabled == 1)
    {
        // Horizon fog
        float horizonEffect = 1.0 - abs(v.y);
        horizonEffect = pow(horizonEffect, 3.0);
        
        // Volumetric noise
        vec3 noiseDir = v * 2.5 + vec3(uTime * 0.04, uTime * 0.01, uTime * 0.03);
        float n = noise(noiseDir);
        
        float fogFactor = clamp(horizonEffect + n * 0.4, 0.0, 1.0);
        fogFactor = mix(fogFactor, 1.0, clamp(fogDensity * 10.0, 0.0, 1.0));
        
        finalColor = mix(background, fogColor, fogFactor);
    }
    else
    {
        finalColor = background;
    }
}

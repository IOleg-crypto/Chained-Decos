#version 450 core

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec3 fragPosition;

// Input uniform values
layout(binding = 0) uniform sampler2D environmentMap;
uniform bool doGamma;
uniform float fragGamma;
uniform float exposure;
uniform float brightness;
uniform float contrast;
uniform float uTime;

// Fog data
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

// Output fragment color
layout(location = 0) out vec4 finalColor;

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
    vec2 uv = vec2(atan(v.z, v.x) / (2.0 * PI) + 0.5, asin(v.y) / PI + 0.5);
    
    // Fetch color from texture map
    vec3 color = texture(environmentMap, uv).rgb;

    // Apply exposure
    color *= exposure;

    // Apply brightness
    color += brightness;

    // Apply contrast
    color = (color - 0.5) * contrast + 0.5;

    if (doGamma) // Apply gamma correction
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

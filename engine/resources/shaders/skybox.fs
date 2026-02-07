#version 450 core

// Input vertex attributes (from vertex shader)
layout(location = 0) in vec3 fragPosition;

// Input uniform values
layout(binding = 0) uniform sampler2D environmentMap;
uniform bool vflipped;
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

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec3 dir = normalize(fragPosition);
    vec2 uv = SampleSphericalMap(dir);
    
    if (vflipped) uv.y = 1.0 - uv.y;

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
        // Horizon fog: thicker near the horizon line
        float horizonEffect = 1.0 - abs(dir.y);
        horizonEffect = pow(horizonEffect, 3.0);
        
        // Volumetric noise for "mist patches" in the distance
        vec3 noiseDir = dir * 2.0 + vec3(uTime * 0.05, uTime * 0.02, uTime * 0.03);
        float n = noise(noiseDir);
        
        // Combine horizon and noise, modulated by fog density
        float fogFactor = clamp(horizonEffect + n * 0.4, 0.0, 1.0);
        
        // Apply global density boost
        fogFactor = mix(fogFactor, 1.0, clamp(fogDensity * 10.0, 0.0, 1.0));
        
        finalColor = mix(background, fogColor, fogFactor);
    }
    else
    {
        finalColor = background;
    }
}

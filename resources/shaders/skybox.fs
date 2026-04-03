#version 450 core
#include "include/color_space.glsl"

in vec3 v_Position;

layout(binding = 0) uniform sampler2D u_Panorama;

uniform int u_IsHDR;
uniform float u_Exposure;
uniform float u_Brightness;
uniform float u_Contrast;
uniform int u_VFlipped;

// Fog uniforms
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;

layout(location = 0) out vec4 finalColor;

vec2 SampleSpherical(vec3 dir)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    return uv + 0.5;
}

void main()
{
    vec3 direction = normalize(v_Position);
    vec2 uv = SampleSpherical(direction);
    if (u_VFlipped == 1) uv.y = 1.0 - uv.y;

    // Sample the panorama
    vec3 color = texture(u_Panorama, uv).rgb;

    // Convert to Linear if LDR (PNG/JPG)
    if (u_IsHDR == 0) color = pow(color, vec3(2.2));

    // 1. Exposure & Color Correction
    color *= u_Exposure;
    color = color + u_Brightness;
    color = pow(max(color, vec3(0.0001)), vec3(u_Contrast));

    // 2. ACES Tonemapping
    vec3 x = color;
    vec3 mapped = (x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14);
    mapped = clamp(mapped, 0.0, 1.0);
    
    vec4 background = vec4(mapped, 1.0);

    if (fogEnabled == 1) {
        float verticalFactor = clamp(1.0 - (direction.y + 0.05) * 10.0, 0.0, 1.0);
        float fogFactor = pow(verticalFactor, 2.0); 
        float horizonHaze = pow(1.0 - abs(direction.y), 5.0) * 0.5;
        fogFactor = max(fogFactor, horizonHaze);
        fogFactor = clamp(fogFactor * clamp(fogDensity * 5.0, 0.0, 1.0), 0.0, 1.0);
        finalColor = mix(background, fogColor, fogFactor);
    } else {
        finalColor = background;
    }
}

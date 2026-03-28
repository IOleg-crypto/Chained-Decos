#version 450 core
#include "include/color_space.glsl"

in vec3 v_Position;

uniform samplerCube environmentMap;

uniform int isHDR;
uniform float exposure;
uniform float brightness;
uniform float contrast;

// Fog uniforms
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;

layout(location = 0) out vec4 finalColor;

void main()
{
    vec3 direction = normalize(v_Position);
    
    // Sample the environment cubemap
    vec3 color = texture(environmentMap, direction).rgb;

    // 1. Convert to Linear if LDR
    if (isHDR == 0) color = pow(color, vec3(2.2));

    // 2. Exposure & Basic Tonemapping (ACES-like)
    color *= exposure;
    
    // Apply contrast/brightness in linear space
    color = max(color + brightness, vec3(0.0));
    color = pow(max(color, vec3(0.0)), vec3(contrast)); // Use contrast as power for HDR

    // 3. Simple Tonemapping for HDR (Reinhard) 
    // This prevents burning if post-process is bypassed or for preview
    vec3 mapped = color / (color + vec3(1.0));
    
    vec4 background = vec4(mapped, 1.0);

    // 4. Unified Horizon & Ground Fog
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

#version 450 core
#include "include/color_space.glsl"

in vec2 fragTexCoord; 

uniform sampler2D texture0;

uniform int vflipped;
uniform int isHDR;
uniform float exposure;
uniform float brightness;
uniform float contrast;
uniform int skyboxMode; // 0: Sphere, 1: Direct

uniform mat4 projection;
uniform mat4 view;

// Fog uniforms
uniform int fogEnabled;
uniform vec4 fogColor;
uniform float fogDensity;

layout(location = 0) out vec4 finalColor;

// Converts 3D direction to Spherical UVs for equirectangular panorama
vec2 SampleSpherical(vec3 dir)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    return uv + 0.5;
}

void main()
{
    // 1. Reconstruct world-space direction from UV
    // Map [0,1] UV to [-1,1] NDC
    vec4 clipPos = vec4(fragTexCoord * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewPos = inverse(projection) * clipPos;
    viewPos /= viewPos.w;
    
    // View matrix passed to shader already has translation removed for cubemaps,
    // but for panoramas it might still be there or we use this reconstruction.
    mat3 invRotView = inverse(mat3(view));
    vec3 direction = normalize(invRotView * viewPos.xyz);
    
    vec2 uv;
    if (skyboxMode == 1) {
        uv = fragTexCoord;
    } else {
        uv = SampleSpherical(direction);
    }

    if (vflipped == 1) uv.y = 1.0 - uv.y;

    // Sample the panorama
    vec3 color = texture(texture0, uv).rgb;

    // Convert to Linear if LDR (PNG/JPG)
    if (isHDR == 0) color = ToLinear(color);

    // 1. Exposure & Basic Tonemapping
    color *= exposure;
    
    // Apply contrast/brightness in linear space
    color = max(color + brightness, vec3(0.0));
    color = pow(max(color, vec3(0.0)), vec3(contrast));

    // 2. Simple Tonemapping for preview
    vec3 mapped = color / (color + vec3(1.0));
    
    vec4 background = vec4(mapped, 1.0);

    // 2. Unified Horizon & Ground Fog
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
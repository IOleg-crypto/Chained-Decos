#version 450 core

layout(location = 0) in vec3 fragPosition;

uniform samplerCube environmentMap;

uniform int isHDR;
uniform float exposure;
uniform float brightness;
uniform float contrast;

layout(location = 0) out vec4 finalColor;

// Note: Tonemapping and Gamma correction are handled globally in post_process.fs

void main()
{
    vec3 direction = normalize(fragPosition);
    
    // Sample the environment cubemap
    vec3 color = texture(environmentMap, direction).rgb;

    // 1. Exposure & Color Corrections
    color *= exposure;
    color += brightness;
    color = (color - 0.5) * contrast + 0.5;

    // 2. Output Linear HDR Color
    // Tonemapping will be handled globally in post_process.fs
    
    finalColor = vec4(color, 1.0);
}
